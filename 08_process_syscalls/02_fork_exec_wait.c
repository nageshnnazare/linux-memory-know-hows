/*
 * 02_fork_exec_wait.c
 * --------------------
 * The bread-and-butter way to run a program from C:
 *
 *   parent ----fork----> child
 *                          |
 *                          | execve("/bin/echo", ...)
 *                          v
 *                       echo runs (parent's address space is REPLACED)
 *                          |
 *                          | exits
 *                          v
 *   parent <----wait--- exit status
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        char *argv[] = { "/bin/echo", "hello", "from", "child", NULL };
        char *envp[] = { "LANG=C", NULL };
        execve(argv[0], argv, envp);
        perror("execve");      /* only reached if execve fails */
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
        printf("child exited with %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("child killed by signal %d\n", WTERMSIG(status));
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 08_process_syscalls/02_fork_exec_wait.c
 * Command: make -C 08_process_syscalls 02_fork_exec_wait
 * Exit status: 0
 * Output:
 * hello from child
 * child exited with 0
 * AUTO-GENERATED RUN OUTPUT END
 */
