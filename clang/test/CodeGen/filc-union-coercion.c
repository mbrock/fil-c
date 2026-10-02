// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -mllvm -filc-light-verbose -o %t %s 2>&1 | FileCheck %s --check-prefix=PRE
// RUN: FileCheck %s --check-prefix=POST < %t

union Hidden { long integer; void *pointers[1]; };
struct __attribute__((packed)) Packed { char prefix[7]; union Hidden value; };
extern union Hidden take(union Hidden value);

// The initial verbose module is immediately before FilPizlonator, after early
// optimization. Staging must remain an opaque copy, followed by an aligned
// pointer load, rather than an integer load/inttoptr or unaligned pointer load.
// PRE-LABEL: define {{.*}} @read_field(
// PRE: call void @zmemmove_builtin(
// PRE: load ptr, ptr {{.*}}, align 8
// PRE: call ptr @take(
union Hidden read_field(struct Packed *p) { return take(p->value); }

// PRE-LABEL: define {{.*}} @read_volatile(
// PRE: call void @zmemmove_builtin_volatile(
// PRE: load ptr, ptr {{.*}}, align 8
// PRE: call ptr @take(
union Hidden read_volatile(volatile struct Packed *p) { return take(p->value); }

// POST-LABEL: define {{.*}} @pizlonatedFIP{{[1-9][0-9]*}}_read_volatile(
// POST-NOT: call {{.*}} @zmemmove_builtin_volatile(
// POST: call void @filc_memmove(
// POST-NOT: call {{.*}} @zmemmove_builtin_volatile(
