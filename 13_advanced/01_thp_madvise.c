/*
 * 01_thp_madvise.c
 * -----------------
 * Allocate 64 MiB, hint MADV_HUGEPAGE, touch the pages, and read
 * /proc/self/smaps to see "AnonHugePages: ..." for our VMA.
 *
 *   khugepaged works in the background; the promotion may not happen
 *   immediately.  Re-run after a few seconds if you don't see it yet.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int main(void)
{
    size_t len = 64UL << 20;
    void *p = mmap(NULL, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }

    if (madvise(p, len, MADV_HUGEPAGE) < 0) perror("madvise(HUGEPAGE)");
    memset(p, 0x41, len);

    /* Wait a few seconds for khugepaged */
    sleep(3);

    /* Locate the VMA in smaps */
    char our[64]; snprintf(our, sizeof our, "%lx-%lx", (unsigned long)p,
                           (unsigned long)((char *)p + len));
    FILE *f = fopen("/proc/self/smaps", "r");
    char line[512]; int in_ours = 0;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, our)) in_ours = 1;
        if (in_ours) {
            fputs(line, stdout);
            if (line[0] != ' ' && strchr(line, '-') == NULL && in_ours)
                /* hit next VMA */
                break;
            if (strncmp(line, "VmFlags:", 8) == 0) break;
        }
    }
    fclose(f);

    munmap(p, len);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 13_advanced/01_thp_madvise.c
 * Command: make -C 13_advanced 01_thp_madvise
 * Exit status: 0
 * Output:
 * 7f7484400000-7f7488400000 rw-p 00000000 00:00 0 
 * Size:              65536 kB
 * AUTO-GENERATED RUN OUTPUT END
 */
