/*
 * 02_recursion_depth.c
 * ---------------------
 * Find out how deep you can recurse before the stack overflows.  We catch
 * SIGSEGV on an alternate stack so we can print results before dying.
 *
 *   Typical: ~80,000 frames for an empty function (8 MiB / ~100 B per frame).
 *
 * Build:  gcc -O0 -g 02_recursion_depth.c -o 02_recursion_depth
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

static jmp_buf jb;
static long depth_reached;

static void on_segv(int sig, siginfo_t *si, void *ctx)
{
    (void)sig; (void)si; (void)ctx;
    /* Note: long-jumping out of a signal handler is technically UB but
     * works on Linux for SIGSEGV stack-overflow scenarios when run on an
     * alternate stack. */
    siglongjmp(jb, 1);
}

static long counter;
static void recurse(void)
{
    char filler[64];  /* make the frame non-trivial */
    filler[0] = 0;
    counter++;
    recurse();
    (void)filler;
}

int main(void)
{
    /* alternate signal stack so the handler runs even with a smashed stack */
    static char altstack[SIGSTKSZ];
    stack_t ss = { .ss_sp = altstack, .ss_size = sizeof altstack };
    sigaltstack(&ss, NULL);

    struct sigaction sa = {0};
    sa.sa_sigaction = on_segv;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);

    if (sigsetjmp(jb, 1) == 0) {
        recurse();
    } else {
        depth_reached = counter;
        printf("SIGSEGV caught at depth %ld\n", depth_reached);
        printf("each frame ~= %ld bytes\n",
               (long)(8 * 1024 * 1024) / (depth_reached ? depth_reached : 1));
    }
    return 0;
}
