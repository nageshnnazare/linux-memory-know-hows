/*
 * 04_pagemap.c
 * -------------
 * Translate a virtual address to its physical frame number using
 * /proc/self/pagemap.
 *
 * /proc/self/pagemap is an array indexed by virtual page number:
 *   vpn = VA / pagesize;     offset = vpn * 8;     read 8 bytes
 *
 * 64-bit entry layout (from Documentation/admin-guide/mm/pagemap.rst):
 *
 *   bit  63   present
 *   bit  62   swapped out
 *   bit  61   page is file-backed or shared anonymous
 *   bit  56   exclusively mapped (since 4.2)
 *   bit  55   pte soft-dirty
 *   bits 0..54 if present: physical frame number (PFN)
 *              if swapped:  swap type [0..4] | offset [5..54]
 *
 * NOTE: since Linux 4.0 the PFN is masked out for non-root users unless
 *       CAP_SYS_ADMIN is held -- you'll see 0.  Run with sudo to see PFNs.
 *
 * Build:  gcc -O0 -g 04_pagemap.c -o 04_pagemap
 * Run :   sudo ./04_pagemap     (for real PFN values)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/mman.h>

static uint64_t pa_of(void *va)
{
    long pg = sysconf(_SC_PAGESIZE);
    uint64_t vpn = (uintptr_t)va / pg;
    int fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd < 0) { perror("open"); exit(1); }
    uint64_t entry = 0;
    if (pread(fd, &entry, 8, vpn * 8) != 8) { perror("pread"); exit(1); }
    close(fd);
    return entry;
}

static void describe(const char *label, void *va)
{
    uint64_t e = pa_of(va);
    int present = (e >> 63) & 1;
    int swap    = (e >> 62) & 1;
    int filebak = (e >> 61) & 1;
    int excl    = (e >> 56) & 1;
    uint64_t pfn= e & ((1ULL << 55) - 1);

    printf("%-22s  va=%p  present=%d swap=%d filebak=%d excl=%d  pfn=0x%" PRIx64 "\n",
           label, va, present, swap, filebak, excl, pfn);
}

int main(void)
{
    int stack_var;
    static int bss_var;
    static int data_var = 42;
    char *heap = malloc(4096); *heap = 1;
    char *anon = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    *anon = 1; /* fault it in */

    describe("stack",        &stack_var);
    describe("bss",          &bss_var);
    describe("data",         &data_var);
    describe("malloc heap",  heap);
    describe("mmap anon",    anon);
    describe("text (main)",  (void *)main);

    free(heap); munmap(anon, 4096);
    return 0;
}
