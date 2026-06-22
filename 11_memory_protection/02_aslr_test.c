/*
 * 02_aslr_test.c
 * ---------------
 * Print key addresses; run several times to see ASLR.
 *
 *   $ for i in 1 2 3 4 5; do ./02_aslr_test; done
 *
 * Disable per-run:    setarch $(uname -m) -R ./02_aslr_test
 * System-wide:        sudo sysctl kernel.randomize_va_space=0  (don't!)
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int stack;
    char *h = malloc(16);
    printf("text  = %p   stack = %p   heap = %p   libc = %p   argv = %p\n",
           (void *)main, (void *)&stack, (void *)h, (void *)printf, (void *)argv);
    free(h);
    (void)argc;
    return 0;
}
