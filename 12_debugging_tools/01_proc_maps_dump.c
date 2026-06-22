/*
 * 01_proc_maps_dump.c
 * --------------------
 * Pretty-print /proc/self/maps with totals.
 */
#include <stdio.h>
#include <string.h>

int main(void)
{
    FILE *f = fopen("/proc/self/maps", "r");
    char line[512];
    unsigned long total = 0;
    printf("%-30s %-5s %-10s  %-25s  %-10s\n",
           "range", "perm", "size(KiB)", "path", "kind");
    while (fgets(line, sizeof line, f)) {
        unsigned long a, b; char perm[6], dev[8]; unsigned long off, inode;
        char path[256] = "";
        int n = sscanf(line, "%lx-%lx %5s %lx %7s %lu %255[^\n]",
                       &a, &b, perm, &off, dev, &inode, path);
        if (n < 6) continue;
        const char *kind = path[0] == '[' ? path
                         : path[0] == 0   ? "(anon)"
                         : "file";
        printf("%012lx-%012lx %-5s %10lu  %-25.25s  %-10s\n",
               a, b, perm, (b - a) >> 10, path, kind);
        total += (b - a);
    }
    fclose(f);
    printf("VSZ total = %lu KiB (%lu MiB)\n", total >> 10, total >> 20);
    return 0;
}
