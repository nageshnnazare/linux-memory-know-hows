/*
 * 01_segments_addresses.c
 * ------------------------
 * Print the address of one variable per segment so you can see the layout
 * of a Linux process at run-time.  Compare these addresses to the lines in
 * /proc/self/maps (printed at the end).
 *
 *   text   = code,            low addresses (PIE base, ~0x55... with ASLR)
 *   rodata = const strings,   right above text
 *   data   = init globals,    just above rodata
 *   bss    = uninit globals,  right after data
 *   heap   = brk-managed,     above bss with an ASLR gap
 *   mmap   = anonymous,       high addresses, just below shared libs
 *   stack  = local variables, very high addresses, grows down
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static const char rodata_const[] = "I live in .rodata";
static int   data_int = 42;     /* initialised -> .data */
static int   bss_int;           /* uninitialised, zero -> .bss */

static void some_function(void) {}

int main(int argc, char **argv)
{
    int    stack_int = 1;
    char  *heap_buf  = malloc(64);
    char  *mmap_buf  = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    printf("=== Segment addresses ===\n");
    printf("  text   (main)            : %p\n", (void *)main);
    printf("  text   (some_function)   : %p\n", (void *)some_function);
    printf("  rodata (literal)         : %p\n", (void *)"a string literal");
    printf("  rodata (const array)     : %p\n", (void *)rodata_const);
    printf("  data   (init global)     : %p  (val=%d)\n", (void *)&data_int, data_int);
    printf("  bss    (uninit global)   : %p  (val=%d)\n", (void *)&bss_int,  bss_int);
    printf("  heap   (malloc)          : %p\n", (void *)heap_buf);
    printf("  mmap   (anonymous)       : %p\n", (void *)mmap_buf);
    printf("  stack  (local var)       : %p\n", (void *)&stack_int);
    printf("  stack  (argv ptr array)  : %p\n", (void *)argv);
    printf("  stack  (argv[0] string)  : %p  (= \"%s\")\n",
           (void *)argv[0], argv[0]);
    printf("  libc   (printf)          : %p\n", (void *)printf);

    /* dump /proc/self/maps so you can correlate */
    puts("\n=== /proc/self/maps ===");
    FILE *f = fopen("/proc/self/maps", "r");
    if (f) { char buf[512]; while (fgets(buf, sizeof buf, f)) fputs(buf, stdout); fclose(f); }

    free(heap_buf);
    munmap(mmap_buf, 4096);
    (void)argc;
    return 0;
}
