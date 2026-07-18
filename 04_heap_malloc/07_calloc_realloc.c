/*
 * 07_calloc_realloc.c
 * --------------------
 * calloc() vs malloc(); realloc() in place vs realloc() that moves.
 *
 *   calloc(n, s):
 *     - checks n*s for overflow (returns NULL + ENOMEM if it overflows)
 *     - zeroes the result (often "for free" because anon pages are zero)
 *
 *   realloc(p, n):
 *     - if the chunk happens to be big enough already, returns p unchanged.
 *     - else picks a bigger chunk, memcpys, frees the old one, returns new.
 *     - realloc(NULL, n) == malloc(n)
 *     - realloc(p, 0) was historically equivalent to free(p), returning NULL,
 *       but is UB in C23.  Don't.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *a = calloc(8, 1);
    printf("calloc(8,1) = %p  contents: %02x %02x %02x %02x...\n",
           (void *)a, a[0], a[1], a[2], a[3]);
    free(a);

    /* calloc detects overflow: */
    void *huge = calloc((size_t)-1 / 2, 4);
    printf("calloc(huge,4) = %p (NULL expected)\n", huge);

    /* realloc may return the same pointer */
    char *p = malloc(8);
    memcpy(p, "1234567", 8);
    printf("before realloc: p=%p \"%s\"\n", (void *)p, p);
    p = realloc(p, 16);
    printf("after realloc(16): p=%p \"%s\"\n", (void *)p, p);

    /* growing a lot usually moves it */
    p = realloc(p, 8 * 1024 * 1024);
    printf("after realloc(8 MiB): p=%p \"%s\"\n", (void *)p, p);
    free(p);

    /* realloc(NULL, n) is just malloc(n) */
    char *q = realloc(NULL, 32);
    printf("realloc(NULL,32) = %p\n", (void *)q);
    free(q);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/07_calloc_realloc.c
 * Command: make -C 04_heap_malloc 07_calloc_realloc
 * Exit status: 0
 * Output:
 * calloc(8,1) = 0x564f4e5252a0  contents: 00 00 00 00...
 * calloc(huge,4) = (nil) (NULL expected)
 * before realloc: p=0x564f4e5252a0 "1234567"
 * after realloc(16): p=0x564f4e5252a0 "1234567"
 * after realloc(8 MiB): p=0x7fbcc91ff010 "1234567"
 * realloc(NULL,32) = 0x564f4e5262d0
 * AUTO-GENERATED RUN OUTPUT END
 */
