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

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 11_memory_protection/02_aslr_test.c
 * Command: make -C 11_memory_protection 02_aslr_test
 * Exit status: 0
 * Output:
 * text  = 0x55d1882a7199   stack = 0x7ffc9455e92c   heap = 0x55d194a9e2a0   libc = 0x7efd05e60100   argv = 0x7ffc9455ea68
 * AUTO-GENERATED RUN OUTPUT END
 */
