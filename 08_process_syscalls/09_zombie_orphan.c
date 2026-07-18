/*
 * 09_zombie_orphan.c
 * -------------------
 * Demonstrate the two life-after-death scenarios.
 *
 *   ZOMBIE  : child exits before parent waits.
 *             Visible as state Z in ps; ppid is still us.
 *
 *   ORPHAN  : parent exits before child.
 *             init/systemd adopts the child (ppid=1).
 *
 * Run:        ./09_zombie_orphan zombie   then   ps -e -o pid,ppid,stat,cmd | grep 09
 *             ./09_zombie_orphan orphan
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s zombie|orphan\n", argv[0]); return 1; }

    if (!strcmp(argv[1], "zombie")) {
        pid_t c = fork();
        if (c == 0) _exit(0);
        printf("parent pid=%d  child pid=%d (now a zombie, not waited)\n",
               getpid(), c);
        printf("look: ps -o pid,ppid,stat,cmd -p %d\n", c);
        sleep(20);
        waitpid(c, NULL, 0);
    } else if (!strcmp(argv[1], "orphan")) {
        pid_t c = fork();
        if (c == 0) {
            printf("[child] ppid=%d  -- I'm about to be orphaned in 1 sec\n", getppid());
            sleep(2);
            printf("[child] now ppid=%d (re-parented to init)\n", getppid());
            _exit(0);
        }
        printf("[parent] pid=%d  exiting now to orphan %d\n", getpid(), c);
        _exit(0);
    } else {
        fprintf(stderr, "unknown mode '%s'\n", argv[1]);
        return 1;
    }
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 08_process_syscalls/09_zombie_orphan.c
 * Command: make -C 08_process_syscalls 09_zombie_orphan
 * Exit status: 0
 * Output:
 * usage: ./09_zombie_orphan zombie|orphan
 * AUTO-GENERATED RUN OUTPUT END
 */
