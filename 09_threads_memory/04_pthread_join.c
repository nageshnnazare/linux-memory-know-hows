/*
 * 04_pthread_join.c
 * ------------------
 * Spawn a thread that returns a heap-allocated value; main joins and
 * takes ownership.
 *
 *   pthread_create(&tid, attr, fn, arg)
 *   pthread_join (tid, &retval)
 *
 *   join blocks until the thread exits and writes its return into retval.
 *   Under the hood: glibc futex-waits on the tid word being CLEAR'd by the
 *   kernel via CLONE_CHILD_CLEARTID.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

static void *worker(void *arg)
{
    int n = *(int *)arg;
    int *result = malloc(sizeof *result);
    *result = n * n;
    usleep(100000);
    return result;
}

int main(void)
{
    int n = 7;
    pthread_t t;
    pthread_create(&t, NULL, worker, &n);

    void *r = NULL;
    pthread_join(t, &r);
    printf("worker(%d) returned %d\n", n, *(int *)r);
    free(r);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 09_threads_memory/04_pthread_join.c
 * Command: make -C 09_threads_memory 04_pthread_join
 * Exit status: 0
 * Output:
 * worker(7) returned 49
 * AUTO-GENERATED RUN OUTPUT END
 */
