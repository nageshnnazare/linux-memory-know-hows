/*
 * 03_shared_mmap.c
 * -----------------
 * Allocate a shared anonymous region, then fork.  Both parent and child
 * see the same memory; one writes, the other reads.
 *
 *   parent                                           child
 *     |                                                |
 *     |  mmap(MAP_SHARED|ANONYMOUS) ->  p              |
 *     |                                                |
 *     |  fork() (child inherits the SAME mapping)      |
 *     |                                                |
 *     |  *p = 'P'         ----------+                  |
 *     |                              \                 |
 *     |                               +-> *p visible -> read in child
 *
 * Compare to MAP_PRIVATE -- after fork, the parent's write would be CoW'd
 * away from the child.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>

int main(void)
{
    int *p = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    *p = 0;

    pid_t pid = fork();
    if (pid == 0) {
        /* child: spin until parent sets *p, then print */
        while (__atomic_load_n(p, __ATOMIC_ACQUIRE) == 0) usleep(1000);
        printf("[child] saw *p=%d via SHARED mapping\n", *p);
        _exit(0);
    }
    /* parent: write to the shared region */
    sleep(1);
    __atomic_store_n(p, 42, __ATOMIC_RELEASE);
    waitpid(pid, NULL, 0);
    printf("[parent] done\n");
    munmap(p, 4096);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 05_mmap/03_shared_mmap.c
 * Command: make -C 05_mmap 03_shared_mmap
 * Exit status: 0
 * Output:
 * [parent] done
 * AUTO-GENERATED RUN OUTPUT END
 */
