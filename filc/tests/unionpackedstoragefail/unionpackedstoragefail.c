/* An underaligned union compiles when its pointers are word-aligned relative
 * to its start. The second element of a packed array with a 9-byte stride is
 * actually misaligned: copying it by value moves only bytes, so a pointer
 * placed there must not arrive with a usable capability. */
#include <stdio.h>
#include <string.h>

union __attribute__((packed)) Odd { int *pointer; char bytes[9]; };

__attribute__((noinline)) int take(union Odd value)
{
    return *value.pointer;
}

union Odd odds[2];

int main(void)
{
    int target = 42;
    int *pointer = &target;
    memcpy(&odds[1], &pointer, sizeof(pointer));
    int result = take(odds[1]);
    printf("nono %d\n", result);
    return 0;
}
