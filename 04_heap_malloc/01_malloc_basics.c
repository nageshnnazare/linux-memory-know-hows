/*
 * 01_malloc_basics.c
 * -------------------
 * Show the basic malloc family in action.  Note the addresses: small
 * allocations cluster in one region (heap), really large ones live in their
 * own VMA (each is its own mmap).
 *
 *   p = malloc(N) ;   p points to N bytes of UNINITIALISED memory.
 *   q = calloc(n,s);  n*s bytes of ZEROED memory; checked for overflow.
 *   r = realloc(p,n); may return p; may return new ptr (old data copied).
 *   free(p)        ;  ok for p==NULL.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void hexdump(const char *t, const void *p, size_t n)
{
    const unsigned char *q = p;
    printf("%s @ %p : ", t, p);
    for (size_t i = 0; i < n; ++i) printf("%02x ", q[i]);
    putchar('\n');
}

int main(void)
{
    /* malloc -- contents undefined */
    char *a = malloc(8);
    hexdump("after malloc(8)        ", a, 8);

    /* calloc -- zeroed */
    char *b = calloc(8, 1);
    hexdump("after calloc(8,1)      ", b, 8);

    /* realloc -- maybe in place, maybe moves */
    a = realloc(a, 64);
    memset(a, 'a', 64);
    printf("after realloc(8->64)   @ %p\n", (void *)a);

    a = realloc(a, 1024 * 1024);   /* big -> probably mmap'd */
    printf("after realloc(64->1MiB)@ %p\n", (void *)a);

    /* free always returns memory to the allocator (not always to the OS) */
    free(a);
    free(b);

    /* free(NULL) is a no-op, perfectly legal */
    free(NULL);

    /* zero-size malloc is implementation-defined; glibc returns a valid
       non-NULL ptr that you must still free. */
    char *z = malloc(0);
    printf("malloc(0) = %p  (still must free!)\n", (void *)z);
    free(z);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/01_malloc_basics.c
 * Command: make -C 04_heap_malloc 01_malloc_basics
 * Exit status: 0
 * Output:
 * after malloc(8)         @ 0x556407df82a0 : 00 00 00 00 00 00 00 00 
 * after calloc(8,1)       @ 0x556407df92d0 : 00 00 00 00 00 00 00 00 
 * after realloc(8->64)   @ 0x556407df92f0
 * after realloc(64->1MiB)@ 0x7f0a260c8010
 * malloc(0) = 0x556407df92d0  (still must free!)
 * AUTO-GENERATED RUN OUTPUT END
 */
