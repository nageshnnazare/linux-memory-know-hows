/*
 * 03_segfault_handler.c
 * ----------------------
 * Install a SIGSEGV handler that runs on an alternate stack and prints the
 * fault address.  Useful for: post-mortem dumps, JIT page-fault tricks,
 * automatic recovery in some servers.
 *
 *   1. sigaltstack -- reserve a separate small stack for handlers
 *   2. sigaction with SA_ONSTACK | SA_SIGINFO
 *   3. siginfo_t->si_addr is the faulting virtual address
 *
 * Run this and watch it crash gracefully (after printing useful info).
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static char altstk[64 * 1024];

static void h(int sig, siginfo_t *si, void *uc)
{
    (void)uc;
    char msg[160];
    int n = snprintf(msg, sizeof msg,
                     "[handler] signal %d at addr %p (errno=%d code=%d)\n",
                     sig, si->si_addr, si->si_errno, si->si_code);
    int unused = write(2, msg, n);
    (void)unused;
    _exit(128 + sig);
}

int main(void)
{
    stack_t ss = { .ss_sp = altstk, .ss_size = sizeof altstk };
    sigaltstack(&ss, NULL);

    struct sigaction sa = {0};
    sa.sa_sigaction = h;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS,  &sa, NULL);

    /* Deliberately deref NULL to trigger SIGSEGV */
    printf("about to deref NULL...\n");
    int *p = NULL;
    *p = 42;
    printf("unreachable\n");
    return 0;
}
