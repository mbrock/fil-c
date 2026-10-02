// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o - %s | FileCheck %s

// A scalar union occupying one pointer word keeps its capability-bearing direct
// signature. In particular, it must not acquire a byval argument, which would
// force a generic Fil-C call and heap promotion even for this tiny record.
union Scalar { long integer; double number; void *pointer; };
struct First { union Scalar value; long tag; };
struct Last { long tag; union Scalar value; };

// CHECK-LABEL: define {{.*}} @pizlonatedFIP{{[1-9][0-9]*}}_first(
// CHECK-NOT: @filc_promote_already_checked_stack_to_heap_without_exiting(
// CHECK: ret
struct First first(struct First value) { return value; }

// CHECK-LABEL: define {{.*}} @pizlonatedFIP{{[1-9][0-9]*}}_last(
// CHECK-NOT: @filc_promote_already_checked_stack_to_heap_without_exiting(
// CHECK: ret
struct Last last(struct Last value) { return value; }

// A pointer in an aggregate alternative may not be represented as a pointer in
// the union's LLVM storage. Keep these cases on the conservative memory path.
struct Hidden { union { long integer; void *pointers[1]; } value; long tag; };
struct Wide { union { void *pointer; void *pointers[2]; } value; };
struct Large { union { void *pointer; __int128 integer; } value; };

// CHECK-LABEL: define {{.*}} @pizlonatedFIP0_hidden(
// CHECK: call {{.*}} @filc_promote_already_checked_stack_to_heap_without_exiting(
struct Hidden hidden(struct Hidden value) { return value; }

// CHECK-LABEL: define {{.*}} @pizlonatedFIP0_wide(
// CHECK: call {{.*}} @filc_promote_already_checked_stack_to_heap_without_exiting(
struct Wide wide(struct Wide value) { return value; }

// CHECK-LABEL: define {{.*}} @pizlonatedFIP0_large(
// CHECK: call {{.*}} @filc_promote_already_checked_stack_to_heap_without_exiting(
struct Large large(struct Large value) { return value; }
