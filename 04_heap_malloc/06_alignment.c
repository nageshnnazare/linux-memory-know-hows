/*
 * 06_alignment.c
 * ---------------
 * Show the different allocator alignment APIs.
 *
 *   default malloc          : 16-byte aligned on x86_64 glibc
 *   aligned_alloc(a, sz)    : C11; sz must be a multiple of a; a power of 2
 *   posix_memalign(&p,a,sz) : POSIX; returns 0 or an errno
 *   memalign(a, sz)         : glibc legacy
 *   valloc(sz)              : page-aligned (deprecated)
 *
 *   Use cases:
 *     SSE 128b       -> 16-byte alignment   (already default)
 *     AVX 256b       -> 32-byte alignment
 *     AVX-512 512b   -> 64-byte alignment   (cache line)
 *     DMA buffers    -> page-aligned
 */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>

static int aligned(void *p, size_t a) { return ((uintptr_t)p & (a - 1)) == 0; }

int main(void)
{
    void *p;

    p = malloc(64);
    printf("malloc(64)              = %p  16-aligned? %d  64-aligned? %d\n",
           p, aligned(p, 16), aligned(p, 64));
    free(p);

    p = aligned_alloc(64, 128);
    printf("aligned_alloc(64, 128)  = %p  64-aligned? %d\n",
           p, aligned(p, 64));
    free(p);

    if (posix_memalign(&p, 4096, 4096) == 0) {
        printf("posix_memalign(4096,4K) = %p  page-aligned? %d\n",
               p, aligned(p, 4096));
        free(p);
    }

    p = memalign(32, 100);
    printf("memalign(32, 100)       = %p  32-aligned? %d\n",
           p, aligned(p, 32));
    free(p);

    /* DEMO: invalid use of aligned_alloc -- alignment not power of 2,
       OR size not multiple of alignment.  Modern glibc returns NULL+EINVAL.*/
    p = aligned_alloc(64, 100);
    printf("aligned_alloc(64, 100)  = %p  (UB, may be NULL)\n", p);
    if (p) free(p);

    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/06_alignment.c
 * Command: make -C 04_heap_malloc 06_alignment
 * Exit status: 0
 * Output:
 * malloc(64)              = 0x56498d2122a0  16-aligned? 1  64-aligned? 0
 * aligned_alloc(64, 128)  = 0x56498d213300  64-aligned? 1
 * posix_memalign(4096,4K) = 0x56498d214000  page-aligned? 1
 * memalign(32, 100)       = 0x56498d2133c0  32-aligned? 1
 * aligned_alloc(64, 100)  = 0x56498d2133c0  (UB, may be NULL)
 * AUTO-GENERATED RUN OUTPUT END
 */
