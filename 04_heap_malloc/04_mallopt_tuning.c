/*
 * 04_mallopt_tuning.c
 * --------------------
 * Tune ptmalloc and print mallinfo2 before/after.
 *
 *   M_MMAP_THRESHOLD  -- size at/above which malloc uses mmap directly
 *   M_TRIM_THRESHOLD  -- if top chunk grows beyond this, sbrk back to OS
 *   M_TOP_PAD         -- bonus padding added to each sbrk extension
 *   M_ARENA_MAX       -- cap on per-thread arenas (1 = single arena)
 *   M_PERTURB         -- byte to overwrite freed memory with (debugging)
 */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

static void show(const char *tag)
{
    struct mallinfo2 mi = mallinfo2();
    printf("[%-12s] arena=%zu hblks=%zu hblkhd=%zu  uordblks=%zu  fordblks=%zu\n",
           tag, mi.arena, mi.hblks, mi.hblkhd, mi.uordblks, mi.fordblks);
}

int main(void)
{
    show("start");

    /* Force every malloc >= 4096 to use mmap. */
    mallopt(M_MMAP_THRESHOLD, 4096);
    show("after mmap_thr=4K");

    void *p[8];
    for (int i = 0; i < 8; ++i) p[i] = malloc(64 * 1024);
    show("after big mallocs");

    for (int i = 0; i < 8; ++i) free(p[i]);
    show("after free");

    /* Splat 0xCC into every freed chunk so use-after-free yields obvious garbage. */
    mallopt(M_PERTURB, 0xCC);
    void *q = malloc(32);
    free(q);
    /* q is dangling now; reading would show 0xCC fill. */

    /* Force a trim attempt */
    malloc_trim(0);
    show("after trim");
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/04_mallopt_tuning.c
 * Command: make -C 04_heap_malloc 04_mallopt_tuning
 * Exit status: 0
 * Output:
 * [start       ] arena=0 hblks=0 hblkhd=0  uordblks=0  fordblks=0
 * [after mmap_thr=4K] arena=135168 hblks=0 hblkhd=0  uordblks=4768  fordblks=130400
 * [after big mallocs] arena=135168 hblks=7 hblkhd=487424  uordblks=70320  fordblks=64848
 * [after free  ] arena=135168 hblks=0 hblkhd=0  uordblks=4768  fordblks=130400
 * [after trim  ] arena=8192 hblks=0 hblkhd=0  uordblks=4816  fordblks=3376
 * AUTO-GENERATED RUN OUTPUT END
 */
