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

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 12_debugging_tools/01_proc_maps_dump.c
 * Command: make -C 12_debugging_tools 01_proc_maps_dump
 * Exit status: 0
 * Output:
 * range                          perm  size(KiB)   path                       kind      
 * 55807a453000-55807a454000 r--p           4  /home/runner/work/linux-m  file      
 * 55807a454000-55807a455000 r-xp           4  /home/runner/work/linux-m  file      
 * 55807a455000-55807a456000 r--p           4  /home/runner/work/linux-m  file      
 * 55807a456000-55807a457000 r--p           4  /home/runner/work/linux-m  file      
 * 55807a457000-55807a458000 rw-p           4  /home/runner/work/linux-m  file      
 * 5580b12a6000-5580b12c7000 rw-p         132  [heap]                     [heap]    
 * 7f6fc7a00000-7f6fc7a28000 r--p         160  /usr/lib/x86_64-linux-gnu  file      
 * 7f6fc7a28000-7f6fc7bb0000 r-xp        1568  /usr/lib/x86_64-linux-gnu  file      
 * 7f6fc7bb0000-7f6fc7bff000 r--p         316  /usr/lib/x86_64-linux-gnu  file      
 * 7f6fc7bff000-7f6fc7c03000 r--p          16  /usr/lib/x86_64-linux-gnu  file      
 * 7f6fc7c03000-7f6fc7c05000 rw-p           8  /usr/lib/x86_64-linux-gnu  file      
 * 7f6fc7c05000-7f6fc7c12000 rw-p          52                             (anon)    
 * 7f6fc7cbd000-7f6fc7cc0000 rw-p          12                             (anon)    
 * 7f6fc7cca000-7f6fc7ccc000 rw-p           8                             (anon)    
 * 7f6fc7ccc000-7f6fc7cd0000 r--p          16  [vvar]                     [vvar]    
 * 7f6fc7cd0000-7f6fc7cd2000 r--p           8  [vvar_vclock]              [vvar_vclock]
 * 7f6fc7cd2000-7f6fc7cd4000 r-xp           8  [vdso]                     [vdso]    
 * 7f6fc7cd4000-7f6fc7cd5000 r--p           4  /usr/lib/x86_64-linux-gnu  file      
 * 7f6fc7cd5000-7f6fc7d00000 r-xp         172  /usr/lib/x86_64-linux-gnu  file      
 * 7f6fc7d00000-7f6fc7d0a000 r--p          40  /usr/lib/x86_64-linux-gnu  file      
 * 7f6fc7d0a000-7f6fc7d0c000 r--p           8  /usr/lib/x86_64-linux-gnu  file      
 * 7f6fc7d0c000-7f6fc7d0e000 rw-p           8  /usr/lib/x86_64-linux-gnu  file      
 * 7ffeca957000-7ffeca979000 rw-p         136  [stack]                    [stack]   
 * ffffffffff600000-ffffffffff601000 --xp           4  [vsyscall]                 [vsyscall]
 * VSZ total = 2696 KiB (2 MiB)
 * AUTO-GENERATED RUN OUTPUT END
 */
