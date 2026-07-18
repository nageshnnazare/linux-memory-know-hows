/*
 * 08_map_populate.c
 * ------------------
 * MAP_POPULATE: prefault the entire mapping so there are no on-demand
 * faults later.  Trades startup latency for predictable runtime.
 *
 *   Time two scenarios:
 *     A: mmap WITHOUT POPULATE  -> first sweep faults each page
 *     B: mmap WITH    POPULATE  -> mmap already faulted them in
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

static double now(void)
{
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static double sweep(char *p, size_t len)
{
    long pg = sysconf(_SC_PAGESIZE);
    double t0 = now();
    for (size_t i = 0; i < len; i += pg) p[i] = 1;
    return now() - t0;
}

int main(void)
{
    size_t len = 256UL << 20;

    char *p = mmap(NULL, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    double t1 = sweep(p, len);
    printf("no POPULATE: first sweep took %.3f s\n", t1);
    double t1b = sweep(p, len);
    printf("no POPULATE: second sweep     %.3f s (already resident)\n", t1b);
    munmap(p, len);

    p = mmap(NULL, len, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    double t2 = sweep(p, len);
    printf("POPULATE   : first sweep took %.3f s (already faulted)\n", t2);
    munmap(p, len);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 05_mmap/08_map_populate.c
 * Command: make -C 05_mmap 08_map_populate
 * Exit status: 0
 * Output:
 * no POPULATE: first sweep took 0.015 s
 * no POPULATE: second sweep     0.001 s (already resident)
 * POPULATE   : first sweep took 0.001 s (already faulted)
 * AUTO-GENERATED RUN OUTPUT END
 */
