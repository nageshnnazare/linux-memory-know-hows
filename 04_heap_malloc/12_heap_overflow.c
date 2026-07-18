/*
 * 12_heap_overflow.c
 * -------------------
 * Overflow a small chunk and trash the next chunk's metadata.
 *
 *   memory                                   what's there
 *   p [ ........ N bytes payload ........ ]  our chunk's data
 *     [ prev_size | size | A M P (header) ]  NEXT chunk's header
 *     [ ........ next chunk's data ...... ]  someone else's allocation
 *
 *   Writing past the end of p clobbers the next chunk's header.  On the
 *   next free()/malloc() that touches the corrupt chunk, glibc's
 *   consistency check (e.g. "size > arena_max" or "fd/bk invalid")
 *   triggers an abort:
 *
 *     malloc(): corrupted unsorted chunks
 *     Aborted (core dumped)
 *
 * Build with ASan for clean diagnostics:
 *   gcc -O0 -g -fsanitize=address 12_heap_overflow.c -o 12_overflow
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *a = malloc(32);
    char *b = malloc(32);
    printf("a=%p  b=%p  (b-a=%ld bytes)\n", a, b, b - a);
    /* If b sits right after a, writing 80 bytes into a will trash b's header */
    memset(a, 'A', 80);

    /* now try to free both; the second one likely aborts */
    free(a);
    free(b);
    puts("survived (some glibc versions only catch this at coalesce time)");
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/12_heap_overflow.c
 * Command: make -C 04_heap_malloc 12_heap_overflow
 * Exit status: 0
 * Output:
 * double free or corruption (out)
 * AUTO-GENERATED RUN OUTPUT END
 */
