// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -mllvm -filc-light-verbose -o %t %s 2>&1 | FileCheck %s --check-prefix=PRE
// RUN: FileCheck %s --check-prefix=POST < %t

// Check the early-optimized module, before FilPizlonator. Pointer-bearing
// storage must cover exactly the AST size, including an odd-sized tail.
union __attribute__((packed)) Odd {
  void *pointer;
  unsigned char bytes[9];
};
Odd odd;
// PRE: %union.Odd = type { <{ [1 x ptr], i8 }> }
static_assert(sizeof(Odd) == 9);

struct Owner { int value; };
union Member { int Owner::*member; void *pointer; };
struct __attribute__((packed)) Packed { char lead; Member member; char tail; };
Packed packed;
thread_local Packed tlsPacked;

// A data-member null is -1, not zero. Its packed static/TLS representation
// must remain integer data, not an unaligned synthetic inttoptr initializer.
// PRE: @packed = {{.*}}global <{ i8, { i64 }, i8 }> <{ i8 0, { i64 } { i64 -1 }, i8 0 }>, align 1
// PRE: @tlsPacked = {{.*}}thread_local global <{ i8, { i64 }, i8 }> <{ i8 0, { i64 } { i64 -1 }, i8 0 }>, align 1

// Internal packed offsets require an opaque copy even with aligned bases.
// PRE-LABEL: define {{.*}} @copy_packed(
// PRE: call void @zmemmove_builtin({{.*}}i64 10)
// PRE: ret void
extern "C" void copy_packed(Packed *dest, const Packed *src) { *dest = *src; }

// An odd array stride misaligns the second normalized union word.
struct Array { Odd values[2]; char guard; };
// PRE-LABEL: define {{.*}} @copy_array(
// PRE: call void @zmemmove_builtin({{.*}}i64 19)
// PRE: ret void
extern "C" void copy_array(Array *dest, const Array *src) { *dest = *src; }

// Bitcasts must not erase source/destination types before checking alignment.
struct Bytes { char data[sizeof(Packed)]; };
// PRE-LABEL: define {{.*}} @bitcast_packed(
// PRE: call void @zmemmove_builtin({{.*}}i64 10)
// PRE: ret void
extern "C" void bitcast_packed(Packed *dest, const Bytes *src) {
  *dest = __builtin_bit_cast(Packed, *src);
}

// A nontrivial last field causes the defaulted operation to group earlier
// trivial fields into one copy. Looking only at the first byte field is wrong.
struct __attribute__((packed)) Nontrivial {
  Nontrivial(const Nontrivial &);
  Nontrivial &operator=(const Nontrivial &);
};
struct __attribute__((packed, aligned(16))) Grouped {
  char lead;
  Member member;
  char tail;
  Nontrivial last;
  Grouped &operator=(const Grouped &) = default;
};
// PRE-LABEL: define {{.*}} @copy_grouped(
// PRE: call void @zmemmove_builtin({{.*}}i64 10)
// PRE: call {{.*}} @_ZN10NontrivialaSERKS_
extern "C" void copy_grouped(Grouped *dest, const Grouped *src) { *dest = *src; }

struct __attribute__((packed)) Payload { char prefix[7]; int *pointer; };
union __attribute__((packed)) Shifted { Payload payload; char bytes[15]; };
struct __attribute__((aligned(8))) Wrapper { char tag; Shifted choice; };
extern void take_shifted(Wrapper);
// The byval descriptor is bytewise transport, not typed reads of the synthetic
// word at byte 1. Actual member accesses keep their original storage types.
// PRE-LABEL: define {{.*}} @send_shifted(
// PRE: call void @zmemmove_builtin({{.*}}i64 16)
// PRE: call void @_Z12take_shifted7Wrapper(ptr {{.*}}byval([16 x i8]) align 8
extern "C" void send_shifted(const Wrapper *value) { take_shifted(*value); }

struct Wide { Wrapper shifted; __int128 stamp; };
// PRE-LABEL: define {{.*}} @consume_wide(
// PRE: call ptr @llvm.filc.va.arg.address{{.*}}i64 32, i64 8)
extern "C" long consume_wide(int tag, ...) {
  __builtin_va_list args;
  __builtin_va_start(args, tag);
  Wide value = __builtin_va_arg(args, Wide);
  __builtin_va_end(args);
  return *value.shifted.choice.payload.pointer + (long)value.stamp;
}

// Genuine addresses must keep relocations and capabilities; the address-free
// conversion applies only to null initialization, not arbitrary constants.
int pointee = 42;
union PointerFirst { int *pointer; unsigned long integer; };
PointerFirst relocated = { &pointee };
// POST: @filc_constant_relocations = {{.*}}%filc_constant_relocation { i64 0, i32 0, ptr @pizlonated_pointee }

// A pre-created lifetime-extended global can receive a differently typed null
// initializer. Compilation itself catches the former setInitializer assertion.
struct Dynamic {
  int Owner::*member;
  void *pointer;
  explicit Dynamic(int);
};
extern int runtime_value();
const Dynamic &temporary = Dynamic(runtime_value());
