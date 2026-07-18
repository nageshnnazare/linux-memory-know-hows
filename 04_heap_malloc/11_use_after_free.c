/*
 * 11_use_after_free.c
 * --------------------
 * Read/write a chunk after free()ing it.  Behaviour:
 *
 *   p = malloc(32)
 *   free(p)               -> chunk pushed onto tcache; first 8 bytes hold
 *                            the "next" pointer of the freelist.
 *   *p = ...              -> we just clobbered the freelist link.
 *   q = malloc(32)        -> tcache pops p, returns it again -> q == p.
 *   r = malloc(32)        -> tcache uses the corrupted "next" -- could
 *                            return ANY pointer (a "tcache poisoning"
 *                            primitive for exploits).
 *
 * Build with ASan to see the real diagnostic:
 *   gcc -O0 -g -fsanitize=address 11_use_after_free.c -o 11_uaf
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(void)
{
    char *p = malloc(32);
    strcpy(p, "live");
    printf("alloc:        p=%p  data=\"%s\"\n", p, p);

    free(p);
    /* p now dangling; reading prints what tcache wrote there (the fd ptr) */
    printf("after free:   p=%p  data (first 8 hex): %02x%02x%02x%02x%02x%02x%02x%02x\n",
           p,
           (unsigned char)p[0], (unsigned char)p[1],
           (unsigned char)p[2], (unsigned char)p[3],
           (unsigned char)p[4], (unsigned char)p[5],
           (unsigned char)p[6], (unsigned char)p[7]);

    /* Overwrite the freelist pointer */
    *(uintptr_t *)p = 0xdeadbeef;

    char *q = malloc(32);
    char *r = malloc(32);
    printf("q=%p  r=%p   (note q==p, r could be anywhere)\n", q, r);
    /* don't dereference r -- likely UB */
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/11_use_after_free.c
 * Command: make -C 04_heap_malloc 11_use_after_free
 * Exit status: 0
 * Output:
 * alloc:        p=0x55f8a35942a0  data="live"
 * after free:   p=0x55f8a35942a0  data (first 8 hex): 94358a5f05000000
 * q=0x55f8a35942a0  r=0x55f8a35952e0   (note q==p, r could be anywhere)
 * AUTO-GENERATED RUN OUTPUT END
 */
