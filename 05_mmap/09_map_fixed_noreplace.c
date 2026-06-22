/*
 * 09_map_fixed_noreplace.c
 * -------------------------
 * MAP_FIXED      -- place at the requested address; if anything is there,
 *                   CLOBBER it (unmap and replace).  Footgun.
 * MAP_FIXED_NOREPLACE -- place at the requested address; if anything is
 *                   there, fail with EEXIST.  Safer.
 *
 * Use cases: shared libraries / runtime linkers / language runtimes that
 * want a specific layout, sandboxes, debuggers.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

int main(void)
{
    /* 1) pick a known-free address by mmap'ing then unmapping */
    void *p = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    munmap(p, 4096);
    printf("scratch addr = %p\n", p);

    /* 2) place at exactly that address with FIXED_NOREPLACE */
    void *q = mmap(p, 4096, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (q == MAP_FAILED) { perror("mmap NOREPLACE"); return 1; }
    printf("first NOREPLACE at %p: %s\n", q, (q == p) ? "OK" : "moved (shouldn't happen)");

    /* 3) try again at the SAME address; this time it should fail */
    void *r = mmap(p, 4096, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (r == MAP_FAILED && errno == EEXIST)
        printf("second NOREPLACE at %p: EEXIST  (good)\n", p);
    else
        printf("second NOREPLACE: surprise! r=%p errno=%d\n", r, errno);

    /* 4) MAP_FIXED would have clobbered it.  DON'T use unless you know
     * exactly what's at that address. */
    munmap(q, 4096);
    return 0;
}
