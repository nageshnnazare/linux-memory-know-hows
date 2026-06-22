/*
 * 01_mprotect_jit.c
 * ------------------
 * A tiny "JIT".  We allocate an anonymous page RW, write x86_64 machine
 * code into it, flip to RX, and call the result as a function.
 *
 * The encoded function is:
 *     int f(int a, int b) { return a + b; }
 *
 *   x86_64 SysV ABI:
 *     args:    rdi = a, rsi = b
 *     return:  rax
 *
 *   asm:
 *     mov %rdi, %rax   ; 48 89 f8     (mov rax, rdi)
 *     add %rsi, %rax   ; 48 01 f0     (add rax, rsi)
 *     ret              ; c3
 *
 *  Run with:  ./01_mprotect_jit
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int main(void)
{
    long pg = sysconf(_SC_PAGESIZE);
    void *p = mmap(NULL, pg, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }

    unsigned char code[] = {
        0x48, 0x89, 0xf8,       /* mov rax, rdi */
        0x48, 0x01, 0xf0,       /* add rax, rsi */
        0xc3                    /* ret           */
    };
    memcpy(p, code, sizeof code);

    /* Flip to read+execute -- W^X: never have W AND X at the same time */
    if (mprotect(p, pg, PROT_READ | PROT_EXEC) < 0) { perror("mprotect"); return 1; }

    int (*f)(int, int) = p;
    int r = f(3, 4);
    printf("JIT'd f(3,4) = %d\n", r);

    munmap(p, pg);
    return 0;
}
