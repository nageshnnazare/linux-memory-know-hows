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

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 02_process_address_space/03_dump_maps.c
 * Command: make -C 02_process_address_space 03_dump_maps
 * Exit status: 0
 * Output:
 * range                                 perm  offset    dev     inode    path
 * ---------------------------------------------------------------------
 * 0x55ce1e1b1000-0x55ce1e1b2000 r--p  00000000 08:01   2118100  /home/runner/work/linux-memory-know-hows/linux-memory-know-hows/02_process_address_space/03_dump_maps  (4 KiB)
 * 0x55ce1e1b2000-0x55ce1e1b3000 r-xp  00001000 08:01   2118100  /home/runner/work/linux-memory-know-hows/linux-memory-know-hows/02_process_address_space/03_dump_maps  (4 KiB)
 * 0x55ce1e1b3000-0x55ce1e1b4000 r--p  00002000 08:01   2118100  /home/runner/work/linux-memory-know-hows/linux-memory-know-hows/02_process_address_space/03_dump_maps  (4 KiB)
 * 0x55ce1e1b4000-0x55ce1e1b5000 r--p  00002000 08:01   2118100  /home/runner/work/linux-memory-know-hows/linux-memory-know-hows/02_process_address_space/03_dump_maps  (4 KiB)
 * 0x55ce1e1b5000-0x55ce1e1b6000 rw-p  00003000 08:01   2118100  /home/runner/work/linux-memory-know-hows/linux-memory-know-hows/02_process_address_space/03_dump_maps  (4 KiB)
 * 0x55ce3eb98000-0x55ce3ebb9000 rw-p  00000000 00:00   0        [heap]  (132 KiB)
 * 0x7f24c7c00000-0x7f24c7c28000 r--p  00000000 08:01   6456     /usr/lib/x86_64-linux-gnu/libc.so.6  (160 KiB)
 * 0x7f24c7c28000-0x7f24c7db0000 r-xp  00028000 08:01   6456     /usr/lib/x86_64-linux-gnu/libc.so.6  (1568 KiB)
 * 0x7f24c7db0000-0x7f24c7dff000 r--p  001b0000 08:01   6456     /usr/lib/x86_64-linux-gnu/libc.so.6  (316 KiB)
 * 0x7f24c7dff000-0x7f24c7e03000 r--p  001fe000 08:01   6456     /usr/lib/x86_64-linux-gnu/libc.so.6  (16 KiB)
 * 0x7f24c7e03000-0x7f24c7e05000 rw-p  00202000 08:01   6456     /usr/lib/x86_64-linux-gnu/libc.so.6  (8 KiB)
 * 0x7f24c7e05000-0x7f24c7e12000 rw-p  00000000 00:00   0        (anon)  (52 KiB)
 * 0x7f24c7f2f000-0x7f24c7f32000 rw-p  00000000 00:00   0        (anon)  (12 KiB)
 * 0x7f24c7f3c000-0x7f24c7f3e000 rw-p  00000000 00:00   0        (anon)  (8 KiB)
 * 0x7f24c7f3e000-0x7f24c7f42000 r--p  00000000 00:00   0        [vvar]  (16 KiB)
 * 0x7f24c7f42000-0x7f24c7f44000 r--p  00000000 00:00   0        [vvar_vclock]  (8 KiB)
 * 0x7f24c7f44000-0x7f24c7f46000 r-xp  00000000 00:00   0        [vdso]  (8 KiB)
 * 0x7f24c7f46000-0x7f24c7f47000 r--p  00000000 08:01   6453     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2  (4 KiB)
 * 0x7f24c7f47000-0x7f24c7f72000 r-xp  00001000 08:01   6453     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2  (172 KiB)
 * 0x7f24c7f72000-0x7f24c7f7c000 r--p  0002c000 08:01   6453     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2  (40 KiB)
 * 0x7f24c7f7c000-0x7f24c7f7e000 r--p  00036000 08:01   6453     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2  (8 KiB)
 * 0x7f24c7f7e000-0x7f24c7f80000 rw-p  00038000 08:01   6453     /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2  (8 KiB)
 * 0x7ffc4330b000-0x7ffc4332d000 rw-p  00000000 00:00   0        [stack]  (136 KiB)
 * 0xffffffffff600000-0xffffffffff601000 --xp  00000000 00:00   0        [vsyscall]  (4 KiB)
 * ---------------------------------------------------------------------
 * VSS (all VMAs)  = 2696 KiB  (2 MiB)
 * AUTO-GENERATED RUN OUTPUT END
 */
