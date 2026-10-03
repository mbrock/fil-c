#include "unionstoragecopy.h"

#include <string.h>

#define CHECK(condition, case_number) do { \
    if (!(condition)) return (case_number); \
} while (0)

/* Genuine address initializers must retain their relocations/capabilities. */
static int global_value = 42;
static NaturalPointerUnion relocated = { .pointer = &global_value };
static _Thread_local NaturalPointerUnion tls_relocated = {
    .pointer = &global_value
};

static void fill_bytes(OddPointerUnion *value, unsigned char seed)
{
    for (size_t i = 0; i < sizeof(*value); ++i)
        value->bytes[i] = (unsigned char)(seed + i * 13);
}

static int same_bytes(const OddPointerUnion *left,
                      const OddPointerUnion *right)
{
    return memcmp(left->bytes, right->bytes, sizeof(*left)) == 0;
}

int main(void)
{
    /* Aligned container bases still leave packed union members unaligned. */
    _Alignas(16) OffsetOne one_source, one_destination;
    _Alignas(16) OffsetSeven seven_source, seven_destination;
    one_source.value.number = 0x0123456789abcdefULL;
    one_source.value.bytes[8] = 0xd1;
    one_destination.value = one_source.value;
    CHECK(one_destination.value.bytes[8] == 0xd1 &&
          one_destination.value.bytes[0] == 0xef, 1);

    seven_source.value.number = 0xfedcba9876543210ULL;
    seven_source.value.bytes[8] = 0x7b;
    seven_destination.value = seven_source.value;
    CHECK(seven_destination.value.bytes[8] == 0x7b &&
          seven_destination.value.bytes[0] == 0x10, 2);

    /* Arrays expose a 9-byte stride and the byte after each object is a guard. */
    struct __attribute__((packed)) UnionArray {
        OddPointerUnion values[2];
        unsigned char guard;
    } array_source, array_destination;
    fill_bytes(&array_source.values[0], 0x11);
    fill_bytes(&array_source.values[1], 0x42);
    array_source.guard = 0xbc;
    array_destination = array_source;
    CHECK((unsigned char *) &array_destination.values[1] -
          (unsigned char *) &array_destination.values[0] == 9, 3);
    CHECK(same_bytes(&array_source.values[0], &array_destination.values[0]) &&
          same_bytes(&array_source.values[1], &array_destination.values[1]) &&
          array_destination.guard == 0xbc, 4);

    /* Volatile aggregate copy must preserve the ninth byte as well. */
    volatile OddPointerUnion volatile_source, volatile_destination;
    for (size_t i = 0; i < sizeof(OddPointerUnion); ++i)
        volatile_source.bytes[i] = (unsigned char)(0xa0 + i);
    volatile_destination = volatile_source;
    for (size_t i = 0; i < sizeof(OddPointerUnion); ++i)
        CHECK(volatile_destination.bytes[i] == (unsigned char)(0xa0 + i), 5);

    /* Named by-value calls cross a separately compiled translation unit. */
    OddPointerUnion call_value;
    fill_bytes(&call_value, 0x63);
    OddPointerUnion direct = union_round_trip(call_value);
    CHECK(same_bytes(&call_value, &direct), 6);
    OddPointerUnion (*indirect_call)(OddPointerUnion) = union_round_trip;
    OddPointerUnion indirect = indirect_call(call_value);
    CHECK(same_bytes(&call_value, &indirect), 7);

    /* Pointer capabilities must remain shared for a naturally aligned pointer. */
    int shared_value = 41;
    NaturalPointerUnion pointer_source = { .pointer = &shared_value };
    NaturalPointerUnion pointer_copy = pointer_source;
    CHECK(pointer_copy.pointer == &shared_value, 8);
    pointer_copy.pointer = pointer_source.pointer;
    *(int *) pointer_copy.pointer = 73;
    CHECK(shared_value == 73, 9);
    NaturalPointerUnion pointer_return =
        natural_pointer_round_trip(pointer_source);
    *(int *) pointer_return.pointer = 97;
    CHECK(shared_value == 97, 10);

    /* The aggregate is followed by unlike scalar classes to catch va_list drift. */
    OddPointerUnion vararg_value;
    fill_bytes(&vararg_value, 0x20);
    vararg_value.bytes[0] = 0x91;
    vararg_value.bytes[8] = 0xe7;
    CHECK(union_varargs_check(0x1357, vararg_value, 0x2468,
                              0x1020304050607080ULL, 19.75), 11);

    /* Exhausting GPRs must not turn the last aggregate into integer bits. */
    CHECK(exhausted_registers(1, 2, 3, 4, 5, 6, pointer_source), 12);

    /* Check the actual pointer, not just its address bits, in both directions. */
    Wrapper shifted;
    shifted.tag = 0x39;
    shifted.choice.payload.pointer = &shared_value;
    CHECK(shifted_argument(shifted), 13);
    Wrapper shifted_return = shifted_round_trip(shifted);
    CHECK(shifted_return.tag == 0x39 &&
          *shifted_return.choice.payload.pointer == 97, 14);
    CHECK(*(int *)relocated.pointer == 42, 15);
    *(int *)tls_relocated.pointer = 73;
    CHECK(global_value == 73 && *(int *)relocated.pointer == 73, 16);

    /* A named 8-byte scalar exposes disagreement about 8-vs-16 packet offsets. */
    WideWrapper wide = { shifted,
        ((__int128)0x1020304050607080ULL << 64) | 0x9173 };
    CHECK(wide_varargs_check(0x1357, wide, 0x2468, 19.75), 17);

    /* This source really is unaligned, not just declared packed. Stage before
     * calling the packet demoter, whose source must be word-aligned. */
    fill_bytes(&one_source.value, 0x20);
    one_source.value.bytes[0] = 0x91;
    one_source.value.bytes[8] = 0xe7;
    CHECK(union_varargs_check(0x1357, one_source.value, 0x2468,
                             0x1020304050607080ULL, 19.75), 18);
#ifdef __FILC__
    /* GIMSO: this cast lies about alignment. Live source shadow pointers must
     * never be read at fractional offsets by unchecked packet demotion. The
     * forwarder's parameter has declared alignment 8, unlike the case above.
     * Only byte payload is consumed; shifted pointer bits are not dereferenced.
     * Native C makes no guarantee for this deliberately invalid cast. */
    void *live_slots[3] = { &shared_value, &global_value, &shared_value };
    const unsigned char *misaligned = (const unsigned char *)live_slots + 1;
    int expected_tag = misaligned[0];
    CHECK(forwarded_tag((const Wrapper *)misaligned) == expected_tag, 19);
    CHECK(forwarded_varargs_tag((const Wrapper *)misaligned) == expected_tag, 20);
#endif
    return 0;
}
