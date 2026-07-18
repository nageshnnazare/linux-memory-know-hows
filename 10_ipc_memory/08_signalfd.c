/*
 * 08_signalfd.c
 * --------------
 * Deliver signals through an fd, sidestepping handler safety constraints.
 *
 *   1. block the signal so default handler doesn't fire
 *   2. signalfd() returns a read'able fd that yields signalfd_siginfo
 *   3. read it, dispatch normally (full libc available)
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <string.h>

int main(void)
{
    sigset_t s; sigemptyset(&s); sigaddset(&s, SIGTERM); sigaddset(&s, SIGINT);
    sigprocmask(SIG_BLOCK, &s, NULL);

    int sfd = signalfd(-1, &s, SFD_CLOEXEC);
    if (sfd < 0) { perror("signalfd"); return 1; }

    printf("pid=%d  send me SIGINT (Ctrl-C) or SIGTERM (kill -TERM %d)\n",
           getpid(), getpid());

    struct signalfd_siginfo si;
    ssize_t n = read(sfd, &si, sizeof si);
    if (n == sizeof si)
        printf("got signal %d from pid=%d\n", si.ssi_signo, si.ssi_pid);
    close(sfd);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 10_ipc_memory/08_signalfd.c
 * Command: make -C 10_ipc_memory 08_signalfd
 * Exit status: 0
 * Output:
 * (no output)
 * AUTO-GENERATED RUN OUTPUT END
 */
