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
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 12_debugging_tools/01_proc_maps_dump.c
 * Command: make -C 12_debugging_tools 01_proc_maps_dump
 * Exit status: 0
 * Output:
 * range                          perm  size(KiB)   path                       kind      
 * 558f64667000-558f64668000 r--p           4  /home/runner/work/linux-m  file      
 * 558f64668000-558f64669000 r-xp           4  /home/runner/work/linux-m  file      
 * 558f64669000-558f6466a000 r--p           4  /home/runner/work/linux-m  file      
 * 558f6466a000-558f6466b000 r--p           4  /home/runner/work/linux-m  file      
 * 558f6466b000-558f6466c000 rw-p           4  /home/runner/work/linux-m  file      
 * 558f7416c000-558f7418d000 rw-p         132  [heap]                     [heap]    
 * 7f53f5600000-7f53f5628000 r--p         160  /usr/lib/x86_64-linux-gnu  file      
 * 7f53f5628000-7f53f57b0000 r-xp        1568  /usr/lib/x86_64-linux-gnu  file      
 * 7f53f57b0000-7f53f57ff000 r--p         316  /usr/lib/x86_64-linux-gnu  file      
 * 7f53f57ff000-7f53f5803000 r--p          16  /usr/lib/x86_64-linux-gnu  file      
 * 7f53f5803000-7f53f5805000 rw-p           8  /usr/lib/x86_64-linux-gnu  file      
 * 7f53f5805000-7f53f5812000 rw-p          52                             (anon)    
 * 7f53f5890000-7f53f5893000 rw-p          12                             (anon)    
 * 7f53f589d000-7f53f589f000 rw-p           8                             (anon)    
 * 7f53f589f000-7f53f58a3000 r--p          16  [vvar]                     [vvar]    
 * 7f53f58a3000-7f53f58a5000 r--p           8  [vvar_vclock]              [vvar_vclock]
 * 7f53f58a5000-7f53f58a7000 r-xp           8  [vdso]                     [vdso]    
 * 7f53f58a7000-7f53f58a8000 r--p           4  /usr/lib/x86_64-linux-gnu  file      
 * 7f53f58a8000-7f53f58d3000 r-xp         172  /usr/lib/x86_64-linux-gnu  file      
 * 7f53f58d3000-7f53f58dd000 r--p          40  /usr/lib/x86_64-linux-gnu  file      
 * 7f53f58dd000-7f53f58df000 r--p           8  /usr/lib/x86_64-linux-gnu  file      
 * 7f53f58df000-7f53f58e1000 rw-p           8  /usr/lib/x86_64-linux-gnu  file      
 * 7ffef6d77000-7ffef6d99000 rw-p         136  [stack]                    [stack]   
 * ffffffffff600000-ffffffffff601000 --xp           4  [vsyscall]                 [vsyscall]
 * VSZ total = 2696 KiB (2 MiB)
 * AUTO-GENERATED RUN OUTPUT END
 */
