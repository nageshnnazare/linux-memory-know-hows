/*
 * 03_vfork.c
 * -----------
 * vfork() shares the parent's address space and suspends the parent until
 * the child execve's or _exit's.  The child must NOT touch any variables
 * (other than the local PID returned by vfork).
 *
 * The classic safe pattern is:  vfork(); execve(...); _exit(...);
 *
 * Use case: very cheap "spawn" when you can't spare the CoW cost.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    printf("[parent] before vfork, pid=%d\n", getpid());

    pid_t pid = vfork();
    if (pid < 0) { perror("vfork"); return 1; }
    if (pid == 0) {
        /* child: ONLY exec or _exit; nothing else */
        execlp("/bin/echo", "echo", "child ran via vfork", NULL);
        _exit(127);
    }

    /* parent resumes only after child execve/_exit */
    int s; waitpid(pid, &s, 0);
    printf("[parent] child exit = %d\n", WEXITSTATUS(s));
    return 0;
}
