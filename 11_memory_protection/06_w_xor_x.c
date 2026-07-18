/*
 * 06_w_xor_x.c
 * -------------
 * W^X demo: try to execute a writable page.  CPU's NX bit + Linux mapping
 * combine to SIGSEGV us instantly.
 *
 * If you remove PROT_WRITE before the call (mprotect -> RX) it works
 * (that's how a JIT does it).
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>

static void h(int s, siginfo_t *si, void *uc)
{
    (void)s; (void)uc;
    char m[128];
    int n = snprintf(m, sizeof m,
                     "SIGSEGV: attempted to execute non-X page %p\n",
                     si->si_addr);
    int unused = write(2, m, n);
    (void)unused;
    _exit(1);
}

int main(void)
{
    struct sigaction sa = {0};
    sa.sa_sigaction = h; sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);

    long pg = sysconf(_SC_PAGESIZE);
    void *p = mmap(NULL, pg, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    /* x86_64:  ret == 0xC3 */
    *(unsigned char *)p = 0xC3;

    printf("about to call writable (NX) page %p ...\n", p);
    ((void (*)(void))p)();   /* SIGSEGV here */
    printf("never reached\n");
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 11_memory_protection/06_w_xor_x.c
 * Command: make -C 11_memory_protection 06_w_xor_x
 * Exit status: 0
 * Output:
 * SIGSEGV: attempted to execute non-X page 0x7ff9beabb000
 * AUTO-GENERATED RUN OUTPUT END
 */
