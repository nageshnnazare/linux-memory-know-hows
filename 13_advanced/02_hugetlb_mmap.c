/*
 * 02_hugetlb_mmap.c
 * ------------------
 * Allocate a 2 MiB hugepage with mmap.  Prerequisites:
 *
 *   sudo sh -c 'echo 64 > /proc/sys/vm/nr_hugepages'
 *   cat /proc/meminfo | grep HugePages
 *
 *   MAP_HUGETLB|MAP_HUGE_2MB asks for a 2 MiB page.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#ifndef MAP_HUGE_2MB
#define MAP_HUGE_2MB (21 << MAP_HUGE_SHIFT)
#endif

int main(void)
{
    size_t len = 2UL << 20; /* one 2 MiB huge page */
    void *p = mmap(NULL, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_2MB,
                   -1, 0);
    if (p == MAP_FAILED) { perror("mmap(HUGETLB) -- need nr_hugepages>0"); return 1; }

    memset(p, 0x42, len);
    printf("got 2 MiB huge page @ %p\n", p);

    /* one TLB entry covers all of `p` */
    munmap(p, len);
    return 0;
}
