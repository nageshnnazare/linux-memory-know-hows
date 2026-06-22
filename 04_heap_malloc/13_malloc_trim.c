/*
 * 13_malloc_trim.c
 * -----------------
 * Allocate, free, then call malloc_trim() to hand top-of-heap back to the
 * kernel.  Watch VmRSS drop.
 *
 *   Without trim, glibc holds the heap so future malloc()s are fast.
 *   For long-lived servers that have spikes followed by quiet periods,
 *   calling malloc_trim(0) after a spike reduces RSS substantially.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

static void show(const char *t)
{
    FILE *f = fopen("/proc/self/status", "r");
    char line[256];
    long vm = 0, rss = 0;
    while (fgets(line, sizeof line, f)) {
        if (!strncmp(line, "VmSize:", 7)) sscanf(line + 7, "%ld", &vm);
        if (!strncmp(line, "VmRSS:",  6)) sscanf(line + 6, "%ld", &rss);
    }
    fclose(f);
    printf("[%-18s] VmSize=%ld KiB  VmRSS=%ld KiB\n", t, vm, rss);
}

int main(void)
{
    show("start");

    enum { N = 200 };
    void *p[N];
    for (int i = 0; i < N; ++i) p[i] = malloc(64 * 1024);  /* keep some big ones */
    for (int i = 0; i < N; ++i) memset(p[i], 1, 64 * 1024);
    show("after 200x64K");

    for (int i = 0; i < N; ++i) free(p[i]);
    show("after free");

    malloc_trim(0);
    show("after malloc_trim");

    return 0;
}
