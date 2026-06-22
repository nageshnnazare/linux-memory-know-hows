/*
 * 01_memory_hierarchy.c
 * ----------------------
 * Demonstrate the L1 / L2 / L3 / DRAM hierarchy by running the same
 * pointer-chase loop over arrays of different sizes and printing the
 * average time per access.
 *
 *   array size:                  expected to land in:
 *     16 KiB        ............   L1d
 *    256 KiB        ............   L2
 *      8 MiB        ............   L3
 *    256 MiB        ............   DRAM
 *
 * We chase a random permutation so the prefetcher cannot help.
 *
 *   array  -> [ slot 0 ][ slot 1 ][ slot 2 ] ... [ slot N-1 ]
 *                |          |        |
 *                v          v        v
 *               5  ->  next=42  -> next=17  -> ...
 *
 * Build:   gcc -O2 01_memory_hierarchy.c -o 01_memory_hierarchy
 * Run  :   ./01_memory_hierarchy
 */

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* Fisher-Yates shuffle of indices 0..n-1 */
static void shuffle(size_t *a, size_t n)
{
    for (size_t i = n - 1; i > 0; --i) {
        size_t j = (size_t)rand() % (i + 1);
        size_t t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

/*
 * Build a circular linked list inside `buf`.  Each slot is a size_t storing
 * the index of the NEXT slot.  We then chase that chain `iters` times.
 */
static double measure(size_t bytes, size_t iters)
{
    size_t n = bytes / sizeof(size_t);
    size_t *buf = aligned_alloc(64, n * sizeof(size_t));
    if (!buf) return -1.0;

    size_t *idx = malloc(n * sizeof(size_t));
    for (size_t i = 0; i < n; ++i) idx[i] = i;
    shuffle(idx, n);

    /* buf[idx[i]] = idx[i+1]   -> a single cycle visiting every slot */
    for (size_t i = 0; i < n; ++i)
        buf[idx[i]] = idx[(i + 1) % n];

    size_t k = 0;
    double t0 = now_sec();
    for (size_t i = 0; i < iters; ++i) k = buf[k];
    double t1 = now_sec();

    /* keep k live so the compiler doesn't delete the loop */
    if (k == (size_t)-1) fprintf(stderr, "unreachable\n");

    free(idx);
    free(buf);
    return (t1 - t0) * 1e9 / (double)iters; /* ns/access */
}

int main(void)
{
    srand(42);
    size_t sizes[] = {
        16   * 1024,        /*  16 KiB  -> L1 */
        64   * 1024,
        256  * 1024,        /* 256 KiB  -> L2 */
        1    * 1024 * 1024,
        8    * 1024 * 1024, /*   8 MiB  -> L3 */
        64   * 1024 * 1024,
        256  * 1024 * 1024, /* 256 MiB  -> DRAM */
    };

    printf("%-12s  %-10s\n", "size", "ns/access");
    printf("%-12s  %-10s\n", "----", "----------");

    for (size_t i = 0; i < sizeof(sizes)/sizeof(*sizes); ++i) {
        size_t iters = 100 * 1000 * 1000;
        /* fewer iterations for huge arrays so the test finishes */
        if (sizes[i] > 16 * 1024 * 1024) iters /= 10;
        double ns = measure(sizes[i], iters);
        if (sizes[i] >= 1024 * 1024)
            printf("%5zu MiB    %8.2f\n", sizes[i] / (1024*1024), ns);
        else
            printf("%5zu KiB    %8.2f\n", sizes[i] / 1024, ns);
    }
    return 0;
}
