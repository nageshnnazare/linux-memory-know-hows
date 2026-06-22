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
