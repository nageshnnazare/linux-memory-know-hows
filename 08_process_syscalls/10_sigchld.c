/*
 * 10_sigchld.c
 * -------------
 * Reap any number of children asynchronously by handling SIGCHLD with a
 * WNOHANG-looping waitpid.
 *
 *   The classic mistake:  do `waitpid(-1, &s, 0)` in the handler and only
 *   reap one child per SIGCHLD.  SIGCHLD is NOT queued; if two children
 *   exit in quick succession, you get ONE signal -- the second zombie
 *   stays.  Always loop with WNOHANG until ECHILD or 0.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

static volatile sig_atomic_t reaped = 0;

static void on_chld(int s, siginfo_t *si, void *uc)
{
    (void)s; (void)si; (void)uc;
    int saved = errno;
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) reaped++;
    errno = saved;
}

int main(void)
{
    struct sigaction sa = {0};
    sa.sa_sigaction = on_chld;
    sa.sa_flags = SA_SIGINFO | SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    for (int i = 0; i < 5; ++i) {
        pid_t c = fork();
        if (c == 0) {
            usleep((i + 1) * 100000);
            _exit(0);
        }
        printf("[parent] spawned %d\n", c);
    }

    /* sleep generously so SIGCHLDs land */
    while (reaped < 5) pause();
    printf("[parent] reaped %d children\n", reaped);
    return 0;
}
