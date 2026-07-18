/*
 * 07_mremap.c
 * ------------
 * mremap() can grow, shrink, or move a mapping without copying data --
 * the kernel just re-wires page tables.
 *
 *   step 1: mmap 1 MiB.  store a marker.
 *   step 2: mremap to 16 MiB (MAYMOVE).  marker preserved at the new VA.
 *   step 3: mremap back to 64 KiB.  marker still there.
 *   step 4: munmap.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

int main(void)
{
    size_t a = 1UL << 20;        /* 1 MiB */
    char *p = mmap(NULL, a, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    strcpy(p, "marker");
    printf("mmap'd %zu bytes @ %p; first 6 bytes = \"%s\"\n", a, p, p);

    /* Grow to 16 MiB; may move */
    size_t b = 16UL << 20;
    char *q = mremap(p, a, b, MREMAP_MAYMOVE);
    if (q == MAP_FAILED) { perror("mremap up"); return 1; }
    printf("mremap %zu -> %zu  new=%p (%s)  marker=\"%s\"\n",
           a, b, q, (q == p ? "in-place" : "moved"), q);

    /* Shrink back to 64 KiB */
    size_t c = 64 << 10;
    char *r = mremap(q, b, c, MREMAP_MAYMOVE);
    if (r == MAP_FAILED) { perror("mremap down"); return 1; }
    printf("mremap %zu -> %zu  new=%p marker=\"%s\"\n", b, c, r, r);

    munmap(r, c);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 05_mmap/07_mremap.c
 * Command: make -C 05_mmap 07_mremap
 * Exit status: 0
 * Output:
 * mmap'd 1048576 bytes @ 0x7f80db300000; first 6 bytes = "marker"
 * mremap 1048576 -> 16777216  new=0x7f80da200000 (moved)  marker="marker"
 * mremap 16777216 -> 65536  new=0x7f80da200000 marker="marker"
 * AUTO-GENERATED RUN OUTPUT END
 */
