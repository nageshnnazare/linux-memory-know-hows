/*
 * 06_memfd_create.c
 * ------------------
 * memfd_create -- create an anonymous file in RAM, get an fd back.
 *
 *   Use cases:
 *     - share data between unrelated processes via SCM_RIGHTS
 *     - JIT: write code to memfd, mmap PROT_EXEC
 *     - SEALS: lock down so receivers know the content can't change
 *
 *   No filesystem path; no quota concerns; lives in tmpfs (RAM).
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/memfd.h>
#include <fcntl.h>
#include <string.h>

#ifndef SYS_memfd_create
#define SYS_memfd_create 319 /* x86_64 */
#endif

static int memfd_create_(const char *name, unsigned flags)
{ return syscall(SYS_memfd_create, name, flags); }

int main(void)
{
    int fd = memfd_create_("demo", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) { perror("memfd_create"); return 1; }
    printf("memfd_create fd = %d  (look in /proc/%d/fd/%d)\n", fd, getpid(), fd);

    ftruncate(fd, 4096);
    char *p = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    strcpy(p, "hello memfd!");
    printf("via mmap: \"%s\"\n", p);

    /* lock down further changes to the size / content */
    if (fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE) < 0)
        perror("F_ADD_SEALS (need MFD_ALLOW_SEALING)");
    int seals = fcntl(fd, F_GET_SEALS);
    printf("current seals = 0x%x\n", seals);

    munmap(p, 4096);
    close(fd);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 10_ipc_memory/06_memfd_create.c
 * Command: make -C 10_ipc_memory 06_memfd_create
 * Exit status: 0
 * Output:
 * F_ADD_SEALS (need MFD_ALLOW_SEALING): Device or resource busy
 * memfd_create fd = 3  (look in /proc/3452/fd/3)
 * via mmap: "hello memfd!"
 * current seals = 0x0
 * AUTO-GENERATED RUN OUTPUT END
 */
