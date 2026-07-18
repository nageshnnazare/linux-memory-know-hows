/*
 * 02_proc_smaps.c
 * ----------------
 * Aggregate RSS / PSS / Swap across all VMAs.
 *
 *   RSS  = resident set size (shared pages counted in full per process)
 *   PSS  = proportional set size (shared pages divided by # users)
 *   USS  = unique set size (private pages only)
 *
 *   PSS is the closest thing to "honest" memory usage on shared systems.
 */
#include <stdio.h>
#include <string.h>

int main(void)
{
    FILE *f = fopen("/proc/self/smaps", "r");
    char line[512];
    long rss = 0, pss = 0, swap = 0, priv_dirty = 0, priv_clean = 0;
    while (fgets(line, sizeof line, f)) {
        long v;
        if (sscanf(line, "Rss: %ld kB", &v) == 1) rss += v;
        else if (sscanf(line, "Pss: %ld kB", &v) == 1) pss += v;
        else if (sscanf(line, "Swap: %ld kB", &v) == 1) swap += v;
        else if (sscanf(line, "Private_Dirty: %ld kB", &v) == 1) priv_dirty += v;
        else if (sscanf(line, "Private_Clean: %ld kB", &v) == 1) priv_clean += v;
    }
    fclose(f);
    printf("RSS           = %8ld KiB\n", rss);
    printf("PSS           = %8ld KiB  (your fair share)\n", pss);
    printf("Swap          = %8ld KiB\n", swap);
    printf("Private_Dirty = %8ld KiB  (would survive across exec)\n", priv_dirty);
    printf("Private_Clean = %8ld KiB  (can be dropped freely)\n", priv_clean);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 12_debugging_tools/02_proc_smaps.c
 * Command: make -C 12_debugging_tools 02_proc_smaps
 * Exit status: 0
 * Output:
 * RSS           =     1612 KiB
 * PSS           =      153 KiB  (your fair share)
 * Swap          =        0 KiB
 * Private_Dirty =      100 KiB  (would survive across exec)
 * Private_Clean =       12 KiB  (can be dropped freely)
 * AUTO-GENERATED RUN OUTPUT END
 */
