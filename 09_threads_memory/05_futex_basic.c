/*
 * 05_futex_basic.c
 * -----------------
 * Build a tiny mutex on top of SYS_futex.  Two threads each increment a
 * shared counter 100K times.
 *
 *   futex word states:
 *     0 = unlocked
 *     1 = locked, no waiters
 *     2 = locked, contended (someone in FUTEX_WAIT)
 *
 *   uncontested lock/unlock = a single atomic CAS + maybe atomic_exchange.
 *   contested lock           = FUTEX_WAIT syscall.
 *   contested unlock         = FUTEX_WAKE syscall.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <unistd.h>

static int futex_wait(_Atomic int *uaddr, int val)
{ return syscall(SYS_futex, uaddr, FUTEX_WAIT_PRIVATE, val, NULL, NULL, 0); }
static int futex_wake(_Atomic int *uaddr, int n)
{ return syscall(SYS_futex, uaddr, FUTEX_WAKE_PRIVATE, n, NULL, NULL, 0); }

static _Atomic int m = 0;

static void lock(void)
{
    int e = 0;
    if (atomic_compare_exchange_strong_explicit(
            &m, &e, 1, memory_order_acquire, memory_order_acquire))
        return;
    while (atomic_exchange_explicit(&m, 2, memory_order_acquire) != 0)
        futex_wait(&m, 2);
}

static void unlock(void)
{
    if (atomic_exchange_explicit(&m, 0, memory_order_release) == 2)
        futex_wake(&m, 1);
}

static long shared;
#define N (100 * 1000)

static void *worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < N; ++i) {
        lock();
        shared++;
        unlock();
    }
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("shared=%ld expected=%ld\n", shared, (long)(2 * N));
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 09_threads_memory/05_futex_basic.c
 * Command: make -C 09_threads_memory 05_futex_basic
 * Exit status: 0
 * Output:
 * shared=200000 expected=200000
 * AUTO-GENERATED RUN OUTPUT END
 */
