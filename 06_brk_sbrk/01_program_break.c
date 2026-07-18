/*
 * 01_program_break.c
 * -------------------
 * Show the program break before and after a series of malloc() calls.
 *
 *   Small mallocs   -> heap grown with sbrk; break advances.
 *   Large malloc    -> mmap'd; break unchanged.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void show(const char *t)
{
    printf("[%-20s] sbrk(0) = %p\n", t, sbrk(0));
}

int main(void)
{
    show("start");

    void *p = malloc(64);
    show("after malloc(64)");

    void *p2 = malloc(64 * 1024);  /* still small enough for brk */
    show("after malloc(64K)");

    void *p3 = malloc(200 * 1024); /* triggers mmap path */
    show("after malloc(200K)");

    free(p); free(p2); free(p3);
    show("after frees");
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 06_brk_sbrk/01_program_break.c
 * Command: make -C 06_brk_sbrk 01_program_break
 * Exit status: 0
 * Output:
 * [start               ] sbrk(0) = 0x560434c47000
 * [after malloc(64)    ] sbrk(0) = 0x560434c68000
 * [after malloc(64K)   ] sbrk(0) = 0x560434c68000
 * [after malloc(200K)  ] sbrk(0) = 0x560434c68000
 * [after frees         ] sbrk(0) = 0x560434c68000
 * AUTO-GENERATED RUN OUTPUT END
 */
