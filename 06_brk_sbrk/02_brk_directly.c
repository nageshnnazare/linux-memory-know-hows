/*
 * 02_brk_directly.c
 * ------------------
 * Allocate memory WITHOUT malloc by calling sbrk() directly.
 *
 *   step 1: remember the current break (= start of our "arena").
 *   step 2: grow by 16 KiB.
 *   step 3: use the new 16 KiB.
 *   step 4: shrink back.
 *
 * This is essentially what early-day malloc implementations did.
 * Not recommended in production -- use mmap() for one-off slabs.
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    void *base = sbrk(0);
    printf("initial break = %p\n", base);

    void *got = sbrk(16 * 1024);
    if (got == (void *)-1) { perror("sbrk grow"); return 1; }
    printf("got 16 KiB from sbrk; old break = %p, new break = %p\n",
           got, sbrk(0));

    /* use it */
    char *p = got;
    memset(p, 'A', 16 * 1024);
    printf("wrote 16 KiB starting at %p\n", p);

    /* give it back */
    if (sbrk(-16 * 1024) == (void *)-1) perror("sbrk shrink");
    printf("new break = %p (should equal initial)\n", sbrk(0));
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 06_brk_sbrk/02_brk_directly.c
 * Command: make -C 06_brk_sbrk 02_brk_directly
 * Exit status: 0
 * Output:
 * initial break = 0x5651f35b6000
 * got 16 KiB from sbrk; old break = 0x5651f35d7000, new break = 0x5651f35db000
 * wrote 16 KiB starting at 0x5651f35d7000
 * new break = 0x5651f35d7000 (should equal initial)
 * AUTO-GENERATED RUN OUTPUT END
 */
