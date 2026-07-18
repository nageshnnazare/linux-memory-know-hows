/*
 * 07_pthread_stack.c
 * -------------------
 * Show where a pthread's stack lives, what its size and guard size are,
 * and how to override them.
 *
 *   Process address space (high addr at top):
 *
 *   +-----------------+   main thread stack (~8 MiB)
 *   +-----------------+
 *   |   mmap area     |   --- thread 2 stack ---
 *   |                 |   --- guard page     ---
 *   |                 |   --- thread 1 stack ---
 *   |                 |   --- guard page     ---
 *   +-----------------+   heap (brk)
 *
 * Build:  gcc -O0 -g -pthread 07_pthread_stack.c -o 07_pthread_stack
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

static void *worker(void *arg)
{
    (void)arg;
    int local;

    pthread_attr_t a;
    pthread_getattr_np(pthread_self(), &a);
    void *stkaddr; size_t stksize, guard;
    pthread_attr_getstack(&a, &stkaddr, &stksize);
    pthread_attr_getguardsize(&a, &guard);

    printf("[thread %lu] &local       = %p\n",
           (unsigned long)pthread_self(), (void *)&local);
    printf("[thread %lu] stack base   = %p\n",
           (unsigned long)pthread_self(), stkaddr);
    printf("[thread %lu] stack top    = %p\n",
           (unsigned long)pthread_self(), (char *)stkaddr + stksize);
    printf("[thread %lu] stack size   = %zu KiB\n",
           (unsigned long)pthread_self(), stksize / 1024);
    printf("[thread %lu] guard size   = %zu bytes\n",
           (unsigned long)pthread_self(), guard);
    pthread_attr_destroy(&a);
    return NULL;
}

int main(void)
{
    /* Thread 1: default attributes */
    pthread_t t1; pthread_create(&t1, NULL, worker, NULL);
    pthread_join(t1, NULL);

    /* Thread 2: tiny custom stack (256 KiB) */
    pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setstacksize(&a, 256 * 1024);
    pthread_attr_setguardsize(&a, sysconf(_SC_PAGESIZE));
    pthread_t t2; pthread_create(&t2, &a, worker, NULL);
    pthread_join(t2, NULL);
    pthread_attr_destroy(&a);

    /* And the main thread for comparison */
    int local; printf("[ main ] &local         = %p\n", (void *)&local);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 03_stack/07_pthread_stack.c
 * Command: make -C 03_stack 07_pthread_stack
 * Exit status: 0
 * Output:
 * [thread 140285866669760] &local       = 0x7f96d93fee54
 * [thread 140285866669760] stack base   = 0x7f96d8400000
 * [thread 140285866669760] stack top    = 0x7f96d9400000
 * [thread 140285866669760] stack size   = 16384 KiB
 * [thread 140285866669760] guard size   = 4096 bytes
 * [thread 140285849888448] &local       = 0x7f96d83fde54
 * [thread 140285849888448] stack base   = 0x7f96d83bf000
 * [thread 140285849888448] stack top    = 0x7f96d83ff000
 * [thread 140285849888448] stack size   = 256 KiB
 * [thread 140285849888448] guard size   = 4096 bytes
 * [ main ] &local         = 0x7ffe602af62c
 * AUTO-GENERATED RUN OUTPUT END
 */
