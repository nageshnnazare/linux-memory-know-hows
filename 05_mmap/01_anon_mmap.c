/*
 * 01_anon_mmap.c
 * ---------------
 * Anonymous mmap is "give me N bytes of zero".  This is the foundation of
 * malloc's big allocations.
 */
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    size_t len = 4 * 1024 * 1024; /* 4 MiB */
    void *p = mmap(NULL, len,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }

    printf("4 MiB anonymous mapping @ %p\n", p);
    /* anonymous pages are guaranteed zero (kernel either maps the zero page
       or allocates a fresh frame and zeroes it) */
    char *q = p;
    printf("q[0]=%d q[4095]=%d q[len-1]=%d (all zero?)\n",
           q[0], q[4095], q[len - 1]);

    /* write some data; subsequent reads see it */
    memcpy(q, "hello mmap", 11);
    printf("after write: \"%s\"\n", q);

    munmap(p, len);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 05_mmap/01_anon_mmap.c
 * Command: make -C 05_mmap 01_anon_mmap
 * Exit status: 0
 * Output:
 * 4 MiB anonymous mapping @ 0x7f213ca00000
 * q[0]=0 q[4095]=0 q[len-1]=0 (all zero?)
 * after write: "hello mmap"
 * AUTO-GENERATED RUN OUTPUT END
 */
