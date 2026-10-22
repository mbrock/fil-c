/* A frame's address reaches user code without a capability, also through a phi and an integer
   round trip, so stores through it must still trap. */
#include <stdfil.h>
#include <stdint.h>
#include <stdlib.h>

static __attribute__((noinline)) char* pick(int which, char* heap)
{
    char* p;
    if (which)
        p = (char*)__builtin_frame_address(0);
    else
        p = heap;
    uintptr_t i = (uintptr_t)p;
    return (char*)(i + 0);
}

int main(int argc, char** argv)
{
    (void)argv;
    char* heap = malloc(16);
    char* p = pick(0, heap);
    *p = 1;
    p = pick(argc, heap);
    ZASSERT(!zhasvalidcap(p));
    zprintf("Got here.\n");
    *p = 1;
    zprintf("Should not get here.\n");
    return 0;
}
