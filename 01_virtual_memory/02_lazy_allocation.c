/*
 * 02_lazy_allocation.c
 * ---------------------
 * Reserve 1 GiB with mmap, then touch one byte per page in a loop.
 * Watch RSS climb from ~0 to ~1 GiB even though VSZ was 1 GiB from the
 * start.
 *
 *   VSZ (virtual)  = sum of all VMAs (mmap regions, stack, heap, ...)
 *   RSS (resident) = sum of frames actually mapped into RAM
 *
 *   step 1: mmap 1 GiB                   -> VSZ +1 GiB, RSS +0
 *   step 2: read one byte of each page   -> RSS +0   (zero page mapped RO)
 *   step 3: write one byte of each page  -> RSS +1 GiB  (real frames!)
 *
 * Inspect with:  watch -n0.2 "cat /proc/$(pidof 02_lazy_allocation)/status \
 *                              | grep -E 'Vm(Size|RSS|Data)'"
 *
 * Build:  gcc -O0 -g 02_lazy_allocation.c -o 02_lazy_allocation
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>

static void show(const char *tag)
{
    /* Read /proc/self/status to print VmRSS without parsing tools */
    FILE *f = fopen("/proc/self/status", "r");
    char line[256];
    long vm = 0, rss = 0;
    while (fgets(line, sizeof line, f)) {
        if (!strncmp(line, "VmSize:", 7)) sscanf(line + 7, "%ld", &vm);
        if (!strncmp(line, "VmRSS:",  6)) sscanf(line + 6, "%ld", &rss);
    }
    fclose(f);

    struct rusage r; getrusage(RUSAGE_SELF, &r);
    printf("[%-15s] VmSize=%6ld KiB  VmRSS=%6ld KiB  minflt=%-5ld majflt=%ld\n",
           tag, vm, rss, r.ru_minflt, r.ru_majflt);
}

#include <string.h>
int main(void)
{
    const size_t BYTES = 1024UL * 1024 * 1024; /* 1 GiB */
    const long pg = sysconf(_SC_PAGESIZE);

    show("start");

    char *p = mmap(NULL, BYTES, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    show("after mmap");

    /* Read one byte of every page.  The kernel maps the zero page RO. */
    volatile char sink = 0;
    for (size_t i = 0; i < BYTES; i += pg) sink = p[i];
    (void)sink;
    show("after reads");

    /* Write one byte of every page.  CoW from zero page -> real frame. */
    for (size_t i = 0; i < BYTES; i += pg) p[i] = (char)i;
    show("after writes");

    /* Give it back to the OS without unmapping.  RSS should drop. */
    madvise(p, BYTES, MADV_DONTNEED);
    show("after DONTNEED");

    munmap(p, BYTES);
    show("after munmap");
    return 0;
}
