/*
 * 06_madvise_dontneed.c
 * ----------------------
 * Allocate 256 MiB anon, dirty it, then drop with madvise(MADV_DONTNEED).
 * RSS goes back to near zero; reading the region returns zero.
 *
 *   MADV_DONTNEED  -- discard the frames immediately, future reads = 0
 *   MADV_FREE      -- lazy discard, kernel reclaims at memory pressure
 *
 * Useful for explicit "I no longer need this big buffer" without unmapping.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

static long rss_kib(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    char line[256]; long rss = 0;
    while (fgets(line, sizeof line, f))
        if (!strncmp(line, "VmRSS:", 6)) sscanf(line + 6, "%ld", &rss);
    fclose(f); return rss;
}

int main(void)
{
    size_t len = 256UL * 1024 * 1024;
    char *p = mmap(NULL, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }

    printf("after mmap        : RSS = %ld KiB\n", rss_kib());
    memset(p, 1, len);
    printf("after memset      : RSS = %ld KiB\n", rss_kib());

    madvise(p, len, MADV_DONTNEED);
    printf("after DONTNEED    : RSS = %ld KiB\n", rss_kib());

    /* a fresh read returns zeros */
    printf("p[0]=%d (zeroed)  p[1MB]=%d\n", p[0], p[1024*1024]);
    printf("after one read    : RSS = %ld KiB (zero page mapped RO)\n", rss_kib());

    munmap(p, len);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 05_mmap/06_madvise_dontneed.c
 * Command: make -C 05_mmap 06_madvise_dontneed
 * Exit status: 0
 * Output:
 * after mmap        : RSS = 1360 KiB
 * after memset      : RSS = 263764 KiB
 * after DONTNEED    : RSS = 1620 KiB
 * p[0]=0 (zeroed)  p[1MB]=0
 * after one read    : RSS = 1620 KiB (zero page mapped RO)
 * AUTO-GENERATED RUN OUTPUT END
 */
