/*
 * 05_overcommit.c
 * ----------------
 * Allocate way more virtual address space than the system has physical
 * memory.  With MAP_NORESERVE (and default vm.overcommit_memory=0/1) it
 * usually succeeds; with vm.overcommit_memory=2 it can fail.
 *
 *   You only get into trouble when you TOUCH more pages than fit.  At
 *   that point the OOM killer steps in.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int main(void)
{
    /* try 256 GiB virtual */
    size_t n = 256UL * 1024 * 1024 * 1024;
    char *p = mmap(NULL, n, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    printf("mmap'd 256 GiB virtual at %p (no physical RAM used yet)\n", p);

    /* Read /proc/self/status to confirm VSZ */
    FILE *f = fopen("/proc/self/status", "r");
    char l[256];
    while (fgets(l, sizeof l, f))
        if (!strncmp(l, "VmSize:", 7) || !strncmp(l, "VmRSS:", 6)
            || !strncmp(l, "VmData:", 7))
            fputs(l, stdout);
    fclose(f);

    printf("\nNow touching just 64 MiB (not 256 GiB)...\n");
    memset(p, 0x42, 64UL << 20);
    f = fopen("/proc/self/status", "r");
    while (fgets(l, sizeof l, f))
        if (!strncmp(l, "VmRSS:", 6)) fputs(l, stdout);
    fclose(f);

    munmap(p, n);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 07_paging_swapping/05_overcommit.c
 * Command: make -C 07_paging_swapping 05_overcommit
 * Exit status: 0
 * Output:
 * mmap'd 256 GiB virtual at 0x7ebe1e000000 (no physical RAM used yet)
 * VmSize:	268438148 kB
 * VmRSS:	    1484 kB
 * VmData:	268435680 kB
 * 
 * Now touching just 64 MiB (not 256 GiB)...
 * VmRSS:	   67020 kB
 * AUTO-GENERATED RUN OUTPUT END
 */
