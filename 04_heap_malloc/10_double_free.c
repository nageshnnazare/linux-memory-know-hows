/*
 * 10_double_free.c
 * -----------------
 * Trigger glibc's double-free detector.  On a modern glibc you'll see:
 *
 *   free(): double free detected in tcache 2
 *   Aborted (core dumped)
 *
 * Mechanism:
 *   tcache_entry has a "key" field set to the address of the per-thread
 *   tcache_perthread_struct.  On free, glibc checks: if a tcache entry
 *   carries that key, this chunk is suspected to already be in the bin.
 *
 * This is HEURISTIC, not a security boundary -- attackers can bypass it.
 * Use ASan to detect double-free reliably.
 */
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    void *p = malloc(32);
    printf("p = %p\n", p);
    free(p);
    printf("first free OK\n");
    free(p);                       /* boom (eventually) */
    printf("second free? unreachable\n");
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 04_heap_malloc/10_double_free.c
 * Command: make -C 04_heap_malloc 10_double_free
 * Exit status: 0
 * Output:
 * free(): double free detected in tcache 2
 * AUTO-GENERATED RUN OUTPUT END
 */
