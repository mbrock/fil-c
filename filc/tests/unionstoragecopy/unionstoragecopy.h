#ifndef UNIONSTORAGECOPY_H
#define UNIONSTORAGECOPY_H

#include <stddef.h>
#include <stdint.h>

/* Pointer-capable, but deliberately odd-sized: arrays advance by 9 bytes. */
typedef union __attribute__((packed)) OddPointerUnion {
    uint64_t number;
    void *pointer;
    unsigned char bytes[9];
} OddPointerUnion;

/* Real pointer accesses use this naturally aligned pointer-capable union. */
typedef union NaturalPointerUnion {
    void *pointer;
    uint64_t number;
} NaturalPointerUnion;

/* The actual pointer is aligned in Wrapper, but the union's synthetic words
 * begin at byte 1. Register coercion must not read those shifted words. */
typedef struct __attribute__((packed)) ShiftedPayload {
    unsigned char prefix[7];
    int *pointer;
} ShiftedPayload;
typedef union __attribute__((packed)) ShiftedChoice {
    ShiftedPayload payload;
    unsigned char bytes[15];
} ShiftedChoice;
typedef struct __attribute__((aligned(8))) Wrapper {
    unsigned char tag;
    ShiftedChoice choice;
} Wrapper;
_Static_assert(offsetof(Wrapper, choice.payload.pointer) == 8,
               "actual pointer must be aligned");
_Static_assert(sizeof(Wrapper) == 16, "small aggregate ABI boundary");

/* LLVM storage has 16-byte alignment here. Its byte transport descriptor has
 * alignment 1, so va_arg must use the descriptor's packet alignment too. */
typedef struct WideWrapper {
    Wrapper shifted;
    __int128 stamp;
} WideWrapper;

/* These packed containers put the union at internal offsets 1 and 7. */
typedef struct __attribute__((packed)) OffsetOne {
    unsigned char prefix;
    OddPointerUnion value;
} OffsetOne;

typedef struct __attribute__((packed)) OffsetSeven {
    unsigned char prefix[7];
    OddPointerUnion value;
} OffsetSeven;

_Static_assert(sizeof(OddPointerUnion) == 9, "odd union storage size");
_Static_assert(offsetof(OffsetOne, value) == 1, "offset-one container");
_Static_assert(offsetof(OffsetSeven, value) == 7, "offset-seven container");

OddPointerUnion union_round_trip(OddPointerUnion value);
NaturalPointerUnion natural_pointer_round_trip(NaturalPointerUnion value);
int exhausted_registers(long, long, long, long, long, long,
                       NaturalPointerUnion value);
Wrapper shifted_round_trip(Wrapper value);
int shifted_argument(Wrapper value);
int forwarded_tag(const Wrapper *value);
int forwarded_varargs_tag(const Wrapper *value);
int union_varargs_check(int tag, ...);
int wide_varargs_check(int tag, ...);

#endif
