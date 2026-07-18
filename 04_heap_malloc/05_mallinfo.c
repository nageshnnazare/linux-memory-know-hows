/*
 * 05_mallinfo.c
 * --------------
 * Print every field of mallinfo2() with a one-line explanation, then call
 * malloc_stats() to dump the per-arena stats to stderr.
 *
 *   arena    -- total non-mmapped allocated space (heap from sbrk) in bytes
 *   ordblks  -- number of free chunks
 *   smblks   -- number of fastbin blocks
 *   hblks    -- number of mmap'd allocations currently outstanding
 *   hblkhd   -- bytes used by mmap'd allocations
 *   usmblks  -- (deprecated, always 0)
 *   fsmblks  -- bytes in fastbins
 *   uordblks -- total IN-USE allocated bytes (your "memory in flight")
 *   fordblks -- total free bytes in the heap
 *   keepcost -- top-chunk free bytes (releasable via malloc_trim)
 */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

static void dump(const char *tag)
{
    struct mallinfo2 m = mallinfo2();
    printf("[%s]\n", tag);
    printf("  arena    = %10zu B  (heap from sbrk)\n",      m.arena);
    printf("  hblks    = %10zu     (# mmap allocs)\n",      m.hblks);
    printf("  hblkhd   = %10zu B  (bytes in mmap allocs)\n", m.hblkhd);
    printf("  uordblks = %10zu B  (in-use)\n",              m.uordblks);
    printf("  fordblks = %10zu B  (free in heap)\n",        m.fordblks);
    printf("  fsmblks  = %10zu B  (in fastbins)\n",         m.fsmblks);
    printf("  ordblks  = %10zu     (# free chunks)\n",      m.ordblks);
    printf("  smblks   = %10zu     (# fastbin chunks)\n",   m.smblks);
    printf("  keepcost = %10zu B  (top chunk; releasable)\n", m.keepcost);
}

int main(void)
{
    dump("start");

    void *small[100];
    for (int i = 0; i < 100; ++i) small[i] = malloc(128);
    dump("after 100x malloc(128)");

    for (int i = 0; i < 100; ++i) free(small[i]);
    dump("after 100x free");

    void *big = malloc(8 * 1024 * 1024);
    dump("after malloc(8 MiB)");
    free(big);
    dump("after free(big)");

    fprintf(stderr, "\n=== malloc_stats() to stderr ===\n");
    malloc_stats();
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/05_mallinfo.c
 * Command: make -C 04_heap_malloc 05_mallinfo
 * Exit status: 0
 * Output:
 * 
 * === malloc_stats() to stderr ===
 * Arena 0:
 * system bytes     =     135168
 * in use bytes     =       5776
 * Total (incl. mmap):
 * system bytes     =     135168
 * in use bytes     =       5776
 * max mmap regions =          1
 * max mmap bytes   =    8392704
 * [start]
 *   arena    =          0 B  (heap from sbrk)
 *   hblks    =          0     (# mmap allocs)
 *   hblkhd   =          0 B  (bytes in mmap allocs)
 *   uordblks =          0 B  (in-use)
 *   fordblks =          0 B  (free in heap)
 *   fsmblks  =          0 B  (in fastbins)
 *   ordblks  =          1     (# free chunks)
 *   smblks   =          0     (# fastbin chunks)
 *   keepcost =          0 B  (top chunk; releasable)
 * [after 100x malloc(128)]
 *   arena    =     135168 B  (heap from sbrk)
 *   hblks    =          0     (# mmap allocs)
 *   hblkhd   =          0 B  (bytes in mmap allocs)
 *   uordblks =      19168 B  (in-use)
 *   fordblks =     116000 B  (free in heap)
 *   fsmblks  =          0 B  (in fastbins)
 *   ordblks  =          1     (# free chunks)
 *   smblks   =          0     (# fastbin chunks)
 *   keepcost =     116000 B  (top chunk; releasable)
 * [after 100x free]
 *   arena    =     135168 B  (heap from sbrk)
 *   hblks    =          0     (# mmap allocs)
 *   hblkhd   =          0 B  (bytes in mmap allocs)
 *   uordblks =       5776 B  (in-use)
 *   fordblks =     129392 B  (free in heap)
 *   fsmblks  =          0 B  (in fastbins)
 *   ordblks  =          1     (# free chunks)
 *   smblks   =          0     (# fastbin chunks)
 *   keepcost =     129392 B  (top chunk; releasable)
 * [after malloc(8 MiB)]
 *   arena    =     135168 B  (heap from sbrk)
 *   hblks    =          1     (# mmap allocs)
 *   hblkhd   =    8392704 B  (bytes in mmap allocs)
 *   uordblks =       5776 B  (in-use)
 *   fordblks =     129392 B  (free in heap)
 *   fsmblks  =          0 B  (in fastbins)
 *   ordblks  =          1     (# free chunks)
 *   smblks   =          0     (# fastbin chunks)
 *   keepcost =     129392 B  (top chunk; releasable)
 * [after free(big)]
 *   arena    =     135168 B  (heap from sbrk)
 *   hblks    =          0     (# mmap allocs)
 *   hblkhd   =          0 B  (bytes in mmap allocs)
 *   uordblks =       5776 B  (in-use)
 *   fordblks =     129392 B  (free in heap)
 *   fsmblks  =          0 B  (in fastbins)
 *   ordblks  =          1     (# free chunks)
 *   smblks   =          0     (# fastbin chunks)
 *   keepcost =     129392 B  (top chunk; releasable)
 * AUTO-GENERATED RUN OUTPUT END
 */
