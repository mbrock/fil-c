#include "unionstoragecopy.h"

#include <stdarg.h>

OddPointerUnion union_round_trip(OddPointerUnion value)
{
    return value;
}

NaturalPointerUnion natural_pointer_round_trip(NaturalPointerUnion value)
{
    return value;
}

int exhausted_registers(long a, long b, long c, long d, long e, long f,
                        NaturalPointerUnion value)
{
    return a == 1 && b == 2 && c == 3 && d == 4 && e == 5 && f == 6 &&
        *(int *)value.pointer == 97;
}

Wrapper shifted_round_trip(Wrapper value)
{
    return value;
}

int shifted_argument(Wrapper value)
{
    return value.tag == 0x39 && *value.choice.payload.pointer == 97;
}

__attribute__((noinline)) int consume_tag(Wrapper value)
{
    return value.tag;
}

int forwarded_tag(const Wrapper *value)
{
    return consume_tag(*value);
}

__attribute__((noinline)) int consume_varargs_tag(int tag, ...)
{
    va_list args;
    va_start(args, tag);
    Wrapper value = va_arg(args, Wrapper);
    va_end(args);
    return value.tag;
}

int forwarded_varargs_tag(const Wrapper *value)
{
    return consume_varargs_tag(0, *value);
}

int union_varargs_check(int tag, ...)
{
    va_list args;
    va_start(args, tag);
    OddPointerUnion value = va_arg(args, OddPointerUnion);
    int first = va_arg(args, int);
    unsigned long long second = va_arg(args, unsigned long long);
    double third = va_arg(args, double);
    va_end(args);

    return tag == 0x1357 && value.bytes[0] == 0x91 &&
        value.bytes[8] == 0xe7 && first == 0x2468 &&
        second == 0x1020304050607080ULL && third == 19.75;
}

int wide_varargs_check(int tag, ...)
{
    va_list args;
    va_start(args, tag);
    WideWrapper value = va_arg(args, WideWrapper);
    int first = va_arg(args, int);
    double second = va_arg(args, double);
    va_end(args);
    return tag == 0x1357 && shifted_argument(value.shifted) &&
        value.stamp == (((__int128)0x1020304050607080ULL << 64) | 0x9173) &&
        first == 0x2468 && second == 19.75;
}
