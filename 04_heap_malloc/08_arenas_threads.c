/*
 * 08_arenas_threads.c
 * --------------------
 * Spawn N threads, each repeatedly malloc/free.  glibc will create up to
 * 8*ncpu thread arenas, each its own mmap'd region.
 *
 *   Watch /proc/self/maps -- you'll see multiple anon segments labelled
 *   "(deleted)" and large mmap regions (each arena starts at 64 MiB).
 *
 *   Tune with:  MALLOC_ARENA_MAX=1 ./08_arenas_threads
 */

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>

#define N_THREADS  8
#define ROUNDS     5000

static void *work(void *arg)
{
    (void)arg;
    void *buf[16];
    for (int r = 0; r < ROUNDS; ++r) {
        for (int i = 0; i < 16; ++i) buf[i] = malloc(256 + (i * 16));
        for (int i = 0; i < 16; ++i) free(buf[i]);
    }
    return NULL;
}

static void count_anon_vmas(const char *tag)
{
    FILE *f = fopen("/proc/self/maps", "r");
    char line[512];
    int anon = 0, file = 0;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "00:00 0")) anon++;
        else file++;
    }
    fclose(f);
    printf("[%s] anonymous VMAs=%d  file-backed VMAs=%d\n", tag, anon, file);
}

int main(void)
{
    count_anon_vmas("start");

    pthread_t t[N_THREADS];
    for (int i = 0; i < N_THREADS; ++i) pthread_create(&t[i], NULL, work, NULL);
    for (int i = 0; i < N_THREADS; ++i) pthread_join(t[i], NULL);

    count_anon_vmas("after threads");
    malloc_stats();   /* shows N arenas if more than one was used */
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/08_arenas_threads.c
 * Command: make -C 04_heap_malloc 08_arenas_threads
 * Exit status: 0
 * Output:
 * Arena 0:
 * system bytes     =     135168
 * in use bytes     =       8592
 * Arena 1:
 * system bytes     =     135168
 * in use bytes     =       2256
 * Arena 2:
 * system bytes     =     135168
 * in use bytes     =       2256
 * Arena 3:
 * system bytes     =     135168
 * in use bytes     =       2256
 * Arena 4:
 * system bytes     =     135168
 * in use bytes     =       2256
 * Arena 5:
 * system bytes     =     135168
 * in use bytes     =       2256
 * Arena 6:
 * system bytes     =     135168
 * in use bytes     =       2256
 * Total (incl. mmap):
 * system bytes     =     946176
 * in use bytes     =      22128
 * max mmap regions =          0
 * max mmap bytes   =          0
 * [start] anonymous VMAs=9  file-backed VMAs=15
 * [after threads] anonymous VMAs=25  file-backed VMAs=15
 * AUTO-GENERATED RUN OUTPUT END
 */
