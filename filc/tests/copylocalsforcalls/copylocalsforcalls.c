/* Locals whose address only reaches nocapture call arguments are passed to the call as a heap copy
   and copied back afterwards, so they can stay on the stack; constant-size memsets into locals
   become stores. Check that values flow both ways, that one call sees one copy, and that a callee
   which captures despite claiming noescape is still memory-safe. */
#include <stdfil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __attribute__((noinline)) void bump(long* p, long* q) { *p += 1; *q += 10; }
static __attribute__((noinline)) int same(long* a, long* b) { return a == b; }
static __attribute__((noinline)) void putptr(char** slot) { *slot = strdup("hello"); }
static __attribute__((noinline)) void setsecond(long* p) { p[1] = 7; }

static __attribute__((noinline)) long fastslow(int slow)
{
    long x = 3;
    long y = 4;
    if (slow)
        bump(&x, &y);
    x *= 2;
    return x * 100 + y;
}

static __attribute__((noinline)) long twoargs(void)
{
    long x[2] = { 1, 2 };
    long r = same(&x[0], x) * 10;
    setsecond(x);
    return r + x[1];
}

static __attribute__((noinline)) char* pointers(void)
{
    char* s = NULL;
    putptr(&s);
    zgc_request_and_wait();
    return s;
}

static void* stash;
__attribute__((noinline)) void liar(__attribute__((noescape)) long* p) { stash = p; }

static __attribute__((noinline)) long lying(void)
{
    long x = 5;
    liar(&x);
    x += 1;
    return x;
}

static __attribute__((noinline)) unsigned spn(const char* s, const char* accept, int fill)
{
    unsigned char table[256];
    unsigned char odd[13];
    memset(table, 0, sizeof(table));
    memset(odd, fill, sizeof(odd));
    for (const char* a = accept; *a; a++)
        table[(unsigned char)*a] = 1;
    unsigned n = 0;
    while (table[(unsigned char)s[n]])
        n++;
    unsigned sum = 0;
    for (unsigned i = 0; i < sizeof(odd); i++)
        sum += odd[i];
    return n * 10000 + sum;
}

struct withptr { char* p; long n; };
static __attribute__((noinline)) void fillptr(struct withptr* w) { w->p = strdup("x"); w->n = 1; }

/* A memset over a slot that held a pointer must clear the capability, not just the bits. */
static __attribute__((noinline)) int clearedptr(void)
{
    struct withptr w;
    fillptr(&w);
    ZASSERT(w.p[0] == 'x');
    memset(&w, 0, sizeof(w));
    return w.p == NULL && !zhasvalidcap(w.p);
}

int main(int argc, char** argv)
{
    (void)argv;
    ZASSERT(fastslow(0) == 604);
    ZASSERT(fastslow(argc) == 814);
    ZASSERT(twoargs() == 17);
    char* s = pointers();
    ZASSERT(!strcmp(s, "hello"));
    ZASSERT(lying() == 6);
    zgc_request_and_wait();
    /* Which value the stashed pointer sees depends on whether the call got a copy; either way the
       access must be in bounds and the object must be alive. */
    long v = *(long*)stash;
    ZASSERT(v == 5 || v == 6);
    *(long*)stash = 9;
    ZASSERT(spn("abcabcx", "abc", argc + 0xaa) == 6 * 10000 + 13 * 0xab);
    ZASSERT(clearedptr());
    printf("ok\n");
    return 0;
}
