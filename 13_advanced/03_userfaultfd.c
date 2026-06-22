/*
 * 03_userfaultfd.c
 * -----------------
 * Userfaultfd: register a region; a worker thread receives page-fault
 * messages and fills pages on demand from user space.
 *
 *   1. addr = mmap(NULL, sz, ANON|PRIVATE) -- reserved but unfilled
 *   2. ufd = userfaultfd()
 *   3. ioctl(UFFDIO_API)
 *   4. ioctl(UFFDIO_REGISTER, MODE_MISSING)
 *   5. thread: for (;;) read(ufd, &msg)
 *                     ioctl(UFFDIO_COPY, src=our buf, dst=fault addr)
 *   6. main: write to addr -> faults trapped to ufd handler, filled with our content.
 *
 * vm.unprivileged_userfaultfd may need to be 1 (Linux 5.11+) or run as root.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

static int ufd;
static char *base;
static size_t reg_len;
static long pg;

static void *fault_thread(void *arg)
{
    (void)arg;
    char fillbuf[1 << 16];
    memset(fillbuf, 'U', sizeof fillbuf); /* our magic byte */
    struct pollfd p = { .fd = ufd, .events = POLLIN };

    for (;;) {
        if (poll(&p, 1, -1) < 0) { if (errno == EINTR) continue; perror("poll"); break; }
        struct uffd_msg msg;
        ssize_t n = read(ufd, &msg, sizeof msg);
        if (n != sizeof msg) break;
        if (msg.event != UFFD_EVENT_PAGEFAULT) continue;

        uintptr_t fa = (uintptr_t)msg.arg.pagefault.address & ~(pg - 1);
        struct uffdio_copy c = {
            .src = (uintptr_t)fillbuf,
            .dst = fa,
            .len = pg,
            .mode = 0
        };
        if (ioctl(ufd, UFFDIO_COPY, &c) < 0) { perror("UFFDIO_COPY"); break; }
        printf("[uffd] satisfied fault @ %p with 'U' fill\n", (void *)fa);
    }
    return NULL;
}

int main(void)
{
    pg = sysconf(_SC_PAGESIZE);
    ufd = syscall(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (ufd < 0) { perror("userfaultfd"); return 1; }

    struct uffdio_api api = { .api = UFFD_API };
    if (ioctl(ufd, UFFDIO_API, &api) < 0) { perror("UFFDIO_API"); return 1; }

    reg_len = 4 * pg;
    base = mmap(NULL, reg_len, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) { perror("mmap"); return 1; }

    struct uffdio_register reg = {
        .range = { .start = (uintptr_t)base, .len = reg_len },
        .mode  = UFFDIO_REGISTER_MODE_MISSING
    };
    if (ioctl(ufd, UFFDIO_REGISTER, &reg) < 0) { perror("UFFDIO_REGISTER"); return 1; }

    pthread_t t; pthread_create(&t, NULL, fault_thread, NULL);

    /* trigger faults */
    for (size_t off = 0; off < reg_len; off += pg) {
        printf("[main] reading first byte of page %zu  ...\n", off / pg);
        printf("[main] got 0x%02x\n", (unsigned char)base[off]);
    }
    munmap(base, reg_len);
    close(ufd);
    return 0;
}
