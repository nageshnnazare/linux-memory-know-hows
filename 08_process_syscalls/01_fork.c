/*
 * 01_fork.c
 * ----------
 * The canonical hello-world for fork().
 *
 *   parent                         child
 *     |                              |
 *     | fork() returns child pid     | fork() returns 0
 *     |                              |
 *     | print PARENT line            | print CHILD line
 *     |                              | _exit(0)
 *     | waitpid()                    |
 *     |                              X
 *     | print "child exited"
 *     |
 *     | exit(0)
 *
 * Note: the heap variable `counter` was 0 before fork; both processes
 * have an independent copy thereafter (CoW until either writes it).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int counter = 0;
    printf("[parent before fork] pid=%d counter=%d (&counter=%p)\n",
           getpid(), counter, (void *)&counter);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        counter = 100;
        printf("[child ]  pid=%d ppid=%d counter=%d (&counter=%p)\n",
               getpid(), getppid(), counter, (void *)&counter);
        _exit(0);
    }

    counter = 1;
    int status;
    waitpid(pid, &status, 0);
    printf("[parent after wait ]  pid=%d counter=%d (&counter=%p)\n",
           getpid(), counter, (void *)&counter);
    printf("[parent] child exit status = %d\n",
           WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 08_process_syscalls/01_fork.c
 * Command: make -C 08_process_syscalls 01_fork
 * Exit status: 0
 * Output:
 * [parent before fork] pid=3143 counter=0 (&counter=0x7ffe484370ac)
 * [parent after wait ]  pid=3143 counter=1 (&counter=0x7ffe484370ac)
 * [parent] child exit status = 0
 * AUTO-GENERATED RUN OUTPUT END
 */
