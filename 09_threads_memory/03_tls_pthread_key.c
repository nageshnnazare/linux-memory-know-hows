/*
 * 03_tls_pthread_key.c
 * ---------------------
 * Pre-C11 TLS via pthread_key_create.  The advantage is a per-thread
 * destructor that runs at thread exit.
 *
 * Use compiler TLS unless you need the destructor mechanism (e.g. for
 * cleaning up a struct that owns heap or fds).
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>

static pthread_key_t key;

static void dtor(void *p)
{
    printf("[dtor] freeing %p in tid=%ld\n", p, syscall(SYS_gettid));
    free(p);
}

static void *worker(void *arg)
{
    char *buf = malloc(64);
    snprintf(buf, 64, "hello from %ld", (long)arg);
    pthread_setspecific(key, buf);
    /* later: */
    char *got = pthread_getspecific(key);
    printf("tid=%ld got=\"%s\" (%p)\n", syscall(SYS_gettid), got, (void *)got);
    return NULL;
}

int main(void)
{
    pthread_key_create(&key, dtor);
    pthread_t t[3];
    for (long i = 0; i < 3; ++i) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < 3; ++i) pthread_join(t[i], NULL);
    pthread_key_delete(key);
    return 0;
}
