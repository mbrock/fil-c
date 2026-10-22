/* A loop with no calls caches the object's upper bound and flags across fast pollchecks. A signal
   handler that frees the object runs inside a slow pollcheck, so the loop must reload them there
   and fail on its next access instead of reading the freed object forever. */
#include <signal.h>
#include <stdfil.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static long* array;

static void handler(int sig)
{
    (void)sig;
    free(array);
}

static __attribute__((noinline)) long spin(long* p)
{
    long s = 0;
    for (unsigned long i = 0;; i++) {
        long v = p[i & 15];
        if (v < 0)
            return s;
        s += v;
    }
}

int main(void)
{
    array = malloc(16 * sizeof(long));
    for (int i = 0; i < 16; i++)
        array[i] = i;
    signal(SIGALRM, handler);
    struct itimerval it = { { 0, 0 }, { 0, 100000 } };
    setitimer(ITIMER_REAL, &it, NULL);
    printf("Got here.\n");
    fflush(stdout);
    printf("%ld\n", spin(array));
    printf("Should not get here.\n");
    return 0;
}
