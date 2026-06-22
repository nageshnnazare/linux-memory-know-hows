/*
 * 04_stack_overflow.c
 * --------------------
 * Overflow the stack on purpose, but catch the SIGSEGV on an alternate
 * stack so we don't die silently.  Compare:
 *
 *   $ ulimit -s          # current limit, KiB
 *   8192
 *   $ ulimit -s 1024     # shrink the limit
 *   $ ./04_stack_overflow
 *
 * Build:  gcc -O0 -g 04_stack_overflow.c -o 04_stack_overflow
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>
#include <setjmp.h>

static jmp_buf jb;

static void h(int s, siginfo_t *si, void *uc)
{
    (void)s; (void)uc;
    /* write(2) is async-signal-safe; printf isn't */
    char msg[128];
    int n = snprintf(msg, sizeof msg,
                     "[signal] SIGSEGV at fault addr %p (this is the stack guard page)\n",
                     si ? si->si_addr : NULL);
    write(2, msg, n);
    siglongjmp(jb, 1);
}

static char altstack[1 << 16];

static long used;
static void grow(void)
{
    char waste[1024];
    waste[0] = 0;
    used += sizeof waste;
    grow();
    (void)waste;
}

int main(void)
{
    struct rlimit rl;
    getrlimit(RLIMIT_STACK, &rl);
    printf("RLIMIT_STACK soft=%lu hard=%lu\n",
           (unsigned long)rl.rlim_cur, (unsigned long)rl.rlim_max);

    stack_t ss = { .ss_sp = altstack, .ss_size = sizeof altstack };
    sigaltstack(&ss, NULL);

    struct sigaction sa = {0};
    sa.sa_sigaction = h;
    sa.sa_flags     = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);

    if (sigsetjmp(jb, 1) == 0) {
        grow();
    } else {
        printf("recovered; approximate stack used = %ld bytes (~%ld KiB)\n",
               used, used / 1024);
    }
    return 0;
}
