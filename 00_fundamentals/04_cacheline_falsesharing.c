/*
 * 04_cacheline_falsesharing.c
 * ----------------------------
 * Two threads each increment their own counter.  When the counters share
 * the same 64-byte cache line the program is dramatically slower because
 * the line ping-pongs between the cores (MESI: I -> S -> M back and forth).
 *
 *   bad layout (one cache line, 64B):
 *     +--------------------------------+
 *     | counter A | counter B |  ...   |
 *     +--------------------------------+
 *     ^^ both writers fight for this line
 *
 *   good layout (two cache lines, 128B):
 *     +-----------------+ +-----------------+
 *     | counter A | pad | | counter B | pad |
 *     +-----------------+ +-----------------+
 *      core 0 owns         core 1 owns
 *
 * Build:  gcc -O2 -pthread 04_cacheline_falsesharing.c -o 04_cl
 */

#define _POSIX_C_SOURCE 199309L
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define ITERS (200ULL * 1000 * 1000)

/* ----- bad: counters share a line ----- */
struct shared_bad { uint64_t a; uint64_t b; };

/* ----- good: each counter on its own cache line ----- */
struct shared_good {
    _Alignas(64) uint64_t a;
    _Alignas(64) uint64_t b;
};

static void *worker(void *arg)
{
    volatile uint64_t *p = arg;
    for (uint64_t i = 0; i < ITERS; ++i) (*p)++;
    return NULL;
}

static double now_sec(void)
{
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static double run(volatile uint64_t *pa, volatile uint64_t *pb)
{
    pthread_t t1, t2;
    double t0 = now_sec();
    pthread_create(&t1, NULL, worker, (void *)pa);
    pthread_create(&t2, NULL, worker, (void *)pb);
    pthread_join(t1, NULL); pthread_join(t2, NULL);
    return now_sec() - t0;
}

int main(void)
{
    /* aligned_alloc(a, sz) requires sz be a multiple of a; round up. */
    size_t bsz = (sizeof(struct shared_bad)  + 63) & ~63ULL;
    size_t gsz = (sizeof(struct shared_good) + 63) & ~63ULL;
    struct shared_bad  *b = aligned_alloc(64, bsz); b->a = b->b = 0;
    struct shared_good *g = aligned_alloc(64, gsz); g->a = g->b = 0;

    printf("FALSE-SHARING bench, %llu iters per thread\n", ITERS);
    printf("  bad layout  (counters in same cache line) : %.3f s\n",
           run(&b->a, &b->b));
    printf("  good layout (counters in different lines) : %.3f s\n",
           run(&g->a, &g->b));

    free(b); free(g);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 00_fundamentals/04_cacheline_falsesharing.c
 * Command: make -C 00_fundamentals 04_cacheline_falsesharing
 * Exit status: 0
 * Output:
 * FALSE-SHARING bench, 200000000 iters per thread
 *   bad layout  (counters in same cache line) : 0.317 s
 *   good layout (counters in different lines) : 0.071 s
 * AUTO-GENERATED RUN OUTPUT END
 */
