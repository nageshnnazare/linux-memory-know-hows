/*
 * 03_brk_vs_mmap.c
 * -----------------
 * Demonstrate that small malloc()s come from the brk-managed heap and big
 * ones come from their own mmap region.  We print the addresses and the
 * current program break and dump /proc/self/maps.
 *
 *   small allocations:  addresses clustered around the [heap] VMA.
 *   large allocations:  each lives in its own anon VMA in the mmap area.
 *
 * Threshold defaults to 128 KiB but is dynamic -- glibc tunes it based on
 * previous free()s.  We override with mallopt to force the threshold.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>

static void dump_maps_filtered(void)
{
    FILE *f = fopen("/proc/self/maps", "r");
    char line[512];
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "[heap]") || strstr(line, "(anon)")
            || strstr(line, "00:00 0")) fputs(line, stdout);
    }
    fclose(f);
}

#include <string.h>
int main(void)
{
    mallopt(M_MMAP_THRESHOLD, 64 * 1024);

    printf("program break (sbrk(0)) = %p\n", sbrk(0));

    void *small[4];
    for (int i = 0; i < 4; ++i) { small[i] = malloc(1024); }
    void *big[4];
    for (int i = 0; i < 4; ++i) { big[i]   = malloc(200 * 1024); }

    printf("small allocations (1 KiB each):\n");
    for (int i = 0; i < 4; ++i) printf("  small[%d] = %p\n", i, small[i]);
    printf("big   allocations (200 KiB each):\n");
    for (int i = 0; i < 4; ++i) printf("  big[%d]   = %p\n", i, big[i]);

    printf("\nnew program break        = %p   (grew? %s)\n",
           sbrk(0),
           ((char *)sbrk(0) > (char *)small[0]) ? "yes" : "maybe");

    puts("\n/proc/self/maps (filtered):");
    dump_maps_filtered();

    for (int i = 0; i < 4; ++i) { free(small[i]); free(big[i]); }
    return 0;
}
