/* FilPizlonator tags its frame stores with an alias scope that ordinary loads and stores are
   noalias with. Calls must still see every lower the frame needs: here the only references to
   the objects are locals whose lowers sit in frame slots, read by a collection inside an opaque
   call. Losing or sinking any of those stores past the call frees an object that is used
   afterwards. Covers straight-line code, a loop, inlined helpers, and many adjacent slots. */
#include <stdfil.h>
#include <stdio.h>
#include <stdlib.h>

struct obj { long tag; struct obj* next; long pad[6]; };

static __attribute__((noinline)) void observe(void)
{
    zgc_request_and_wait();
    for (int i = 0; i < 1000; i++) {
        struct obj* q = malloc(sizeof(struct obj));
        q->tag = -1;
    }
}

static inline struct obj* make(long tag)
{
    struct obj* p = malloc(sizeof(struct obj));
    p->tag = tag;
    p->next = NULL;
    return p;
}

static inline long touch(struct obj* p, long k)
{
    p->tag += k;
    return p->tag;
}

static __attribute__((noinline)) long straight(void)
{
    struct obj* a = make(1);
    struct obj* b = make(2);
    struct obj* c = make(3);
    struct obj* d = make(4);
    struct obj* e = make(5);
    struct obj* f = make(6);
    struct obj* g = make(7);
    struct obj* h = make(8);
    zweak* w = zweak_new(h);
    long s = touch(a, 1) + touch(b, 1);
    observe();
    s += touch(c, 1) + touch(d, 1);
    observe();
    s += touch(e, 1) + touch(f, 1) + touch(g, 1);
    observe();
    ZASSERT(zweak_get(w) == h);
    s += touch(h, 1) + a->tag + b->tag;
    return s;
}

static __attribute__((noinline)) long looped(long n)
{
    long s = 0;
    struct obj* keep = make(100);
    for (long i = 0; i < n; i++) {
        struct obj* p = make(i);
        p->next = (i & 1) ? keep : NULL;
        s += touch(p, 1);
        observe();
        s += p->tag + (p->next ? p->next->tag : 0);
        struct obj* r = make(i * 2);
        observe();
        s += touch(r, 3) + touch(keep, 0);
    }
    return s;
}

int main(void)
{
    long s1 = straight();
    long s2 = looped(50);
    printf("straight = %ld, looped = %ld\n", s1, s2);
    ZASSERT(s1 == (2 + 3) + (4 + 5) + (6 + 7 + 8) + 9 + 2 + 3);
    long expect = 0;
    for (long i = 0; i < 50; i++)
        expect += (i + 1) + (i + 1) + ((i & 1) ? 100 : 0) + (i * 2 + 3) + 100;
    ZASSERT(s2 == expect);
    printf("ok\n");
    return 0;
}
