//===- DeleteRedundantPollchecks.cpp - delete redundant pollchecks --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of Fil-C. https://fil-c.org/
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Instrumentation/DeleteRedundantPollchecks.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

static cl::opt<bool> hoistLoopChecks(
    "filc-hoist-loop-checks",
    cl::desc("Load the upper bounds and flags that a loop's access checks use "
             "once before the loop, reloading them after each slow pollcheck"),
    cl::Hidden, cl::init(true));

static constexpr uint64_t ObjectAuxFlagsShift = 48;
static constexpr uint64_t ObjectFlagReadonly = 2;
static constexpr uint64_t ObjectFlagFree = 4;

static constexpr unsigned NumUnwindRegisters = 2;

static constexpr uint8_t ThreadStateCheckRequested = 2;
static constexpr uint8_t ThreadStateStopRequested = 4;
static constexpr uint8_t ThreadStateDeferredSignal = 8;

struct DeleteRedundantPollchecks {
  DeleteRedundantPollchecks(Module &M) : M(M) {}

  Module &M;
  LLVMContext &C = M.getContext();

  static constexpr unsigned TargetAS = 0;

  const DataLayout DL = M.getDataLayoutAfterFilC();

  unsigned PtrBits = DL.getPointerSizeInBits(TargetAS);
  Type *VoidTy = Type::getVoidTy(C);
  IntegerType *Int8Ty = Type::getInt8Ty(C);
  IntegerType *Int32Ty = Type::getInt32Ty(C);
  IntegerType *IntPtrTy = Type::getIntNTy(C, PtrBits);
  PointerType *RawPtrTy = PointerType::get(C, TargetAS);
  StructType *FlightPtrTy =
      StructType::create({RawPtrTy, RawPtrTy}, "filc_flight_ptr");
  FunctionCallee Pollcheck =
      M.getOrInsertFunction("filc_pollcheck", VoidTy, RawPtrTy, RawPtrTy);
  FunctionCallee PollcheckSlow =
      M.getOrInsertFunction("filc_pollcheck_slow", VoidTy, RawPtrTy, RawPtrTy);

  StructType *ThreadTy = StructType::create(
      {
          IntPtrTy, // stack_limit, index 0
          Int8Ty,   // state, index 1
          Int32Ty,  // tid, index 2
          RawPtrTy, // top_frame, index 3
          RawPtrTy, // alignment word, index 4
          ArrayType::get(FlightPtrTy,
                         NumUnwindRegisters), // unwind_registers, index 5
          FlightPtrTy,                        // cookie_ptr, index 6
          Int8Ty,                             // alignment point, index 7
      },
      "filc_thread_ish_base2");

  Value *threadFieldPtr(Value *Thread, unsigned FieldIndex, const char *Name,
                        Instruction *InsertBefore) {
    GetElementPtrInst *GEP = GetElementPtrInst::Create(
        ThreadTy, Thread,
        {ConstantInt::get(IntPtrTy, 0), ConstantInt::get(Int32Ty, FieldIndex)},
        Name, InsertBefore);
    GEP->setDebugLoc(InsertBefore->getDebugLoc());
    return GEP;
  }

  Value *threadStatePtr(Value *Thread, Instruction *InsertBefore) {
    return threadFieldPtr(Thread, 1, "filc_thread_state_ptr", InsertBefore);
  }

  // Access checks load an object's upper bound and aux word from its header
  // on every iteration of a loop, because the loop's pollcheck is a call
  // that may free the object (free sets upper to lower and sets the free
  // flag). Only the pollcheck's slow path can do that: the fast path is a
  // load and a branch. So in a loop whose only returning calls are slow
  // pollchecks, the check loads happen once in the preheader and again
  // right after each slow pollcheck, and SSA carries them across fast
  // iterations. The checks themselves stay where they were, so a failing
  // check traps at the same access as before.
  //
  // Only loads FilPizlonator marked as feeding checks are touched, and of an
  // aux load only uses that test the free and readonly flags: the aux
  // pointer and the other flags can change without a call (lazy aux
  // creation, weak-key bookkeeping). The preheader load must be safe to
  // execute even if the loop never reaches the access, so the object's
  // lower must be known nonnull there; a nonnull lower is the lower of a
  // live object whose header is readable.
  static bool isCheckFailure(const CallBase *CB) {
    const Function *F = CB->getCalledFunction();
    if (!F || !isa<UnreachableInst>(CB->getNextNode()))
      return false;
    StringRef Name = F->getName();
    return Name == "filc_optimized_access_check_fail" ||
           Name == "filc_masked_write_check_fail" ||
           Name == "filc_masked_read_check_fail" ||
           Name == "filc_compress_write_check_fail" ||
           Name == "filc_expand_read_check_fail";
  }

  bool loopIsEligible(Loop *L) {
    if (!L->isInnermost() || !L->getLoopPredecessor())
      return false;
    for (BasicBlock *BB : L->blocks()) {
      for (Instruction &I : *BB) {
        if (I.isAtomic() || I.isVolatile() || isa<FenceInst>(I))
          return false;
        if (auto *CB = dyn_cast<CallBase>(&I)) {
          if (isa<DbgInfoIntrinsic>(CB))
            continue;
          if (CB->getCalledOperand() == PollcheckSlow.getCallee() ||
              isCheckFailure(CB))
            continue;
          return false;
        }
      }
    }
    return true;
  }

  // Only flags tests: (aux & mask) or ((aux >> shift) & mask) with mask
  // limited to the free and readonly flags.
  static bool isFlagsTest(User *U, Value *Aux) {
    auto *BO = dyn_cast<BinaryOperator>(U);
    if (!BO || BO->getOperand(0) != Aux)
      return false;
    const uint64_t Allowed = ObjectFlagReadonly | ObjectFlagFree;
    if (BO->getOpcode() == Instruction::And) {
      auto *Mask = dyn_cast<ConstantInt>(BO->getOperand(1));
      return Mask && !(Mask->getZExtValue() & ~(Allowed << ObjectAuxFlagsShift));
    }
    if (BO->getOpcode() == Instruction::LShr) {
      auto *Shift = dyn_cast<ConstantInt>(BO->getOperand(1));
      if (!Shift || Shift->getZExtValue() != ObjectAuxFlagsShift)
        return false;
      for (User *UU : BO->users()) {
        auto *And = dyn_cast<BinaryOperator>(UU);
        auto *Mask = And && And->getOpcode() == Instruction::And
                         ? dyn_cast<ConstantInt>(And->getOperand(1))
                         : nullptr;
        if (!Mask || (Mask->getZExtValue() & ~Allowed))
          return false;
      }
      return true;
    }
    return false;
  }

  bool hoistChecksInLoop(Loop *L, DominatorTree &DT, LoopInfo &LI) {
    if (!loopIsEligible(L))
      return false;
    BasicBlock *Preheader = L->getLoopPreheader();
    if (!Preheader)
      Preheader = InsertPreheaderForLoop(L, &DT, &LI, nullptr, false);
    if (!Preheader)
      return false;
    std::vector<CallInst *> Slow;
    for (BasicBlock *BB : L->blocks())
      for (Instruction &I : *BB)
        if (auto *CI = dyn_cast<CallInst>(&I))
          if (CI->getCalledOperand() == PollcheckSlow.getCallee())
            Slow.push_back(CI);
    bool Changed = false;
    for (BasicBlock *BB : L->blocks()) {
      for (Instruction &I : *BB) {
        auto *LI = dyn_cast<LoadInst>(&I);
        if (!LI)
          continue;
        bool IsUpper = LI->getMetadata("filc.check.upper");
        bool IsAux = LI->getMetadata("filc.check.aux");
        if (!IsUpper && !IsAux)
          continue;
        auto *GEP = dyn_cast<GetElementPtrInst>(LI->getPointerOperand());
        if (!GEP || !L->hasLoopInvariantOperands(GEP))
          continue;
        Value *Lower = GEP->getPointerOperand();
        if (!isKnownNonZero(Lower, SimplifyQuery(DL, &DT, nullptr,
                                                 Preheader->getTerminator())))
          continue;
        std::vector<Use *> Uses;
        for (Use &U : LI->uses()) {
          auto *UI = cast<Instruction>(U.getUser());
          if (!L->contains(UI))
            continue;
          if (IsAux && !isFlagsTest(UI, LI))
            continue;
          Uses.push_back(&U);
        }
        if (Uses.empty())
          continue;
        auto Reload = [&](Instruction *InsertBefore) {
          Instruction *Ptr = GEP->clone();
          Ptr->insertBefore(InsertBefore);
          auto *NewLI = cast<LoadInst>(LI->clone());
          NewLI->setOperand(0, Ptr);
          NewLI->insertBefore(InsertBefore);
          return NewLI;
        };
        SSAUpdater SSA;
        SSA.Initialize(LI->getType(), LI->getName());
        SSA.AddAvailableValue(Preheader, Reload(Preheader->getTerminator()));
        for (CallInst *CI : Slow)
          SSA.AddAvailableValue(CI->getParent(), Reload(CI->getNextNode()));
        for (Use *U : Uses)
          SSA.RewriteUse(*U);
        Changed = true;
      }
    }
    return Changed;
  }

  void run() {
    MDBuilder MDB(M.getContext());

    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      DominatorTree DT(F);
      LoopInfo LI(DT);

      std::vector<CallInst *> PollchecksToDelete;
      std::vector<CallInst *> PollchecksToKeep;
      DenseMap<Loop *, std::vector<BasicBlock *>> BlocksWithPollcheck;

      ReversePostOrderTraversal<Function *> RPOT(&F);
      for (BasicBlock *BB : RPOT) {
        bool SeenPollcheck = false;
        Loop *L = LI.getLoopFor(BB);
        auto &BlockList = BlocksWithPollcheck[L];
        for (auto &I : *BB) {
          if (auto *CI = dyn_cast<CallInst>(&I)) {
            if (CI->getCalledOperand() == Pollcheck.getCallee()) {
              if (SeenPollcheck)
                PollchecksToDelete.push_back(CI);
              else {
                SeenPollcheck = true;
                if (llvm::any_of(BlockList, [&](BasicBlock *OtherBB) {
                      return DT.dominates(OtherBB, BB);
                    }))
                  PollchecksToDelete.push_back(CI);
                else
                  PollchecksToKeep.push_back(CI);
                BlockList.push_back(BB);
              }
            }
          }
        }
      }

      for (Instruction *I : PollchecksToDelete)
        I->eraseFromParent();
      for (CallInst *I : PollchecksToKeep) {
        Value *MyThread = I->getArgOperand(0);
        DebugLoc DL = I->getDebugLoc();
        Value *StatePtr = threadStatePtr(MyThread, I);
        LoadInst *StateLoad =
            new LoadInst(Int8Ty, StatePtr, "filc_thread_state_load", I);
        StateLoad->setDebugLoc(DL);
        BinaryOperator *Masked = BinaryOperator::Create(
            Instruction::And, StateLoad,
            ConstantInt::get(Int8Ty, ThreadStateCheckRequested |
                                         ThreadStateStopRequested |
                                         ThreadStateDeferredSignal),
            "filc_thread_state_masked", I);
        Masked->setDebugLoc(DL);
        ICmpInst *PollcheckNotNeeded = new ICmpInst(
            I, ICmpInst::ICMP_EQ, Masked, ConstantInt::get(Int8Ty, 0),
            "filc_pollcheck_not_needed");
        PollcheckNotNeeded->setDebugLoc(DL);
        Instruction *NewTerm = SplitBlockAndInsertIfElse(
            PollcheckNotNeeded, I, false, MDB.createBranchWeights(2000, 1),
            nullptr, &LI);
        I->removeFromParent();
        I->insertBefore(NewTerm);
        I->setCalledOperand(PollcheckSlow.getCallee());
      }

      if (hoistLoopChecks) {
        DominatorTree DT(F);
        LoopInfo LI(DT);
        for (Loop *L : LI.getLoopsInPreorder())
          hoistChecksInLoop(L, DT, LI);
      }
    }
  }
};

PreservedAnalyses DeleteRedundantPollchecksPass::run(Module &M,
                                                     ModuleAnalysisManager &) {
  DeleteRedundantPollchecks P(M);
  P.run();
  return PreservedAnalyses::none();
}
