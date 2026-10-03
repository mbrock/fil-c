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

struct __attribute__((packed)) Payload { char prefix[7]; int *pointer; };
union __attribute__((packed)) Shifted { struct Payload payload; char bytes[15]; };
struct __attribute__((aligned(8))) Wrapper { char tag; union Shifted choice; };
extern int take_shifted(struct Wrapper);
extern int take_unnamed(int tag, ...);

// Even a naturally aligned C pointer parameter can be supplied by a bad cast.
// Byte descriptors remove typed alignment checks, so both named and unnamed
// calls must stage through our own aligned object before packet demotion.
// PRE-LABEL: define {{.*}} @forward_shifted(
// PRE: %[[NAMED:[^ ]+]] = alloca %struct.Wrapper, align 8
// PRE: call void @zmemmove_builtin(ptr {{.*}}%[[NAMED]], ptr %value, i64 16)
// PRE: call i32 @take_shifted(ptr {{.*}}byval([16 x i8]) align 8 %[[NAMED]])
int forward_shifted(const struct Wrapper *value) { return take_shifted(*value); }

// PRE-LABEL: define {{.*}} @forward_unnamed(
// PRE: %[[UNNAMED:[^ ]+]] = alloca %struct.Wrapper, align 8
// PRE: call void @zmemmove_builtin(ptr {{.*}}%[[UNNAMED]], ptr %value, i64 16)
// PRE: call i32 (i32, ...) @take_unnamed(i32 0, ptr {{.*}}byval([16 x i8]) align 8 %[[UNNAMED]])
int forward_unnamed(const struct Wrapper *value) { return take_unnamed(0, *value); }
