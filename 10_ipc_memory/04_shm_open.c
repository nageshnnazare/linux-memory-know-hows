/*
 * 04_shm_open.c
 * --------------
 * POSIX shared memory.  Two processes (parent + child) attach to the same
 * /dev/shm/... object via mmap and synchronise through it.
 *
 *   parent:
 *     fd = shm_open("/foo", O_CREAT|O_RDWR, 0600)
 *     ftruncate(fd, sz)
 *     p = mmap(MAP_SHARED, fd, 0)
 *     fork()
 *   ...both processes see the same memory through different VAs (same frames)
 *
 *   shm_unlink("/foo") removes the name; the mapping persists until
 *   both processes munmap().
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdatomic.h>
#include <string.h>

struct shared { _Atomic int ready; char msg[64]; };

int main(void)
{
    const char *name = "/mem_guide_shm";
    shm_unlink(name);
    int fd = shm_open(name, O_CREAT | O_RDWR, 0600);
    ftruncate(fd, sizeof(struct shared));
    struct shared *s = mmap(NULL, sizeof *s, PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, 0);
    close(fd);
    s->ready = 0;

    pid_t pid = fork();
    if (pid == 0) {
        while (atomic_load(&s->ready) == 0) usleep(1000);
        printf("[child] got: \"%s\"\n", s->msg);
        _exit(0);
    }
    strncpy(s->msg, "hello via shm_open", sizeof s->msg);
    atomic_store(&s->ready, 1);
    waitpid(pid, NULL, 0);

    munmap(s, sizeof *s);
    shm_unlink(name);
    return 0;
}
