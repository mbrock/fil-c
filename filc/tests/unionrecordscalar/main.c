#include "value.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int add(int value) { return value + 13; }

int main(void)
{
    int x = 42, y = 7;
    union Scalar bare = { .pointer = &x };
    union Scalar (*volatile indirect_bare)(union Scalar) = scalar;
    bare = indirect_bare(bare);
    assert(*(int*)bare.pointer == 42);
    bare.function = add;
    bare = scalar(bare);
    assert(bare.function(29) == 42);
    struct First a = { .value.pointer = &x, .tag = 17 };
    struct Last b = { .tag = 19, .value.pointer = &y };
    a = first(a);
    b = last(b);
    assert(a.tag == 17 && *(int*)a.value.pointer == 42);
    assert(b.tag == 19 && *(int*)b.value.pointer == 7);
    *(int*)b.value.pointer = 11;
    assert(y == 11);

    struct Array nested = { .values = { { .pointer = &y } }, .tag = 47 };
    nested = array(nested);
    assert(nested.tag == 47 && *(int*)nested.values[0].pointer == 11);

    struct First (*volatile indirect)(struct First) = first;
    a = indirect(a);
    assert(a.tag == 17 && *(int*)a.value.pointer == 42);
    a.value.function = add;
    a = first(a);
    assert(a.value.function(7) == 20);
    b.value.function = add;
    b = last(b);
    assert(b.value.function(29) == 42);

    a.value.integer = -123456789;
    a = first(a);
    assert(a.value.integer == -123456789 && a.tag == 17);
    b.value.number = -2.75;
    b = last(b);
    assert(b.value.number == -2.75 && b.tag == 19);
    // Returning via a pointer-shaped word must preserve bits, not convert the
    // floating-point value numerically (including signed zero and NaN payload).
    unsigned long bits[] = { 0x8000000000000000UL, 0x7ff8000000000042UL };
    for (unsigned i = 0; i < 2; ++i) {
        memcpy(&a.value.number, &bits[i], sizeof(double));
        a = first(a);
        unsigned long got;
        memcpy(&got, &a.value.number, sizeof(double));
        assert(got == bits[i]);
        memcpy(&bare.number, &bits[i], sizeof(double));
        bare = scalar(bare);
        memcpy(&got, &bare.number, sizeof(double));
        assert(got == bits[i]);
    }

    // These unions cannot be admitted just because they contain a pointer.
    struct Hidden h = { .value.pointers = { &x }, .tag = 23 };
    h = hidden(h);
    assert(h.tag == 23 && *h.value.pointers[0] == 42);
    struct Wide w = { .value.pointers = { &x, &y } };
    w = wide(w);
    assert(*w.value.pointers[0] == 42 && *w.value.pointers[1] == 11);
    struct Large l = { .value.pointer = &y };
    l = large(l);
    assert(*(int*)l.value.pointer == 11);
    l.value.integer = ((__int128)12345 << 80) + 6789;
    l = large(l);
    assert(l.value.integer == ((__int128)12345 << 80) + 6789);

    a.value.pointer = &x;
    b.tag = 29;
    struct First c = { .value.integer = -123456789, .tag = 31 };
    struct Last d = { .tag = 37, .value.pointer = &x };
    variadic(&x, a, 12345L, b, c, 1.25, d);
    assert(x == 73);
    puts("union scalar ABI ok");
    return 0;
}
