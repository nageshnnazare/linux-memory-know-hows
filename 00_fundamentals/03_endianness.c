/*
 * 03_endianness.c
 * ----------------
 * Detect endianness by writing a known integer and reading the underlying
 * bytes.
 *
 *   uint32_t x = 0x11223344;
 *
 *   little-endian memory:           big-endian memory:
 *     addr+0: 0x44                    addr+0: 0x11
 *     addr+1: 0x33                    addr+1: 0x22
 *     addr+2: 0x22                    addr+2: 0x33
 *     addr+3: 0x11                    addr+3: 0x44
 *
 *   x86_64, ARM (default), RISC-V -> little endian
 *   POWER (big mode), older SPARC -> big endian
 */
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    uint32_t x = 0x11223344;
    unsigned char *p = (unsigned char *)&x;

    printf("uint32_t x = 0x%08x at %p\n", x, (void *)&x);
    for (size_t i = 0; i < sizeof x; ++i)
        printf("  byte at +%zu = 0x%02x\n", i, p[i]);

    if (p[0] == 0x44)      puts("=> LITTLE endian");
    else if (p[0] == 0x11) puts("=> BIG endian");
    else                   puts("=> exotic endian (PDP?)");

    /* htonl/ntohl exist for exactly this reason -- network byte order is BE. */
    return 0;
}
