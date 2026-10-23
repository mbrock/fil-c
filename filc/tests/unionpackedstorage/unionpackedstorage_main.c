#include "unionpackedstorage.h"
#include <stdfil.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(struct Indirect) == 39, "packed indirect storage");
_Static_assert(__alignof__(union Odd) == 1, "underaligned union");

struct Entry { int *key; };

/* Initializers whose pointers are word-aligned from the object's start. */
static int global_value = 42;
struct Base base_global = {
    .ops = &global_value, .indirect = { .storage = &global_value, .entries = 3 } };
_Thread_local struct Base base_tls = {
    .ops = &global_value, .indirect = { .storage = &global_value, .entries = 4 } };
union Odd odd_global = { .pointer = &global_value };
_Thread_local union Odd odd_tls = { .pointer = &global_value };
/* A null pointer at byte 1 needs no relocation and stays allowed. */
struct __attribute__((packed)) NullAtOne { char prefix; int *pointer; };
struct NullAtOne null_at_one = { 7, 0 };

int main(void)
{
    int target = 97;
    struct Base *base = calloc(1, sizeof(*base));
    struct Entry *entries = (struct Entry *)base->direct.storage;
    entries[0].key = &target;
    entries[3].key = &target;
    struct Base copy;
    copy_base(&copy, base);
    entries = (struct Entry *)copy.direct.storage;
    ZASSERT(*entries[0].key == 97 && *entries[3].key == 97);

    base->has_indirect = 1;
    base->indirect.storage = &target;
    base->indirect.entries = 5;
    copy_base(&copy, base);
    ZASSERT(*(int *)copy.indirect.storage == 97);
    ZASSERT(copy.indirect.entries == 5 && copy.has_indirect);

    /* Aligned instances of an underaligned union travel by value. */
    union Odd odd = { .pointer = &target };
    union Odd back = round_trip(odd);
    ZASSERT(*back.pointer == 97);
    ZASSERT(check_varargs(17, odd, 0x123456789L));

    ZASSERT(*(int *)base_global.indirect.storage == 42 && base_global.indirect.entries == 3);
    ZASSERT(*(int *)base_tls.indirect.storage == 42 && base_tls.indirect.entries == 4);
    ZASSERT(*odd_global.pointer == 42 && *odd_tls.pointer == 42);
    ZASSERT(null_at_one.prefix == 7);
    return 0;
}
