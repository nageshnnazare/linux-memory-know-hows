/*
 * 07_eventfd.c
 * -------------
 * eventfd is a counter you can read/write/poll.  Common pattern:
 *
 *   - server thread: epoll_wait on the eventfd
 *   - producer thread: write(efd, &(uint64_t){1}, 8) when work is ready
 *   - server: read returns the count, resets to 0; can poll for more.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <stdint.h>

static int efd;

static void *worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < 5; ++i) {
        uint64_t one = 1;
        write(efd, &one, sizeof one);
        usleep(100000);
    }
    return NULL;
}

int main(void)
{
    efd = eventfd(0, EFD_CLOEXEC);
    pthread_t t; pthread_create(&t, NULL, worker, NULL);

    uint64_t v;
    for (int i = 0; i < 5; ++i) {
        read(efd, &v, sizeof v);
        printf("woke up: got count %lu\n", (unsigned long)v);
    }
    pthread_join(t, NULL);
    close(efd);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 10_ipc_memory/07_eventfd.c
 * Command: make -C 10_ipc_memory 07_eventfd
 * Exit status: 0
 * Output:
 * woke up: got count 1
 * woke up: got count 1
 * woke up: got count 1
 * woke up: got count 1
 * woke up: got count 1
 * AUTO-GENERATED RUN OUTPUT END
 */
