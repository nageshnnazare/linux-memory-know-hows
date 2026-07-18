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

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 02_process_address_space/01_segments_addresses.c
 * Command: make -C 02_process_address_space 01_segments_addresses
 * Exit status: 0
 * Output:
 * === Segment addresses ===
 *   text   (main)            : 0x557fafdf2284
 *   text   (some_function)   : 0x557fafdf2279
 *   rodata (literal)         : 0x557fafdf3089
 *   rodata (const array)     : 0x557fafdf3010
 *   data   (init global)     : 0x557fafdf5010  (val=42)
 *   bss    (uninit global)   : 0x557fafdf5024  (val=0)
 *   heap   (malloc)          : 0x557fe82682a0
 *   mmap   (anonymous)       : 0x7f288cbdd000
 *   stack  (local var)       : 0x7ffed316c114
 *   stack  (argv ptr array)  : 0x7ffed316c468
 *   stack  (argv[0] string)  : 0x7ffed316dbf3  (= "./01_segments_addresses")
 *   libc   (printf)          : 0x7f288c860100
 * 
 * === /proc/self/maps ===
 * 557fafdf1000-557fafdf2000 r--p 00000000 08:01 2118074                    /home/runner/work/linux-memory-know-hows/linux-memory-know-hows/02_process_address_space/01_segments_addresses
 * 557fafdf2000-557fafdf3000 r-xp 00001000 08:01 2118074                    /home/runner/work/linux-memory-know-hows/linux-memory-know-hows/02_process_address_space/01_segments_addresses
 * 557fafdf3000-557fafdf4000 r--p 00002000 08:01 2118074                    /home/runner/work/linux-memory-know-hows/linux-memory-know-hows/02_process_address_space/01_segments_addresses
 * 557fafdf4000-557fafdf5000 r--p 00002000 08:01 2118074                    /home/runner/work/linux-memory-know-hows/linux-memory-know-hows/02_process_address_space/01_segments_addresses
 * 557fafdf5000-557fafdf6000 rw-p 00003000 08:01 2118074                    /home/runner/work/linux-memory-know-hows/linux-memory-know-hows/02_process_address_space/01_segments_addresses
 * 557fe8268000-557fe8289000 rw-p 00000000 00:00 0                          [heap]
 * 7f288c800000-7f288c828000 r--p 00000000 08:01 6456                       /usr/lib/x86_64-linux-gnu/libc.so.6
 * 7f288c828000-7f288c9b0000 r-xp 00028000 08:01 6456                       /usr/lib/x86_64-linux-gnu/libc.so.6
 * 7f288c9b0000-7f288c9ff000 r--p 001b0000 08:01 6456                       /usr/lib/x86_64-linux-gnu/libc.so.6
 * 7f288c9ff000-7f288ca03000 r--p 001fe000 08:01 6456                       /usr/lib/x86_64-linux-gnu/libc.so.6
 * 7f288ca03000-7f288ca05000 rw-p 00202000 08:01 6456                       /usr/lib/x86_64-linux-gnu/libc.so.6
 * 7f288ca05000-7f288ca12000 rw-p 00000000 00:00 0 
 * 7f288cbd1000-7f288cbd4000 rw-p 00000000 00:00 0 
 * 7f288cbdd000-7f288cbe0000 rw-p 00000000 00:00 0 
 * 7f288cbe0000-7f288cbe4000 r--p 00000000 00:00 0                          [vvar]
 * 7f288cbe4000-7f288cbe6000 r--p 00000000 00:00 0                          [vvar_vclock]
 * 7f288cbe6000-7f288cbe8000 r-xp 00000000 00:00 0                          [vdso]
 * 7f288cbe8000-7f288cbe9000 r--p 00000000 08:01 6453                       /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
 * 7f288cbe9000-7f288cc14000 r-xp 00001000 08:01 6453                       /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
 * 7f288cc14000-7f288cc1e000 r--p 0002c000 08:01 6453                       /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
 * 7f288cc1e000-7f288cc20000 r--p 00036000 08:01 6453                       /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
 * 7f288cc20000-7f288cc22000 rw-p 00038000 08:01 6453                       /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
 * 7ffed314d000-7ffed316f000 rw-p 00000000 00:00 0                          [stack]
 * ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
 * AUTO-GENERATED RUN OUTPUT END
 */
