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
