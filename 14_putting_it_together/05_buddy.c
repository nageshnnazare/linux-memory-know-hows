/*
 * 05_buddy.c
 * -----------
 * Power-of-2 buddy allocator over a single mmap'd region of size 2^MAX_ORDER
 * pages.  Pages are 4 KiB.
 *
 *   free lists: an array of length MAX_ORDER+1; each holds linked free blocks
 *   of size (1<<order) * PG.
 *
 *   alloc(n):
 *     compute order o = ceil(log2(n / PG))
 *     while free[o] empty and o < MAX_ORDER: o++
 *     pop block from free[o]; while o > requested: split, push half on free[--o]
 *     return block
 *
 *   free(p, order):
 *     buddy = p ^ (1 << order * PG_LOG2)        (bit flip)
 *     if buddy free at same order: merge, recurse
 *     else: push p on free[order]
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#define PG_LOG2 12        /* 4 KiB pages */
#define MIN_PG  1
#define MAX_ORDER 8       /* up to 2^8 pages = 1 MiB per allocation */
#define TOTAL_PAGES (1 << MAX_ORDER)
#define ARENA_BYTES ((size_t)TOTAL_PAGES << PG_LOG2)

typedef struct Block { struct Block *next; int order; } Block;

static char  *base;
static Block *fl[MAX_ORDER + 1];

static void fl_push(int o, Block *b) { b->order = o; b->next = fl[o]; fl[o] = b; }
static Block *fl_pop(int o) { Block *b = fl[o]; if (b) fl[o] = b->next; return b; }
static int   fl_remove(int o, Block *target) {
    Block **p = &fl[o];
    while (*p) {
        if (*p == target) { *p = (*p)->next; return 1; }
        p = &(*p)->next;
    }
    return 0;
}

void buddy_init(void)
{
    base = mmap(NULL, ARENA_BYTES, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    fl_push(MAX_ORDER, (Block *)base);
}

void *buddy_alloc(int pages)
{
    int order = 0; int sz = 1;
    while (sz < pages) { sz <<= 1; order++; }
    if (order > MAX_ORDER) return NULL;

    int o = order;
    while (o <= MAX_ORDER && !fl[o]) o++;
    if (o > MAX_ORDER) return NULL;

    Block *b = fl_pop(o);
    while (o > order) {
        size_t half = (size_t)((1 << (o - 1)) << PG_LOG2);
        Block *buddy = (Block *)((char *)b + half);
        --o;
        fl_push(o, buddy);
    }
    b->order = order;
    return b;
}

void buddy_free(void *p, int pages)
{
    int order = 0; int sz = 1;
    while (sz < pages) { sz <<= 1; order++; }

    while (order < MAX_ORDER) {
        uintptr_t off    = (char *)p - base;
        uintptr_t bsz    = (uintptr_t)((1UL << order) << PG_LOG2);
        uintptr_t buddyo = off ^ bsz;
        Block *buddy     = (Block *)(base + buddyo);
        if (!fl_remove(order, buddy)) break;
        if (buddy < (Block *)p) p = buddy;
        order++;
    }
    fl_push(order, (Block *)p);
}

static void dump(void)
{
    printf("free lists:");
    for (int o = 0; o <= MAX_ORDER; ++o) {
        int n = 0;
        for (Block *b = fl[o]; b; b = b->next) n++;
        printf("  order=%d:%d", o, n);
    }
    puts("");
}

int main(void)
{
    buddy_init();
    dump();

    void *a = buddy_alloc(3);   printf("alloc 3 pages -> %p\n", a); dump();
    void *b = buddy_alloc(1);   printf("alloc 1 page  -> %p\n", b); dump();
    void *c = buddy_alloc(8);   printf("alloc 8 pages -> %p\n", c); dump();

    buddy_free(a, 3);           puts("free a"); dump();
    buddy_free(b, 1);           puts("free b"); dump();
    buddy_free(c, 8);           puts("free c"); dump();
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 14_putting_it_together/05_buddy.c
 * Command: make -C 14_putting_it_together 05_buddy
 * Exit status: 0
 * Output:
 * free lists:  order=0:0  order=1:0  order=2:0  order=3:0  order=4:0  order=5:0  order=6:0  order=7:0  order=8:1
 * alloc 3 pages -> 0x7fc22b066000
 * free lists:  order=0:0  order=1:0  order=2:1  order=3:1  order=4:1  order=5:1  order=6:1  order=7:1  order=8:0
 * alloc 1 page  -> 0x7fc22b06a000
 * free lists:  order=0:1  order=1:1  order=2:0  order=3:1  order=4:1  order=5:1  order=6:1  order=7:1  order=8:0
 * alloc 8 pages -> 0x7fc22b06e000
 * free lists:  order=0:1  order=1:1  order=2:0  order=3:0  order=4:1  order=5:1  order=6:1  order=7:1  order=8:0
 * free a
 * free lists:  order=0:1  order=1:1  order=2:1  order=3:0  order=4:1  order=5:1  order=6:1  order=7:1  order=8:0
 * free b
 * free lists:  order=0:0  order=1:0  order=2:0  order=3:1  order=4:1  order=5:1  order=6:1  order=7:1  order=8:0
 * free c
 * free lists:  order=0:0  order=1:0  order=2:0  order=3:0  order=4:0  order=5:0  order=6:0  order=7:0  order=8:1
 * AUTO-GENERATED RUN OUTPUT END
 */
