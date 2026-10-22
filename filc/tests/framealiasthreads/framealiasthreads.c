/* Frame lowers must root pointers while another thread collects: across a pollchecked arithmetic
   loop, across a blocking syscall, and while signal handlers run (both while the mutator is
   blocked and while it is in the loop). The objects are referenced only from the mutator's
   frame. */
#include <pthread.h>
#include <signal.h>
#include <stdfil.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct obj { long tag; long pad[7]; };

static volatile int done;
static volatile long handled;
static int fds[2];
static pthread_t mutator;

static void handler(int sig)
{
    (void)sig;
    struct obj* q = malloc(sizeof(struct obj));
    q->tag = 7;
    zgc_request_and_wait();
    handled += q->tag;
}

static void* collector(void* arg)
{
    (void)arg;
    int rounds = 0;
    while (!done) {
        for (int i = 0; i < 1000; i++) {
            struct obj* q = malloc(sizeof(struct obj));
            q->tag = -1;
        }
        zgc_request_and_wait();
        if (!(++rounds % 4))
            pthread_kill(mutator, SIGUSR1);
    }
    return NULL;
}

static __attribute__((noinline)) long spin(long n)
{
    struct obj* p = malloc(sizeof(struct obj));
    struct obj* r = malloc(sizeof(struct obj));
    p->tag = 3;
    r->tag = 5;
    zweak* w = zweak_new(p);
    long s = 0;
    for (long i = 0; i < n; i++) {
        s += p->tag * (i & 7) + r->tag;
        p->tag ^= 1;
    }
    ZASSERT(zweak_get(w) == p);
    return s + p->tag + r->tag;
}

static __attribute__((noinline)) long blocked(void)
{
    struct obj* p = malloc(sizeof(struct obj));
    p->tag = 11;
    zweak* w = zweak_new(p);
    char c;
    ZASSERT(read(fds[0], &c, 1) == 1);
    ZASSERT(zweak_get(w) == p);
    return p->tag + c;
}

static void* writer(void* arg)
{
    (void)arg;
    for (int i = 0; i < 20; i++) {
        zgc_request_and_wait();
        pthread_kill(mutator, SIGUSR1);
        usleep(1000);
    }
    char c = 1;
    ZASSERT(write(fds[1], &c, 1) == 1);
    return NULL;
}

int main(void)
{
    struct sigaction sa = { 0 };
    sa.sa_handler = handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);
    mutator = pthread_self();
    ZASSERT(!pipe(fds));

    pthread_t t;
    pthread_create(&t, NULL, collector, NULL);
    long s = 0;
    for (int k = 0; k < 20; k++)
        s += spin(2000000);
    done = 1;
    pthread_join(t, NULL);

    pthread_create(&t, NULL, writer, NULL);
    long b = blocked();
    pthread_join(t, NULL);

    printf("spin = %ld, blocked = %ld, handled = %s\n", s, b, handled ? "yes" : "no");
    ZASSERT(b == 12);
    printf("ok\n");
    return 0;
}
