/*
 * 07_daemonise.c
 * ---------------
 * The textbook double-fork daemon recipe:
 *
 *   parent
 *     |
 *     fork() -> child1
 *     exit(0)             child1 lives; parent is now /init's child
 *                            |
 *                            setsid()           // new session, no tty
 *                            fork() -> child2
 *                            exit(0)            // child1 done
 *                                                  |
 *                                                  chdir("/")
 *                                                  umask(0)
 *                                                  close stdin/out/err
 *                                                  ...do the daemon stuff...
 *
 *   After this, ps -ef shows ppid=1 and no controlling tty.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

static void daemonise(void)
{
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);          /* original parent leaves */

    if (setsid() < 0) exit(1);     /* become session leader */

    pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);          /* session leader leaves */

    chdir("/");
    umask(0);

    int fd = open("/dev/null", O_RDWR);
    dup2(fd, 0); dup2(fd, 1); dup2(fd, 2);
    if (fd > 2) close(fd);
}

int main(void)
{
    daemonise();

    openlog("daemonise-demo", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "alive: pid=%d ppid=%d", getpid(), getppid());
    sleep(30);
    syslog(LOG_INFO, "exiting");
    closelog();
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 08_process_syscalls/07_daemonise.c
 * Command: make -C 08_process_syscalls 07_daemonise
 * Exit status: 0
 * Output:
 * (no output)
 * AUTO-GENERATED RUN OUTPUT END
 */
