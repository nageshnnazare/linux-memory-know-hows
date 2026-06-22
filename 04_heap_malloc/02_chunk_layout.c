/*
 * 02_chunk_layout.c
 * ------------------
 * Peek at the 16-byte ptmalloc header immediately BEFORE each malloc'd
 * pointer.  Layout on x86_64 glibc:
 *
 *   p - 16: prev_size  (8 B)  -- only valid if prev chunk is free
 *   p -  8: size | flags(3b)  -- low 3 bits = A M P
 *                                A = arena (1 = non-main arena)
 *                                M = mmap'd
 *                                P = previous chunk in use
 *   p     : payload (what malloc returned)
 *
 *   The reported "size" includes the 16-byte header and is rounded up to
 *   16 bytes.  malloc(1) -> chunk size 32.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static void show(const char *t, void *p)
{
    uint64_t *hdr = (uint64_t *)((char *)p - 16);
    uint64_t prev_size = hdr[0];
    uint64_t sz_flags  = hdr[1];
    uint64_t sz = sz_flags & ~0x7ULL;
    int A = (sz_flags >> 2) & 1;
    int M = (sz_flags >> 1) & 1;
    int P = (sz_flags >> 0) & 1;
    printf("%-25s p=%p  prev_size=%-6lu  chunk_size=%-6lu  A=%d M=%d P=%d\n",
           t, p,
           (unsigned long)prev_size,
           (unsigned long)sz,
           A, M, P);
}

int main(void)
{
    void *p1 = malloc(1);          show("malloc(1)",       p1);
    void *p2 = malloc(17);         show("malloc(17)",      p2);
    void *p3 = malloc(40);         show("malloc(40)",      p3);
    void *p4 = malloc(64);         show("malloc(64)",      p4);
    void *p5 = malloc(1000);       show("malloc(1000)",    p5);
    void *p6 = malloc(200 * 1024); show("malloc(200K)",    p6);   /* mmap path */

    free(p1); free(p2); free(p3); free(p4); free(p5); free(p6);
    return 0;
}
