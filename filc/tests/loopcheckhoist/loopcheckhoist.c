/* Loop access checks load the object's upper bound and flags once before the loop. That load must
   not happen for a null capability, even when the loop runs zero times or never reaches the
   access. */
#include <stdfil.h>
#include <stdio.h>
#include <stdlib.h>

static __attribute__((noinline)) long sum(const long* p, int n)
{
    long s = 0;
    for (int i = 0; i < n; i++)
        s += p[i];
    return s;
}

static __attribute__((noinline)) long sumif(const long* p, int n, int k)
{
    long s = 0;
    for (int i = 0; i < n; i++) {
        if (i >= k)
            s += p[i];
        else
            s += i;
    }
    return s;
}

static __attribute__((noinline)) void clear(int* p, unsigned first, unsigned last)
{
    for (; first != last; first++)
        p[first] = 0;
}

int main(int argc, char** argv)
{
    (void)argv;
    long* a = malloc(16 * sizeof(long));
    for (int i = 0; i < 16; i++)
        a[i] = i;
    ZASSERT(sum(NULL, 0) == 0);
    ZASSERT(sum(a, 16) == 120);
    ZASSERT(sumif(NULL, 3, 5) == 3);
    ZASSERT(sumif(a, 16, 10) == 45 + 10 + 11 + 12 + 13 + 14 + 15);
    int* b = malloc(8 * sizeof(int));
    for (int i = 0; i < 8; i++)
        b[i] = 1;
    clear(b, 2, 6);
    ZASSERT(b[1] == 1 && b[2] == 0 && b[5] == 0 && b[6] == 1);
    clear(NULL, argc, argc);
    printf("ok\n");
    return 0;
}
