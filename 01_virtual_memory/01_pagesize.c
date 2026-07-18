/*
 * 01_pagesize.c
 * --------------
 * Print the OS page size and demonstrate that mmap rounds up to it.
 *
 *   sysconf(_SC_PAGESIZE)  == getpagesize()  == typically 4096 on x86_64
 *
 *   mmap(NULL, 1, PROT_RW, ANON|PRIVATE)         -> returns a whole 4 KiB page
 *   p = mmap(...);    you can write p[0]..p[4095]; touching p[4096] segfaults
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

int main(void)
{
    long pg = sysconf(_SC_PAGESIZE);
    printf("page size = %ld (sysconf)\n", pg);
    printf("page size = %d (getpagesize)\n", getpagesize());

    /* Ask for 1 byte; get back a whole page. */
    void *p = mmap(NULL, 1, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }

    printf("mmap'd 1 byte at %p (page-aligned? %s)\n",
           p, ((uintptr_t)p % pg == 0) ? "yes" : "no");

    /* writing inside the page is fine: */
    ((char *)p)[0]      = 'a';
    ((char *)p)[pg - 1] = 'z';
    printf("wrote p[0]=%c, p[%ld]=%c   (still in page)\n",
           ((char *)p)[0], pg - 1, ((char *)p)[pg - 1]);

    /* writing past the page would segfault: uncomment to test
       ((char *)p)[pg] = '!';
    */

    munmap(p, 1);  /* munmap also rounds up; the whole page is gone */
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 01_virtual_memory/01_pagesize.c
 * Command: make -C 01_virtual_memory 01_pagesize
 * Exit status: 0
 * Output:
 * page size = 4096 (sysconf)
 * page size = 4096 (getpagesize)
 * mmap'd 1 byte at 0x7fe1b1509000 (page-aligned? yes)
 * wrote p[0]=a, p[4095]=z   (still in page)
 * AUTO-GENERATED RUN OUTPUT END
 */
