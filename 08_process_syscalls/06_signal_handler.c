/*
 * 06_signal_handler.c
 * --------------------
 * Catch SIGINT and SIGTERM using sigaction (the right way; signal(3) is
 * legacy).
 *
 *   Run this:    ./06_signal_handler
 *   Press Ctrl-C; the handler will set a flag; main loop notices and exits.
 *   Or from another shell:  kill -TERM <pid>
 *
 *   Why a flag instead of doing work in the handler?
 *     Because most libc functions are NOT async-signal-safe.  See
 *     `signal-safety(7)`.  Safe inside a handler: write(), _exit(),
 *     sig_atomic_t flag reads/writes, and a tiny list of others.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

static volatile sig_atomic_t g_stop = 0;
static volatile sig_atomic_t g_lastsig = 0;

static void handler(int sig, siginfo_t *si, void *uc)
{
    (void)uc;
    g_stop = 1;
    g_lastsig = sig;
    /* write is async-signal-safe; printf is NOT */
    const char *m = (sig == SIGINT) ? "got SIGINT\n" : "got SIGTERM\n";
    int unused = write(2, m, strlen(m));
    (void)unused; (void)si;
}

int main(void)
{
    struct sigaction sa = {0};
    sa.sa_sigaction = handler;
    sa.sa_flags     = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    printf("pid=%d  press Ctrl-C or kill -TERM %d ...\n", getpid(), getpid());

    /* Block in a syscall, wait for the signal */
    while (!g_stop) {
        /* pause() returns when ANY signal handler runs */
        pause();
    }
    printf("exiting; last sig = %d\n", g_lastsig);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 08_process_syscalls/06_signal_handler.c
 * Command: make -C 08_process_syscalls 06_signal_handler
 * Exit status: 0
 * Output:
 * (no output)
 * AUTO-GENERATED RUN OUTPUT END
 */
