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
