/*
 * 03_count_page_faults.c
 * -----------------------
 * Use getrusage() to count minor and major page faults around a workload.
 *
 *   minor fault = page-table miss, kernel allocates a frame, NO disk I/O.
 *                 (e.g. first write to malloc'd memory)
 *   major fault = needed disk read (mmap'd file or swap-in).
 *
 *   workload:                 expected behaviour
 *   ----------------------    --------------------------------------------
 *   mmap 256 MiB ANON          0 faults yet
 *   touch every page (RW)      ~65536 minor faults (256 MiB / 4 KiB)
 *
 * Build:  gcc -O0 -g 03_count_page_faults.c -o 03_count_page_faults
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>

static void diff_rusage(const char *label,
                        const struct rusage *a, const struct rusage *b)
{
    printf("%-22s  minor=%-8ld  major=%-3ld\n",
           label,
           b->ru_minflt - a->ru_minflt,
           b->ru_majflt - a->ru_majflt);
}

int main(void)
{
    const size_t BYTES = 256UL * 1024 * 1024;
    const long pg = sysconf(_SC_PAGESIZE);
    struct rusage r0, r1;

    getrusage(RUSAGE_SELF, &r0);
    char *p = mmap(NULL, BYTES, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    getrusage(RUSAGE_SELF, &r1);
    diff_rusage("mmap (no touch)", &r0, &r1);

    /* touch every page */
    r0 = r1;
    for (size_t i = 0; i < BYTES; i += pg) p[i] = 1;
    getrusage(RUSAGE_SELF, &r1);
    diff_rusage("write each page",  &r0, &r1);

    /* read again - already mapped, no faults */
    r0 = r1;
    volatile char sink = 0;
    for (size_t i = 0; i < BYTES; i += pg) sink = p[i];
    (void)sink;
    getrusage(RUSAGE_SELF, &r1);
    diff_rusage("re-read each page", &r0, &r1);

    /* MADV_DONTNEED throws frames away; next access re-faults */
    r0 = r1;
    madvise(p, BYTES, MADV_DONTNEED);
    for (size_t i = 0; i < BYTES; i += pg) sink = p[i];
    (void)sink;
    getrusage(RUSAGE_SELF, &r1);
    diff_rusage("DONTNEED + re-read", &r0, &r1);

    munmap(p, BYTES);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 01_virtual_memory/03_count_page_faults.c
 * Command: make -C 01_virtual_memory 03_count_page_faults
 * Exit status: 0
 * Output:
 * mmap (no touch)         minor=0         major=0  
 * write each page         minor=135       major=0  
 * re-read each page       minor=0         major=0  
 * DONTNEED + re-read      minor=128       major=0  
 * AUTO-GENERATED RUN OUTPUT END
 */
