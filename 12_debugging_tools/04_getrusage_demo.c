/*
 * 04_getrusage_demo.c
 * --------------------
 * Drive a small workload (malloc + memset of 256 MiB) and print before/after
 * rusage so you can see ru_maxrss climb and ru_minflt count up.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

static void show(const char *t)
{
    struct rusage r; getrusage(RUSAGE_SELF, &r);
    printf("[%-15s] maxrss=%-8ld KiB  minflt=%-8ld majflt=%ld   utime=%ld.%06lds\n",
           t, r.ru_maxrss, r.ru_minflt, r.ru_majflt,
           (long)r.ru_utime.tv_sec, (long)r.ru_utime.tv_usec);
}

int main(void)
{
    show("start");

    size_t n = 256UL << 20;
    char *p = malloc(n);
    memset(p, 1, n);
    show("after 256M");

    /* hot loop -- run up some utime */
    volatile long s = 0;
    for (long i = 0; i < (long)(n / 64); ++i) s += p[i * 64];
    show("after sweep");
    (void)s;

    free(p);
    show("after free");
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 12_debugging_tools/04_getrusage_demo.c
 * Command: make -C 12_debugging_tools 04_getrusage_demo
 * Exit status: 0
 * Output:
 * [start          ] maxrss=11456    KiB  minflt=51       majflt=0   utime=0.000000s
 * [after 256M     ] maxrss=263636   KiB  minflt=188      majflt=0   utime=0.010731s
 * [after sweep    ] maxrss=263636   KiB  minflt=188      majflt=0   utime=0.022601s
 * [after free     ] maxrss=263636   KiB  minflt=188      majflt=0   utime=0.022610s
 * AUTO-GENERATED RUN OUTPUT END
 */
