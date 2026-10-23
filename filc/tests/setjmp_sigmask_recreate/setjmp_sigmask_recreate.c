#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pizlonated_runtime.h>

/* glibc's macro otherwise redirects plain setjmp to _setjmp. */
#undef setjmp

static sigjmp_buf buf;
static sigset_t mask;
static int save;
static int iteration;
static int failed;

static void block(int blocked)
{
    if (sigprocmask(blocked ? SIG_BLOCK : SIG_UNBLOCK, &mask, NULL))
        abort();
}

static void check(const char* kind, int first)
{
    sigset_t current;
    if (sigprocmask(SIG_BLOCK, NULL, &current))
        abort();
    int blocked = sigismember(&current, SIGUSR1);
    printf("%s %d->%d: %d\n", kind, first, !first, blocked);
    if (blocked != !first)
        failed = 1;
}

static void test_sigsetjmp(int first)
{
    save = first;
    iteration = 0;
    block(1);
    for (;;) {
        if (sigsetjmp(buf, save))
            break;
        if (!iteration++) {
            save = !first;
            /* The second save sees a different mask from the first. */
            block(!first);
        } else {
            block(0);
            siglongjmp(buf, 1);
        }
    }
    check("sigsetjmp", first);
}

static void test_setjmp(int first)
{
    iteration = 0;
    block(1);
    zmake_setjmp_save_sigmask(first);
    for (;;) {
        if (setjmp(buf))
            break;
        if (!iteration++) {
            zmake_setjmp_save_sigmask(!first);
            block(!first);
        } else {
            block(0);
            longjmp(buf, 1);
        }
    }
    check("setjmp", first);
}

int main(void)
{
    sigset_t original;
    if (sigemptyset(&mask) || sigaddset(&mask, SIGUSR1) ||
        sigprocmask(SIG_BLOCK, NULL, &original))
        abort();
    test_sigsetjmp(1);
    test_sigsetjmp(0);
    test_setjmp(1);
    test_setjmp(0);
    if (sigprocmask(SIG_SETMASK, &original, NULL))
        abort();
    return failed;
}
