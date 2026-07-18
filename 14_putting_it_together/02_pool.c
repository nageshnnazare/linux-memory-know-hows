/*
 * 02_pool.c
 * ----------
 * Fixed-size pool / object freelist.  Each slot is exactly OBJ_SZ bytes.
 *
 *   When a slot is FREE we re-use the slot itself to store the freelist
 *   "next" pointer.  This gives us zero per-object metadata.
 *
 *   alloc:    if (head) p = head; head = head->next; return p
 *   free(p):  p->next = head; head = p
 *   both O(1).
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

typedef struct Node { struct Node *next; } Node;

typedef struct {
    char *base;
    size_t obj_sz;
    size_t cap;
    Node *free_head;
} Pool;

static int pool_init(Pool *p, size_t obj_sz, size_t n)
{
    if (obj_sz < sizeof(Node)) obj_sz = sizeof(Node);
    p->obj_sz = obj_sz;
    p->cap    = n;
    p->base = mmap(NULL, obj_sz * n, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p->base == MAP_FAILED) return -1;

    /* thread the freelist through all slots */
    p->free_head = NULL;
    for (size_t i = 0; i < n; ++i) {
        Node *node = (Node *)(p->base + i * obj_sz);
        node->next = p->free_head;
        p->free_head = node;
    }
    return 0;
}

static void *pool_alloc(Pool *p)
{
    if (!p->free_head) return NULL;
    Node *n = p->free_head;
    p->free_head = n->next;
    return n;
}

static void pool_free(Pool *p, void *q)
{
    Node *n = q;
    n->next = p->free_head;
    p->free_head = n;
}

static void pool_destroy(Pool *p) { munmap(p->base, p->obj_sz * p->cap); }

/* demo */
typedef struct { int id; char name[28]; } Obj;

int main(void)
{
    Pool p;
    pool_init(&p, sizeof(Obj), 1024);

    Obj *a = pool_alloc(&p);
    Obj *b = pool_alloc(&p);
    a->id = 1; snprintf(a->name, sizeof a->name, "alpha");
    b->id = 2; snprintf(b->name, sizeof b->name, "beta");
    printf("a=%p {%d, %s}\n", a, a->id, a->name);
    printf("b=%p {%d, %s}\n", b, b->id, b->name);

    pool_free(&p, a);
    Obj *c = pool_alloc(&p);
    printf("after free(a) + alloc -> c=%p (%s)\n", c, c == a ? "got a back" : "different");

    pool_free(&p, b);
    pool_free(&p, c);
    pool_destroy(&p);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 14_putting_it_together/02_pool.c
 * Command: make -C 14_putting_it_together 02_pool
 * Exit status: 0
 * Output:
 * a=0x7ff31fdaefe0 {1, alpha}
 * b=0x7ff31fdaefc0 {2, beta}
 * after free(a) + alloc -> c=0x7ff31fdaefe0 (got a back)
 * AUTO-GENERATED RUN OUTPUT END
 */
