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
