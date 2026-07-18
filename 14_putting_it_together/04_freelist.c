/*
 * 04_freelist.c
 * --------------
 * A minimal variable-size first-fit allocator with coalescing.  ~200 lines.
 * Educational; nowhere near production quality.
 *
 *   Layout of a chunk:
 *     +------------+
 *     |  size  | F |   size in bytes (incl header), F = free flag in low bit
 *     +------------+
 *     |  payload   |   user pointer points here
 *     |  ...       |
 *     +------------+
 *
 *   We keep a SINGLE doubly-linked free-list (no size classes).
 *
 *   alloc(n):     walk list, find first chunk with usable >= n; split if leftover
 *                 is big enough; mark in-use; return payload
 *
 *   free(p):      compute header from p; mark free; coalesce with next neighbour
 *                 by inspecting the trailing chunk; coalesce with previous by
 *                 reading a footer.
 *
 *   We use boundary tags (size repeated as a footer) so coalescing is O(1).
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#define ALIGN 16
#define ALIGN_UP(x) (((x) + (ALIGN - 1)) & ~(ALIGN - 1))

typedef struct Hdr {
    size_t size_flags;  /* low bit = free */
    struct Hdr *prev_free, *next_free;
} Hdr;
typedef struct { size_t size_flags; } Ftr;

#define HDR_SZ ALIGN_UP(sizeof(Hdr))
#define FTR_SZ ALIGN_UP(sizeof(Ftr))
#define MIN_CHUNK (HDR_SZ + ALIGN + FTR_SZ)

static char *heap_base, *heap_end;
static Hdr  *free_head;

static size_t  csz(Hdr *h)   { return h->size_flags & ~1ULL; }
static int     cfree(Hdr *h) { return h->size_flags & 1; }
static Ftr    *ftr_of(Hdr *h){ return (Ftr *)((char *)h + csz(h) - FTR_SZ); }
static Hdr    *next_of(Hdr *h){ return (Hdr *)((char *)h + csz(h)); }
static Hdr    *prev_of(Hdr *h){
    if ((char *)h == heap_base) return NULL;
    Ftr *pf = (Ftr *)((char *)h - FTR_SZ);
    return (Hdr *)((char *)h - (pf->size_flags & ~1ULL));
}
static void    set_size(Hdr *h, size_t sz, int free){
    h->size_flags = sz | (free ? 1 : 0);
    ftr_of(h)->size_flags = h->size_flags;
}

static void fl_insert(Hdr *h)
{
    h->prev_free = NULL;
    h->next_free = free_head;
    if (free_head) free_head->prev_free = h;
    free_head = h;
}
static void fl_remove(Hdr *h)
{
    if (h->prev_free) h->prev_free->next_free = h->next_free;
    else              free_head             = h->next_free;
    if (h->next_free) h->next_free->prev_free = h->prev_free;
}

void my_init(size_t cap)
{
    cap = (cap + 4095) & ~4095;
    heap_base = mmap(NULL, cap, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    heap_end  = heap_base + cap;
    Hdr *whole = (Hdr *)heap_base;
    set_size(whole, cap, 1);
    free_head = NULL;
    fl_insert(whole);
}

void *my_malloc(size_t req)
{
    if (req == 0) return NULL;
    size_t need = ALIGN_UP(req + HDR_SZ + FTR_SZ);
    if (need < MIN_CHUNK) need = MIN_CHUNK;
    for (Hdr *h = free_head; h; h = h->next_free) {
        if (csz(h) >= need) {
            fl_remove(h);
            size_t leftover = csz(h) - need;
            if (leftover >= MIN_CHUNK) {
                set_size(h, need, 0);
                Hdr *r = next_of(h);
                set_size(r, leftover, 1);
                fl_insert(r);
            } else {
                set_size(h, csz(h), 0);
            }
            return (char *)h + HDR_SZ;
        }
    }
    return NULL;
}

void my_free(void *p)
{
    if (!p) return;
    Hdr *h = (Hdr *)((char *)p - HDR_SZ);
    set_size(h, csz(h), 1);

    /* coalesce with next */
    Hdr *n = next_of(h);
    if ((char *)n < heap_end && cfree(n)) {
        fl_remove(n);
        set_size(h, csz(h) + csz(n), 1);
    }
    /* coalesce with prev */
    Hdr *pr = prev_of(h);
    if (pr && cfree(pr)) {
        fl_remove(pr);
        set_size(pr, csz(pr) + csz(h), 1);
        h = pr;
    }
    fl_insert(h);
}

/* demo */
int main(void)
{
    my_init(1024 * 1024);

    char *a = my_malloc(50);  memset(a, 'A', 50);
    char *b = my_malloc(1000); memset(b, 'B', 1000);
    char *c = my_malloc(20);   memset(c, 'C', 20);
    printf("a=%p b=%p c=%p\n", a, b, c);

    my_free(b);
    char *d = my_malloc(900);
    printf("after free(b) + alloc(900): d=%p (%s)\n",
           d, d == b ? "same as b" : "different");

    my_free(a);
    my_free(c);
    my_free(d);

    /* after all freed, one big chunk again */
    char *huge = my_malloc(900000);
    printf("after free-all + alloc(900K): huge=%p\n", huge);
    my_free(huge);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 14_putting_it_together/04_freelist.c
 * Command: make -C 14_putting_it_together 04_freelist
 * Exit status: 0
 * Output:
 * a=0x7fb87bd00020 b=0x7fb87bd00090 c=0x7fb87bd004b0
 * after free(b) + alloc(900): d=0x7fb87bd00090 (same as b)
 * after free-all + alloc(900K): huge=0x7fb87bd00020
 * AUTO-GENERATED RUN OUTPUT END
 */
