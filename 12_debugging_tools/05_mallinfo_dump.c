/*
 * 05_mallinfo_dump.c
 * -------------------
 * Print mallinfo2, malloc_stats, and malloc_info.
 */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

int main(void)
{
    enum { N = 1000 };
    void *p[N];
    for (int i = 0; i < N; ++i) p[i] = malloc(64 + i);

    struct mallinfo2 mi = mallinfo2();
    printf("=== mallinfo2 ===\n");
    printf("  arena    = %zu\n",  mi.arena);
    printf("  hblks    = %zu\n",  mi.hblks);
    printf("  hblkhd   = %zu\n",  mi.hblkhd);
    printf("  uordblks = %zu\n",  mi.uordblks);
    printf("  fordblks = %zu\n",  mi.fordblks);
    printf("  keepcost = %zu\n",  mi.keepcost);

    printf("\n=== malloc_stats() to stderr ===\n");
    malloc_stats();

    printf("\n=== malloc_info() to stderr ===\n");
    malloc_info(0, stderr);

    for (int i = 0; i < N; ++i) free(p[i]);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 12_debugging_tools/05_mallinfo_dump.c
 * Command: make -C 12_debugging_tools 05_mallinfo_dump
 * Exit status: 0
 * Output:
 * Arena 0:
 * system bytes     =     675840
 * in use bytes     =     583744
 * Total (incl. mmap):
 * system bytes     =     675840
 * in use bytes     =     583744
 * max mmap regions =          0
 * max mmap bytes   =          0
 * <malloc version="1">
 * <heap nr="0">
 * <sizes>
 * </sizes>
 * <total type="fast" count="0" size="0"/>
 * <total type="rest" count="1" size="92096"/>
 * <system type="current" size="675840"/>
 * <system type="max" size="675840"/>
 * <aspace type="total" size="675840"/>
 * <aspace type="mprotect" size="675840"/>
 * </heap>
 * <total type="fast" count="0" size="0"/>
 * <total type="rest" count="1" size="92096"/>
 * <total type="mmap" count="0" size="0"/>
 * <system type="current" size="675840"/>
 * <system type="max" size="675840"/>
 * <aspace type="total" size="675840"/>
 * <aspace type="mprotect" size="675840"/>
 * </malloc>
 * === mallinfo2 ===
 *   arena    = 675840
 *   hblks    = 0
 *   hblkhd   = 0
 *   uordblks = 579632
 *   fordblks = 96208
 *   keepcost = 96208
 * 
 * === malloc_stats() to stderr ===
 * 
 * === malloc_info() to stderr ===
 * AUTO-GENERATED RUN OUTPUT END
 */
