/*
 * 03_slab.c
 * ----------
 * Bonwick-style slab allocator.
 *
 *   A "cache" stores objects of a fixed size, organised into "slabs"
 *   (regions carved into N objects + a small header).
 *
 *   cache:
 *     +-- list of all slabs -----------------------+
 *     | slab1 (some used)  -> slab2 (some used)... |
 *     +--------------------------------------------+
 *     partial = head of slabs with at least one free object
 *
 *   slab:
 *     +-----------------------+
 *     | slab header (next,    |   linked in cache->all_slabs
 *     | in_use, capacity,     |   and (when partial) cache->partial
 *     | free freelist head)   |
 *     +-----------------------+
 *     | obj 0                 |   each object is OBJ_SZ bytes
 *     +-----------------------+
 *     | obj 1                 |   when free, reuses its bytes for a
 *     +-----------------------+   FreeNode->next pointer.
 *     | ... obj N             |
 *     +-----------------------+
 *
 *   alloc:
 *     - if no partial slab, create one (mmap SLAB_BYTES + init freelist).
 *     - pop a FreeNode from its freelist.
 *     - if the slab is now fully used, unlink it from cache->partial.
 *
 *   free(p):
 *     - find the owning slab by searching cache->all_slabs
 *       (in production: store slab pointer at top of object or use page
 *        alignment magic; here we keep it simple).
 *     - push p onto the slab's freelist.
 *     - if slab was full, relink into cache->partial.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define SLAB_BYTES (32 * 1024)
#define ALIGN 16

typedef struct FreeNode { struct FreeNode *next; } FreeNode;
typedef struct Slab {
    struct Slab *next_all;       /* in cache->all_slabs */
    struct Slab *next_partial;   /* in cache->partial */
    FreeNode    *free;
    int          in_use;
    int          capacity;
    char        *base;           /* first object */
    char        *end;            /* one past last object */
} Slab;

typedef struct {
    Slab  *all_slabs;
    Slab  *partial;
    size_t obj_sz;
    int    objs_per_slab;
} Cache;

static void cache_init(Cache *c, size_t obj_sz)
{
    if (obj_sz < sizeof(FreeNode)) obj_sz = sizeof(FreeNode);
    obj_sz = (obj_sz + (ALIGN - 1)) & ~(ALIGN - 1);
    c->obj_sz = obj_sz;
    c->all_slabs = NULL;
    c->partial   = NULL;
    int header   = (int)((sizeof(Slab) + (ALIGN - 1)) & ~(ALIGN - 1));
    c->objs_per_slab = (SLAB_BYTES - header) / obj_sz;
}

static Slab *new_slab(Cache *c)
{
    void *p = mmap(NULL, SLAB_BYTES, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) return NULL;
    Slab *s = p;
    int header = (int)((sizeof(Slab) + (ALIGN - 1)) & ~(ALIGN - 1));
    s->base = (char *)p + header;
    s->end  = s->base + c->objs_per_slab * c->obj_sz;
    s->in_use = 0;
    s->capacity = c->objs_per_slab;
    s->free = NULL;
    for (int i = 0; i < c->objs_per_slab; ++i) {
        FreeNode *n = (FreeNode *)(s->base + i * c->obj_sz);
        n->next = s->free;
        s->free = n;
    }
    s->next_all = c->all_slabs;     c->all_slabs = s;
    s->next_partial = c->partial;   c->partial   = s;
    return s;
}

static void *cache_alloc(Cache *c)
{
    if (!c->partial && !new_slab(c)) return NULL;
    Slab *s = c->partial;
    FreeNode *n = s->free;
    s->free = n->next;
    s->in_use++;
    if (!s->free) c->partial = s->next_partial;   /* slab is full */
    return n;
}

static void cache_free(Cache *c, void *p)
{
    Slab *s = NULL;
    for (Slab *it = c->all_slabs; it; it = it->next_all) {
        if ((char *)p >= it->base && (char *)p < it->end) { s = it; break; }
    }
    if (!s) { fprintf(stderr, "cache_free: pointer %p not from us!\n", p); return; }

    int was_full = (s->free == NULL);
    FreeNode *n = p;
    n->next = s->free;
    s->free = n;
    s->in_use--;
    if (was_full) {
        s->next_partial = c->partial;
        c->partial      = s;
    }
}

typedef struct { int id; char name[60]; } Obj;

int main(void)
{
    Cache cc;
    cache_init(&cc, sizeof(Obj));
    printf("slab=%d KiB, obj=%zu B, objs/slab=%d\n",
           SLAB_BYTES >> 10, cc.obj_sz, cc.objs_per_slab);

    enum { N = 10000 };
    Obj *p[N];
    for (int i = 0; i < N; ++i) p[i] = cache_alloc(&cc);
    for (int i = 0; i < N; ++i) p[i]->id = i;
    int sum = 0; for (int i = 0; i < N; ++i) sum += p[i]->id;
    for (int i = 0; i < N; ++i) cache_free(&cc, p[i]);
    printf("alloc/free %d Obj OK; sum of ids = %d (expected %d)\n",
           N, sum, (N * (N - 1)) / 2);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 14_putting_it_together/03_slab.c
 * Command: make -C 14_putting_it_together 03_slab
 * Exit status: 0
 * Output:
 * slab=32 KiB, obj=64 B, objs/slab=511
 * alloc/free 10000 Obj OK; sum of ids = 49995000 (expected 49995000)
 * AUTO-GENERATED RUN OUTPUT END
 */
