/*
 * 03_dump_maps.c
 * ---------------
 * Read /proc/self/maps and pretty-print it grouped by region kind:
 *   text, rodata, data/bss, heap, stack, mmap-anon, mmap-file, vvar/vdso.
 *
 * Use this on any program you don't understand the memory map of:
 *   $ gdb ./victim ; (gdb) catch syscall mmap ; ...
 * or just have the program pause and read its own maps.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) { perror("open"); return 1; }

    char line[512];
    printf("%-37s %-5s %-9s %-7s %-7s  %s\n",
           "range", "perm", "offset", "dev", "inode", "path");
    printf("---------------------------------------------------------------------\n");

    unsigned long total = 0;
    while (fgets(line, sizeof line, f)) {
        unsigned long start, end;
        char perms[6], dev[8];
        unsigned long off, inode;
        char path[256] = "";
        int n = sscanf(line, "%lx-%lx %5s %lx %7s %lu %255[^\n]",
                       &start, &end, perms, &off, dev, &inode, path);
        if (n < 6) continue;
        unsigned long sz = end - start;
        total += sz;

        const char *kind = "anon";
        if (path[0] == '[') kind = path;       /* [heap] [stack] [vdso] ... */
        else if (path[0])   kind = "file";

        printf("0x%012lx-0x%012lx %-5s %08lx %-7s %-7lu  %s  (%lu KiB)\n",
               start, end, perms, off, dev, inode,
               path[0] ? path : "(anon)",
               sz >> 10);
        (void)kind;
    }
    fclose(f);
    printf("---------------------------------------------------------------------\n");
    printf("VSS (all VMAs)  = %lu KiB  (%lu MiB)\n",
           total >> 10, total >> 20);
    return 0;
}
