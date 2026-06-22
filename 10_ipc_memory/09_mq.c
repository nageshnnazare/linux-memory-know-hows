/*
 * 09_mq.c
 * --------
 * POSIX message queue with priority.
 *
 *   /dev/mqueue/<name> shows up after mq_open(O_CREAT).
 *   Each message is delivered in (priority, time) order.
 *
 * Link with -lrt
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

int main(void)
{
    const char *name = "/mem_guide_mq";
    struct mq_attr a = { .mq_flags = 0, .mq_maxmsg = 8, .mq_msgsize = 64, .mq_curmsgs = 0 };
    mq_unlink(name);
    mqd_t mq = mq_open(name, O_CREAT | O_RDWR, 0600, &a);
    if (mq == (mqd_t)-1) { perror("mq_open"); return 1; }

    pid_t pid = fork();
    if (pid == 0) {
        char buf[64]; unsigned prio;
        for (int i = 0; i < 3; ++i) {
            ssize_t n = mq_receive(mq, buf, sizeof buf, &prio);
            printf("[child] got prio=%u msg=\"%.*s\"\n", prio, (int)n, buf);
        }
        mq_close(mq);
        _exit(0);
    }
    /* parent sends in mixed priorities; receiver gets high prio first */
    mq_send(mq, "low",   3, 0);
    mq_send(mq, "high",  4, 10);
    mq_send(mq, "mid",   3, 5);

    waitpid(pid, NULL, 0);
    mq_close(mq);
    mq_unlink(name);
    return 0;
}
