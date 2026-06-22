/*
 * 05_rlimit.c
 * ------------
 * Print all the memory-related rlimits and explain what each one means.
 *
 *   RLIMIT_AS      = max virtual address space (VSZ)         <- can be unlimited
 *   RLIMIT_DATA    = max heap + bss + data segments
 *   RLIMIT_STACK   = max main-thread stack size              <- typical 8 MiB
 *   RLIMIT_RSS     = max resident set size (legacy, ignored on modern kernels)
 *   RLIMIT_MEMLOCK = max bytes you can mlock without privilege
 *   RLIMIT_NOFILE  = max number of open file descriptors
 *
 *  Inspect & change with:   ulimit -a
 *                           ulimit -s 16384
 *                           prlimit --pid=$$ --stack=64M:64M
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/resource.h>

struct entry { int id; const char *name; const char *unit; };

static struct entry rls[] = {
    { RLIMIT_AS,      "RLIMIT_AS",      "bytes" },
    { RLIMIT_CORE,    "RLIMIT_CORE",    "bytes" },
    { RLIMIT_CPU,     "RLIMIT_CPU",     "sec" },
    { RLIMIT_DATA,    "RLIMIT_DATA",    "bytes" },
    { RLIMIT_FSIZE,   "RLIMIT_FSIZE",   "bytes" },
    { RLIMIT_MEMLOCK, "RLIMIT_MEMLOCK", "bytes" },
    { RLIMIT_NOFILE,  "RLIMIT_NOFILE",  "fds" },
    { RLIMIT_NPROC,   "RLIMIT_NPROC",   "procs" },
    { RLIMIT_RSS,     "RLIMIT_RSS",     "bytes (legacy)" },
    { RLIMIT_STACK,   "RLIMIT_STACK",   "bytes" },
#ifdef RLIMIT_LOCKS
    { RLIMIT_LOCKS,   "RLIMIT_LOCKS",   "count" },
#endif
#ifdef RLIMIT_MSGQUEUE
    { RLIMIT_MSGQUEUE,"RLIMIT_MSGQUEUE","bytes" },
#endif
#ifdef RLIMIT_NICE
    { RLIMIT_NICE,    "RLIMIT_NICE",    "0..40" },
#endif
#ifdef RLIMIT_RTPRIO
    { RLIMIT_RTPRIO,  "RLIMIT_RTPRIO",  "0..99" },
#endif
#ifdef RLIMIT_RTTIME
    { RLIMIT_RTTIME,  "RLIMIT_RTTIME",  "us" },
#endif
#ifdef RLIMIT_SIGPENDING
    { RLIMIT_SIGPENDING,"RLIMIT_SIGPENDING","count" },
#endif
};

static void print_one(const struct entry *e)
{
    struct rlimit r;
    if (getrlimit(e->id, &r) < 0) { perror("getrlimit"); return; }
    char cur[32], max[32];
    if (r.rlim_cur == RLIM_INFINITY) snprintf(cur, sizeof cur, "unlimited");
    else snprintf(cur, sizeof cur, "%lu", (unsigned long)r.rlim_cur);
    if (r.rlim_max == RLIM_INFINITY) snprintf(max, sizeof max, "unlimited");
    else snprintf(max, sizeof max, "%lu", (unsigned long)r.rlim_max);
    printf("  %-18s  soft=%-15s hard=%-15s  (%s)\n",
           e->name, cur, max, e->unit);
}

int main(void)
{
    printf("=== getrlimit() snapshot ===\n");
    for (size_t i = 0; i < sizeof rls / sizeof *rls; ++i) print_one(&rls[i]);
    return 0;
}
