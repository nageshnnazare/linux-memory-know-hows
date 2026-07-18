/*
 * 06_stack_canary.c
 * ------------------
 * Trigger the stack canary (-fstack-protector-strong) by writing past the
 * end of a local buffer.  glibc will abort with:
 *
 *     *** stack smashing detected ***: terminated
 *     Aborted (core dumped)
 *
 * Build:  gcc -O0 -g -fstack-protector-strong 06_stack_canary.c -o 06
 *
 * The canary is a random value placed between locals and the saved
 * return address.  The function epilogue checks it; if it has been
 * corrupted (overflow), libc calls __stack_chk_fail() and the process dies.
 */
#include <stdio.h>
#include <string.h>

__attribute__((noinline))
static void victim(const char *src)
{
    char buf[16];
    /* Intentionally use strcpy without length check */
    strcpy(buf, src);
    printf("buf = %s\n", buf);
}

int main(void)
{
    char ok[]   = "fits";
    char bomb[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

    victim(ok);
    printf("about to overflow on purpose...\n");
    victim(bomb);    /* canary check should fire here */
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 03_stack/06_stack_canary.c
 * Command: make -C 03_stack 06_stack_canary
 * Exit status: 0
 * Output:
 * *** stack smashing detected ***: terminated
 * AUTO-GENERATED RUN OUTPUT END
 */
