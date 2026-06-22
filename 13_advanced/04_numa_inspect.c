/*
 * 04_numa_inspect.c
 * ------------------
 * Cat /proc/self/numa_maps to see which NUMA node each VMA's pages are
 * allocated on, plus the policy in effect.
 */
#include <stdio.h>

int main(void)
{
    FILE *f = fopen("/proc/self/numa_maps", "r");
    if (!f) { perror("open"); return 1; }
    char line[1024];
    while (fgets(line, sizeof line, f)) fputs(line, stdout);
    fclose(f);
    return 0;
}
