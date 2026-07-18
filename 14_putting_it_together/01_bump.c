/*
 * 01_bump.c
 * ----------
 * Bump (arena) allocator.  Backing: a single mmap'd region.
 *
 *   arena layout:
 *     +-------------+
 *     | used        | <-- bump pointer
 *     +-------------+
 *     | free        |
 *     +-------------+
 *
 *   alloc(sz):     return bump; bump += round_up(sz, ALIGN)
 *   reset()  :     bump = base
 *   destroy() :    munmap(base, capacity)
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>

#define ALIGN 16
#define ALIGN_UP(x) (((x) + (ALIGN - 1)) & ~(ALIGN - 1))

typedef struct {
    char *base;
    char *bump;
    char *end;
} Arena;

static int arena_init(Arena *a, size_t cap)
{
    a->base = mmap(NULL, cap, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (a->base == MAP_FAILED) return -1;
    a->bump = a->base;
    a->end  = a->base + cap;
    return 0;
}

static void *arena_alloc(Arena *a, size_t sz)
{
    sz = ALIGN_UP(sz);
    if (a->bump + sz > a->end) return NULL;
    void *p = a->bump;
    a->bump += sz;
    return p;
}

static void arena_reset(Arena *a)   { a->bump = a->base; }
static void arena_destroy(Arena *a) { munmap(a->base, a->end - a->base); }

/* demo */
int main(void)
{
    Arena a;
    arena_init(&a, 4 * 1024 * 1024);

    char *s1 = arena_alloc(&a, 13);    strcpy(s1, "hello world!");
    char *s2 = arena_alloc(&a, 100);   memset(s2, 'B', 100); s2[99] = 0;
    int  *ai = arena_alloc(&a, 1000 * sizeof(int));
    for (int i = 0; i < 1000; ++i) ai[i] = i;

    printf("s1=%p \"%s\"\n", s1, s1);
    printf("s2=%p (%c..%c)\n", s2, s2[0], s2[98]);
    printf("ai[999]=%d  used=%ld bytes\n", ai[999],
           (long)(a.bump - a.base));

    arena_reset(&a);
    printf("after reset: used=%ld\n", (long)(a.bump - a.base));

    arena_destroy(&a);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 14_putting_it_together/01_bump.c
 * Command: make -C 14_putting_it_together 01_bump
 * Exit status: 0
 * Output:
 * s1=0x7f37de400000 "hello world!"
 * s2=0x7f37de400010 (B..B)
 * ai[999]=999  used=4128 bytes
 * after reset: used=0
 * AUTO-GENERATED RUN OUTPUT END
 */
