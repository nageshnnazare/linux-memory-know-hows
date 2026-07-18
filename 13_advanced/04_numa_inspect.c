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

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 13_advanced/04_numa_inspect.c
 * Command: make -C 13_advanced 04_numa_inspect
 * Exit status: 0
 * Output:
 * 5630bb031000 default file=/home/runner/work/linux-memory-know-hows/linux-memory-know-hows/13_advanced/04_numa_inspect mapped=1 active=0 N0=1 kernelpagesize_kB=4
 * 5630bb032000 default file=/home/runner/work/linux-memory-know-hows/linux-memory-know-hows/13_advanced/04_numa_inspect mapped=1 active=0 N0=1 kernelpagesize_kB=4
 * 5630bb033000 default file=/home/runner/work/linux-memory-know-hows/linux-memory-know-hows/13_advanced/04_numa_inspect mapped=1 active=0 N0=1 kernelpagesize_kB=4
 * 5630bb034000 default file=/home/runner/work/linux-memory-know-hows/linux-memory-know-hows/13_advanced/04_numa_inspect anon=1 dirty=1 N0=1 kernelpagesize_kB=4
 * 5630bb035000 default file=/home/runner/work/linux-memory-know-hows/linux-memory-know-hows/13_advanced/04_numa_inspect anon=1 dirty=1 N0=1 kernelpagesize_kB=4
 * 5630cdd0a000 default heap anon=1 dirty=1 N0=1 kernelpagesize_kB=4
 * 7f5fb4200000 default file=/usr/lib/x86_64-linux-gnu/libc.so.6 mapped=40 mapmax=34 N0=40 kernelpagesize_kB=4
 * 7f5fb4228000 default file=/usr/lib/x86_64-linux-gnu/libc.so.6 mapped=200 mapmax=39 N0=200 kernelpagesize_kB=4
 * 7f5fb43b0000 default file=/usr/lib/x86_64-linux-gnu/libc.so.6 mapped=16 mapmax=37 N0=16 kernelpagesize_kB=4
 * 7f5fb43ff000 default file=/usr/lib/x86_64-linux-gnu/libc.so.6 anon=4 dirty=4 N0=4 kernelpagesize_kB=4
 * 7f5fb4403000 default file=/usr/lib/x86_64-linux-gnu/libc.so.6 anon=2 dirty=2 N0=2 kernelpagesize_kB=4
 * 7f5fb4405000 default anon=5 dirty=5 N0=5 kernelpagesize_kB=4
 * 7f5fb448f000 default anon=2 dirty=2 N0=2 kernelpagesize_kB=4
 * 7f5fb449c000 default anon=1 dirty=1 N0=1 kernelpagesize_kB=4
 * 7f5fb449e000 default
 * 7f5fb44a2000 default
 * 7f5fb44a4000 default
 * 7f5fb44a6000 default file=/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 mapped=1 mapmax=33 N0=1 kernelpagesize_kB=4
 * 7f5fb44a7000 default file=/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 mapped=43 mapmax=36 N0=43 kernelpagesize_kB=4
 * 7f5fb44d2000 default file=/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 mapped=10 mapmax=34 N0=10 kernelpagesize_kB=4
 * 7f5fb44dc000 default file=/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 anon=2 dirty=2 N0=2 kernelpagesize_kB=4
 * 7f5fb44de000 default file=/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 anon=2 dirty=2 N0=2 kernelpagesize_kB=4
 * 7ffecea29000 default stack anon=4 dirty=4 active=2 N0=4 kernelpagesize_kB=4
 * AUTO-GENERATED RUN OUTPUT END
 */
