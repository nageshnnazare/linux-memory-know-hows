/*
 * 04_aslr.c
 * ----------
 * Run this several times in a row.  Note that on a normal system the
 * addresses of every segment change between runs -- that's ASLR.
 *
 *   $ for i in 1 2 3; do ./04_aslr; echo "---"; done
 *
 * To disable ASLR for a single run:   setarch $(uname -m) -R ./04_aslr
 * To disable system-wide (dangerous): sudo sysctl -w kernel.randomize_va_space=0
 */

#include <stdio.h>
#include <stdlib.h>

static int data_var = 1;

int main(int argc, char **argv)
{
    int stack_var;
    char *heap = malloc(16);
    printf("text  (main)     = %p\n", (void *)main);
    printf("data  (data_var) = %p\n", (void *)&data_var);
    printf("heap  (malloc)   = %p\n", (void *)heap);
    printf("stack (local)    = %p\n", (void *)&stack_var);
    printf("libc  (printf)   = %p\n", (void *)printf);
    printf("argv  (ptr)      = %p\n", (void *)argv);
    free(heap);
    (void)argc;
    return 0;
}
