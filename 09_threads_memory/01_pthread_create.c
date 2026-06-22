/*
 * 01_pthread_create.c
 * --------------------
 * Spawn a few threads and print the stack ranges so you can see that
 * each non-main thread has its own VMA in the mmap area.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

static void *worker(void *arg)
{
    int local;
    pthread_attr_t a;
    void *stk; size_t sz;
    pthread_getattr_np(pthread_self(), &a);
    pthread_attr_getstack(&a, &stk, &sz);
    pthread_attr_destroy(&a);
    printf("thread #%ld tid=%ld &local=%p stack=[%p..%p) (%zu KiB)\n",
           (long)arg, syscall(SYS_gettid), (void *)&local,
           stk, (char *)stk + sz, sz / 1024);
    return NULL;
}

int main(void)
{
    int local;
    printf("main      tid=%ld &local=%p\n",
           syscall(SYS_gettid), (void *)&local);

    pthread_t t[3];
    for (long i = 0; i < 3; ++i) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < 3; ++i) pthread_join(t[i], NULL);
    return 0;
}
