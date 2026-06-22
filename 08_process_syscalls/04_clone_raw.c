/*
 * 04_clone_raw.c
 * ---------------
 * Call clone(2) directly to make a new process (no shared address space).
 * Equivalent to fork() but lets us explicitly pass flags.
 *
 *   clone(child_fn, child_stack + STACK_SZ, SIGCHLD, arg)
 *
 *     - SIGCHLD          -> notify the parent on exit (just like fork)
 *     - no CLONE_VM      -> address spaces are separate
 *     - we must give it a stack ourselves
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>

#define STK (64 * 1024)

static int child_fn(void *arg)
{
    int n = *(int *)arg;
    printf("[child ] hello from clone'd process; got arg=%d ppid=%d\n",
           n, getppid());
    return 42;        /* becomes exit status */
}

int main(void)
{
    /* allocate a stack for the child; mark MAP_GROWSDOWN/STACK */
    char *stk = mmap(NULL, STK, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stk == MAP_FAILED) { perror("mmap"); return 1; }

    int arg = 7;
    pid_t pid = clone(child_fn, stk + STK, SIGCHLD, &arg);
    if (pid < 0) { perror("clone"); return 1; }

    int s; waitpid(pid, &s, 0);
    printf("[parent] child returned %d\n", WEXITSTATUS(s));

    munmap(stk, STK);
    return 0;
}
