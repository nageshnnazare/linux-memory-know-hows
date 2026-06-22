/*
 * 05_clone_thread.c
 * ------------------
 * Use clone() to make a "thread" -- a child task that SHARES our address
 * space, fd table, and signal handlers.  This is what pthread_create()
 * does under the hood.
 *
 *   flags:
 *     CLONE_VM      -- share the entire address space
 *     CLONE_FS      -- share cwd, umask, root
 *     CLONE_FILES   -- share fd table
 *     CLONE_SIGHAND -- share signal handlers
 *     CLONE_THREAD  -- same TGID -> ps will see one process
 *     CLONE_SYSVSEM -- share SysV semadj
 *     CLONE_PARENT  -- new task's parent = caller's parent
 *
 * NOTE: this does NOT give us the niceties of pthreads (TLS, errno, etc.)
 * It's purely a demo of what's under pthread_create.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <linux/futex.h>
#include <stdatomic.h>

#define STK (256 * 1024)

static int shared_counter = 0;

static int child_fn(void *arg)
{
    (void)arg;
    for (int i = 0; i < 1000; ++i)
        atomic_fetch_add((_Atomic int *)&shared_counter, 1);
    printf("[thread] tid=%ld counter=%d\n",
           syscall(SYS_gettid), shared_counter);
    return 0;
}

int main(void)
{
    char *stk = mmap(NULL, STK, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND
              | CLONE_THREAD | CLONE_SYSVSEM | CLONE_PARENT;
    pid_t tid = clone(child_fn, stk + STK, flags, NULL);
    if (tid < 0) { perror("clone"); return 1; }

    for (int i = 0; i < 1000; ++i)
        atomic_fetch_add((_Atomic int *)&shared_counter, 1);

    /* poor-man's join: just spin until counter reaches 2000.
       Real pthreads uses CLONE_CHILD_CLEARTID + futex_wake on the TID. */
    while (atomic_load((_Atomic int *)&shared_counter) < 2000) usleep(1000);

    printf("[main  ] tid=%ld counter=%d (expected 2000)\n",
           syscall(SYS_gettid), shared_counter);
    return 0;
}
