/* Frame lowers across nonlocal control flow, with collections in between: setjmp in a loop with
   reused buffers and a collection after every longjmp, a musttail that forwards its own pointer
   argument (kept as a tail call) and one that passes a new pointer (demoted to a call), and a
   destructor that collects while an exception propagates through a frame holding the only
   reference to an object. */
#include <setjmp.h>
#include <stdfil.h>
#include <stdio.h>
#include <stdlib.h>

struct obj { long tag; long pad[7]; };

static void churn()
{
    for (int i = 0; i < 1000; i++) {
        obj* q = (obj*)malloc(sizeof(obj));
        q->tag = -1;
    }
    zgc_request_and_wait();
}

static jmp_buf bufs[4];

static __attribute__((noinline)) void jump(int i, int v) { longjmp(bufs[i], v); }

static __attribute__((noinline)) long setjmps()
{
    long s = 0;
    for (int round = 0; round < 200; round++) {
        obj* p = (obj*)malloc(sizeof(obj));
        p->tag = round;
        int i = round & 3;
        int v = setjmp(bufs[i]);
        if (!v)
            jump(i, round + 1);
        churn();
        ZASSERT(v == round + 1);
        s += p->tag;
    }
    return s;
}

static __attribute__((noinline)) long sink(obj* p)
{
    churn();
    return p->tag;
}

static __attribute__((noinline)) long forwarded(obj* p)
{
    p->tag += 1;
    [[clang::musttail]] return sink(p);
}

static __attribute__((noinline)) long demoted(obj* unused)
{
    (void)unused;
    obj* p = (obj*)malloc(sizeof(obj));
    p->tag = 40;
    [[clang::musttail]] return sink(p);
}

struct collect_on_unwind {
    ~collect_on_unwind() { churn(); }
};

static __attribute__((noinline)) void thrower(long v)
{
    collect_on_unwind c;
    throw v;
}

static __attribute__((noinline)) long through(zweak** w)
{
    obj* p = (obj*)malloc(sizeof(obj));
    p->tag = 9;
    *w = zweak_new(p);
    try {
        thrower(p->tag);
    } catch (long v) {
        ZASSERT(zweak_get(*w) == p);
        return v + p->tag;
    }
    return 0;
}

int main()
{
    long s = setjmps();
    ZASSERT(s == 199 * 200 / 2);
    obj* p = (obj*)malloc(sizeof(obj));
    p->tag = 1;
    ZASSERT(forwarded(p) == 2);
    ZASSERT(demoted(nullptr) == 40);
    zweak* w;
    ZASSERT(through(&w) == 18);
    printf("ok\n");
    return 0;
}
