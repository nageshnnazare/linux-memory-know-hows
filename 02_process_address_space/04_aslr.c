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

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 02_process_address_space/04_aslr.c
 * Command: make -C 02_process_address_space 04_aslr
 * Exit status: 0
 * Output:
 * text  (main)     = 0x564837bb0199
 * data  (data_var) = 0x564837bb3010
 * heap  (malloc)   = 0x564848f232a0
 * stack (local)    = 0x7ffc4373fd0c
 * libc  (printf)   = 0x7f4bb0a60100
 * argv  (ptr)      = 0x7ffc4373fe48
 * AUTO-GENERATED RUN OUTPUT END
 */
