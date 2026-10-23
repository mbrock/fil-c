#include "unionpackedstorage.h"
#include <stdarg.h>

void copy_base(struct Base *out, const struct Base *in)
{
    struct Base temporary = *in;
    *out = temporary;
}

union Odd round_trip(union Odd value) { return value; }

int check_varargs(int tag, ...)
{
    va_list args;
    va_start(args, tag);
    union Odd value = va_arg(args, union Odd);
    long after = va_arg(args, long);
    va_end(args);
    return tag == 17 && *value.pointer == 97 && after == 0x123456789L;
}
