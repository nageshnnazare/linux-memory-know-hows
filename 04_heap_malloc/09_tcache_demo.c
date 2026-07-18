/*
 * 09_tcache_demo.c
 * -----------------
 * tcache (glibc 2.26+) is a per-thread LIFO freelist for small chunks.
 * That means malloc/free repeatedly of the same size returns the SAME
 * pointer until the bin is exhausted or coalesced.
 *
 *   Step 1: alloc a, b, c (all size 32)
 *   Step 2: free a, b, c
 *           tcache for 32B bin now:    c -> b -> a -> NULL (LIFO)
 *   Step 3: alloc x, y, z
 *           expect:  x == c, y == b, z == a
 */
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    void *a = malloc(24);
    void *b = malloc(24);
    void *c = malloc(24);
    printf("alloc:  a=%p  b=%p  c=%p\n", a, b, c);

    free(a); free(b); free(c);
    printf("free order: a then b then c\n");

    void *x = malloc(24);
    void *y = malloc(24);
    void *z = malloc(24);
    printf("realloc: x=%p  y=%p  z=%p\n", x, y, z);
    printf("expect:  x==c (%d), y==b (%d), z==a (%d)\n",
           x == c, y == b, z == a);

    free(x); free(y); free(z);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/09_tcache_demo.c
 * Command: make -C 04_heap_malloc 09_tcache_demo
 * Exit status: 0
 * Output:
 * alloc:  a=0x55e7370c72a0  b=0x55e7370c72c0  c=0x55e7370c72e0
 * free order: a then b then c
 * realloc: x=0x55e7370c72e0  y=0x55e7370c72c0  z=0x55e7370c72a0
 * expect:  x==c (1), y==b (1), z==a (1)
 * AUTO-GENERATED RUN OUTPUT END
 */
