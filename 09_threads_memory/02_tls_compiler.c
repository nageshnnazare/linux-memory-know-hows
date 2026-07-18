/*
 * 02_tls_compiler.c
 * ------------------
 * Compiler-supported TLS: each thread gets its own instance.
 *
 *   __thread int x;          // GCC extension
 *   _Thread_local int y;     // C11
 *
 * Access is one instruction on x86_64:    mov %fs:offset, %eax
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

static _Thread_local int counter;   // per-thread, init = 0

static void *worker(void *arg)
{
    long n = (long)arg;
    for (int i = 0; i < n; ++i) counter++;
    printf("tid=%ld counter=%d (&counter=%p)\n",
           syscall(SYS_gettid), counter, (void *)&counter);
    return NULL;
}

int main(void)
{
    counter = 100;
    pthread_t t[3];
    long ns[] = { 10, 20, 30 };
    for (int i = 0; i < 3; ++i) pthread_create(&t[i], NULL, worker, (void *)ns[i]);
    for (int i = 0; i < 3; ++i) pthread_join(t[i], NULL);

    /* main thread's counter is still 100 */
    printf("main counter=%d\n", counter);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 09_threads_memory/02_tls_compiler.c
 * Command: make -C 09_threads_memory 02_tls_compiler
 * Exit status: 0
 * Output:
 * tid=3740 counter=10 (&counter=0x7fc37b1ff6bc)
 * tid=3741 counter=20 (&counter=0x7fc37a1fe6bc)
 * tid=3742 counter=30 (&counter=0x7fc370fff6bc)
 * main counter=100
 * AUTO-GENERATED RUN OUTPUT END
 */
