/* FilPizlonator roots a pointer derived from another one (a GEP, a cast, or a phi or select whose
   inputs all derive from it) through the frame slot of the pointer it derives from. Each case
   below keeps an object reachable only through such a derived pointer, or through a phi that
   must keep its own slot, across collections, and checks it stays alive. */
#include <setjmp.h>
#include <stdfil.h>
#include <stdio.h>
#include <stdlib.h>

struct obj { long tag; struct obj* next; long pad[6]; };

static __attribute__((noinline)) void collect(void)
{
    zgc_request_and_wait();
    for (int i = 0; i < 2000; i++) {
        struct obj* q = malloc(sizeof(struct obj));
        q->tag = -1;
    }
    zgc_request_and_wait();
}

static __attribute__((noinline)) struct obj* fresh(long tag)
{
    struct obj* p = malloc(sizeof(struct obj));
    p->tag = tag;
    p->next = NULL;
    return p;
}

/* Only an interior pointer survives the base pointer's last use. Each case allocates another
   object after that use, whose pointer would take the base's frame slot if the base's liveness
   did not extend over its derived pointers. */
static __attribute__((noinline)) long interior(void)
{
    long* arr = malloc(64 * sizeof(long));
    zweak* w = zweak_new(arr);
    for (int i = 0; i < 64; i++)
        arr[i] = i;
    long* mid = arr + 40;
    /* Gives arr's frame slot, if arr were dead, to another pointer. */
    struct obj* o = fresh(7);
    collect();
    ZASSERT(zweak_get(w));
    return *mid + mid[10] + o->tag - 7;
}

/* A pointer walking an array in a loop, with collections inside the loop. Nothing in the loop
   uses arr itself. */
static __attribute__((noinline)) long walk(long n)
{
    long* arr = malloc(n * sizeof(long));
    zweak* w = zweak_new(arr);
    long* end = arr + n;
    long s = 0;
    long k = 0;
    for (long* p = arr; p != end; p++, k++) {
        *p = k;
        if (!(k % 16)) {
            struct obj* o = fresh(k);
            collect();
            s += o->tag - k;
        }
        s += *p;
    }
    ZASSERT(zweak_get(w));
    return s;
}

/* prev takes cur's value from the previous iteration. Its object is referenced by nothing
   else, so prev needs its own slot: its lower is not cur's. */
static __attribute__((noinline)) long rotate(int n)
{
    struct obj* prev = fresh(0);
    struct obj* cur = fresh(1);
    long s = 0;
    for (int i = 2; i < n; i++) {
        struct obj* old = prev;
        prev = cur;
        cur = fresh(i);
        zweak* w = zweak_new(prev);
        (void)old;
        collect();
        ZASSERT(zweak_get(w) == prev);
        s += prev->tag;
    }
    return s + prev->tag + cur->tag;
}

/* A select between two pointers into the same object, and between two objects. */
static __attribute__((noinline)) long selects(int flip)
{
    long* a = malloc(8 * sizeof(long));
    long* b = malloc(8 * sizeof(long));
    for (int i = 0; i < 8; i++) {
        a[i] = i;
        b[i] = 100 + i;
    }
    zweak* wa = zweak_new(a);
    zweak* wb = zweak_new(b);
    long* same = flip ? a + 1 : a + 5;
    long* other = flip ? a : b;
    collect();
    ZASSERT(zweak_get(wa));
    ZASSERT(zweak_get(wb));
    return *same + other[3];
}

/* An interior pointer whose only later use is through a phi, reached from a predecessor that
   allocates and collects. */
static __attribute__((noinline)) long phiedge(int which)
{
    long* arr = malloc(16 * sizeof(long));
    zweak* w = zweak_new(arr);
    for (int i = 0; i < 16; i++)
        arr[i] = i;
    long* p;
    if (which) {
        p = arr + 3;
        struct obj* o = fresh(1);
        collect();
        p += o->tag;
    } else {
        p = arr + 9;
        collect();
    }
    ZASSERT(zweak_get(w));
    return *p;
}

/* An interior pointer made before setjmp and used only after longjmp returns to it. */
static jmp_buf jb;
static __attribute__((noinline)) void jump(void)
{
    collect();
    longjmp(jb, 1);
}
static __attribute__((noinline)) long acrossjmp(void)
{
    long* arr = malloc(16 * sizeof(long));
    zweak* w = zweak_new(arr);
    for (int i = 0; i < 16; i++)
        arr[i] = 2 * i;
    long* volatile mid = arr + 5;
    if (!setjmp(jb))
        jump();
    collect();
    ZASSERT(zweak_get(w));
    return *mid;
}

int main(int argc, char** argv)
{
    (void)argv;
    long i = interior();
    long w = walk(200);
    long r = rotate(20);
    long s = selects(argc);
    ZASSERT(phiedge(argc) == 4);
    ZASSERT(phiedge(0) == 9);
    ZASSERT(acrossjmp() == 10);
    printf("%ld %ld %ld %ld\n", i, w, r, s);
    ZASSERT(i == 90);
    ZASSERT(w == 199 * 200 / 2);
    ZASSERT(r == (2 + 18) * 17 / 2 + 18 + 19 + 1);
    ZASSERT(s == 1 + 3);
    printf("ok\n");
    return 0;
}
