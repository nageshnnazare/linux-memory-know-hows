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

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/03_brk_vs_mmap.c
 * Command: make -C 04_heap_malloc 03_brk_vs_mmap
 * Exit status: 0
 * Output:
 * program break (sbrk(0)) = 0x561fbfb0b000
 * small allocations (1 KiB each):
 *   small[0] = 0x561fbfb0c2b0
 *   small[1] = 0x561fbfb0c6c0
 *   small[2] = 0x561fbfb0cad0
 *   small[3] = 0x561fbfb0cee0
 * big   allocations (200 KiB each):
 *   big[0]   = 0x7fe31a84f010
 *   big[1]   = 0x7fe31a81c010
 *   big[2]   = 0x7fe31a5cd010
 *   big[3]   = 0x7fe31a59a010
 * 
 * new program break        = 0x561fbfb2c000   (grew? yes)
 * 
 * /proc/self/maps (filtered):
 * 561fbfb0b000-561fbfb2c000 rw-p 00000000 00:00 0                          [heap]
 * 7fe31a59a000-7fe31a600000 rw-p 00000000 00:00 0 
 * 7fe31a805000-7fe31a812000 rw-p 00000000 00:00 0 
 * 7fe31a81c000-7fe31a885000 rw-p 00000000 00:00 0 
 * 7fe31a88f000-7fe31a891000 rw-p 00000000 00:00 0 
 * 7fe31a891000-7fe31a895000 r--p 00000000 00:00 0                          [vvar]
 * 7fe31a895000-7fe31a897000 r--p 00000000 00:00 0                          [vvar_vclock]
 * 7fe31a897000-7fe31a899000 r-xp 00000000 00:00 0                          [vdso]
 * 7ffc6e489000-7ffc6e4ab000 rw-p 00000000 00:00 0                          [stack]
 * ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
 * AUTO-GENERATED RUN OUTPUT END
 */
