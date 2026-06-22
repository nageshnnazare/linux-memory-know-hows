/*
 * 04_canary_demo.c
 * -----------------
 * Build with -fstack-protector-strong; overflow a local buffer; observe
 * "stack smashing detected" abort.
 *
 *   gcc -O0 -g -fstack-protector-strong 04_canary_demo.c -o 04_canary_demo
 */
#include <stdio.h>
#include <string.h>

__attribute__((noinline))
static void f(const char *s)
{
    char buf[16];
    strcpy(buf, s);
    printf("buf=%s\n", buf);
}

int main(void)
{
    f("short");
    f("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");  // overflow
    return 0;
}
