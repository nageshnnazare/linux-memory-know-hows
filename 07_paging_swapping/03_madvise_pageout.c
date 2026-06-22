/*
 * 03_madvise_pageout.c
 * ---------------------
 * MADV_PAGEOUT (Linux 5.4+) hints "you can write these pages to swap now,
 * but don't unmap them".  Compare to MADV_DONTNEED which throws away
 * anonymous pages entirely (next read = zeros).
 *
 * Requires actual swap; if there's none, the hint is silently ignored.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#ifndef MADV_PAGEOUT
#define MADV_PAGEOUT 21
#endif

static long rss(void) {
    FILE *f = fopen("/proc/self/status", "r"); char l[256]; long r = 0;
    while (fgets(l, sizeof l, f))
        if (!strncmp(l, "VmRSS:", 6)) sscanf(l + 6, "%ld", &r);
    fclose(f); return r;
}

int main(void)
{
    size_t n = 64UL << 20;  /* 64 MiB */
    char *p = mmap(NULL, n, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset(p, 0xAA, n);
    printf("touched: RSS=%ld KiB\n", rss());

    if (madvise(p, n, MADV_PAGEOUT) < 0)
        perror("madvise(PAGEOUT) -- need swap and Linux 5.4+");
    printf("after MADV_PAGEOUT: RSS=%ld KiB (may take a moment)\n", rss());

    /* On next access, kernel reads back from swap (major fault). */
    volatile int x = p[1024 * 1024];
    (void)x;
    printf("after touch: RSS=%ld KiB\n", rss());

    munmap(p, n);
    return 0;
}
