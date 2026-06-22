/*
 * 05_mprotect_guard.c
 * --------------------
 * Surround a buffer with PROT_NONE guard pages.  Any access just past
 * the buffer raises SIGSEGV instead of silently corrupting nearby memory.
 *
 *   page 0           page 1                          page 2
 *   +-------------+ +------------------------------+ +-------------+
 *   |  PROT_NONE  | |  PROT_READ | PROT_WRITE      | |  PROT_NONE  |
 *   |  guard      | |  the actual usable buffer    | |  guard      |
 *   +-------------+ +------------------------------+ +-------------+
 *
 * Useful for catching off-by-one bugs at the cost of 2 wasted pages per buffer.
 * (electricfence, page_alloc debug-style.)
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

static void on_segv(int s, siginfo_t *si, void *uc)
{
    (void)s; (void)uc;
    char m[128];
    int n = snprintf(m, sizeof m, "SIGSEGV at %p -- guard tripped, exiting.\n", si->si_addr);
    write(2, m, n);
    _exit(1);
}

int main(void)
{
    size_t pg = sysconf(_SC_PAGESIZE);
    size_t total = 3 * pg;

    char *base = mmap(NULL, total, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    /* make first and last page inaccessible */
    mprotect(base,                PROT_NONE);
    mprotect(base + 2 * pg, pg,   PROT_NONE);
    (void)mprotect(base, pg, PROT_NONE);

    char *buf = base + pg;
    printf("buffer = [%p .. %p)  size=%zu\n", buf, buf + pg, pg);

    /* install handler */
    struct sigaction sa = {0};
    sa.sa_sigaction = on_segv;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);

    /* normal use */
    memset(buf, 0x42, pg);
    printf("normal access OK (set first %zu bytes)\n", pg);

    /* now step past the end -- guard page!  */
    printf("about to deliberately overflow by 1 byte...\n");
    buf[pg] = 'X';      /* SIGSEGV here */
    printf("unreachable\n");
    return 0;
}
