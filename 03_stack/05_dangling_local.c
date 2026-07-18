/*
 * 05_dangling_local.c
 * --------------------
 * Return a pointer to a stack-local buffer.  The storage is GONE when the
 * function returns -- using the returned pointer is undefined behaviour.
 *
 *   bad():
 *     [ frame for bad ]
 *     +----------+
 *     | buf[16]  | <-- ptr returned
 *     +----------+
 *     bad returns; frame collapses;
 *     subsequent calls (printf, etc.) reuse this stack space.
 *
 * Compile WITHOUT optimisation to see the corruption.  With -O2 GCC may
 * detect this at compile time and even refuse to compile.
 */
#include <stdio.h>
#include <string.h>

/* prevent the compiler from inlining bad() into main, which would make
 * the bug "go away" in clever ways. */
__attribute__((noinline))
static char *bad(void)
{
    char buf[16];
    strcpy(buf, "danger!");
    return buf;       /* <--- WRONG, but C lets you do it */
}

__attribute__((noinline))
static void clobber(void)
{
    char other[256];
    memset(other, '#', sizeof other - 1);
    other[sizeof other - 1] = 0;
    /* don't print 'other' so optimizer keeps it; just touch it */
    if (other[0] == 'X') puts("never");
}

int main(void)
{
    char *p = bad();
    printf("right after return: \"%s\"\n", p);
    clobber();
    printf("after clobber:      \"%s\"\n", p);  /* probably garbage */
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 03_stack/05_dangling_local.c
 * Command: make -C 03_stack 05_dangling_local
 * Exit status: 0
 * Output:
 * right after return: "(null)"
 * after clobber:      "(null)"
 * AUTO-GENERATED RUN OUTPUT END
 */
