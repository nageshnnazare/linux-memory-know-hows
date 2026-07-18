/*
 * 05_fortify_demo.c
 * ------------------
 * With _FORTIFY_SOURCE=2 and -O1+, glibc replaces certain calls (strcpy,
 * memcpy, sprintf, ...) with versions that include a destination-size
 * check.  Overflow -> abort with a clear message.
 *
 *   gcc -O2 -D_FORTIFY_SOURCE=2 05_fortify_demo.c -o 05_fortify_demo
 */
#define _FORTIFY_SOURCE 2
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    char buf[16];
    const char *src = (argc > 1) ? argv[1] :
        "this is way too long for the 16-byte buffer";

    strcpy(buf, src);          /* fortified */
    printf("buf=%s\n", buf);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 11_memory_protection/05_fortify_demo.c
 * Command: make -C 11_memory_protection 05_fortify_demo
 * Exit status: 0
 * Output:
 * *** buffer overflow detected ***: terminated
 * AUTO-GENERATED RUN OUTPUT END
 */
