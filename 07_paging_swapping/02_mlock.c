/*
 * 02_mlock.c
 * -----------
 * Pin pages with mlock(); inspect /proc/self/status to see VmLck increase.
 * Non-root processes are limited by RLIMIT_MEMLOCK (typically 64 KiB).
 *
 *   typical use:  protect cryptographic key material from swap-out.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>

static void show(const char *t)
{
    FILE *f = fopen("/proc/self/status", "r");
    char l[256]; long lck = 0, rss = 0;
    while (fgets(l, sizeof l, f)) {
        if (!strncmp(l, "VmLck:", 6)) sscanf(l + 6, "%ld", &lck);
        if (!strncmp(l, "VmRSS:", 6)) sscanf(l + 6, "%ld", &rss);
    }
    fclose(f);
    printf("[%-20s] VmLck=%ld KiB  VmRSS=%ld KiB\n", t, lck, rss);
}

int main(void)
{
    struct rlimit rl; getrlimit(RLIMIT_MEMLOCK, &rl);
    printf("RLIMIT_MEMLOCK soft=%lu hard=%lu (bytes)\n",
           (unsigned long)rl.rlim_cur, (unsigned long)rl.rlim_max);

    show("start");
    size_t n = 32 * 1024;       /* well within default 64 KiB limit */
    void *p = malloc(n);
    memset(p, 1, n);
    show("after malloc+touch");

    if (mlock(p, n) < 0) { perror("mlock"); free(p); return 1; }
    show("after mlock");

    munlock(p, n);
    show("after munlock");

    free(p);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 07_paging_swapping/02_mlock.c
 * Command: make -C 07_paging_swapping 02_mlock
 * Exit status: 0
 * Output:
 * RLIMIT_MEMLOCK soft=8388608 hard=8388608 (bytes)
 * [start               ] VmLck=0 KiB  VmRSS=1488 KiB
 * [after malloc+touch  ] VmLck=0 KiB  VmRSS=1648 KiB
 * [after mlock         ] VmLck=36 KiB  VmRSS=1648 KiB
 * [after munlock       ] VmLck=0 KiB  VmRSS=1648 KiB
 * AUTO-GENERATED RUN OUTPUT END
 */
