# 00 — Fundamentals: the silicon underneath

Before we ever talk about `malloc`, you have to know **what a byte is**, **where it
lives**, and **how the CPU finds it**. This section is the bedrock.

## 1. The memory hierarchy

A CPU is fast (a few hundred picoseconds per instruction); RAM is slow (~100
ns); disks are *very* slow (10 µs SSD, 10 ms spinning rust). The hardware
hides that gap with **caches**:

```
   CPU core
   +-------------------+        speeds (rough order of magnitude)
   |  registers        |         ~1 cycle    < 1 ns,    <1 KiB
   |                   |
   |  L1 d-cache       |         ~4 cycles   ~1 ns,    32 KiB / core
   |                   |
   |  L1 i-cache       |         ~4 cycles   ~1 ns,    32 KiB / core
   |                   |
   |  L2 cache         |         ~12 cycles  ~3 ns,    256 KiB - 1 MiB / core
   |                   |
   |  L3 cache         |         ~40 cycles  ~10 ns,   8-32 MiB / socket (shared)
   +-------------------+
            |
       memory bus
            |
   +-------------------+
   |  DRAM             |         ~200 cycles  ~80 ns,   GiBs
   +-------------------+
            |
       PCIe / NVMe / SATA
            |
   +-------------------+
   |  NVMe SSD         |         ~10 us,   TiBs
   |  SATA SSD         |         ~100 us
   |  Spinning disk    |         ~10 ms
   +-------------------+
```

Two consequences you must internalise:

1. **Locality is everything.** If your data fits in L1 you are ~100× faster
   than a worst-case miss-to-DRAM. Loops over arrays beat loops over linked
   lists not because pointers are slow but because they thrash the cache.
2. **The smallest unit of caching is a cache line (64 bytes on x86\_64).**
   Touching one byte loads the whole line. Two threads writing to two adjacent
   `int`s is *false sharing* — the line ping-pongs between cores.

```
   array of int32 (4 bytes each), aligned to 64-byte cache line:

   |  byte 0  ..  byte 63   |  byte 64 .. byte 127  | ...
   +------------------------+-----------------------+
   |a[0] a[1] a[2] ... a[15]|  a[16] a[17] ...      |
   +------------------------+-----------------------+
        ^                          ^
        |                          |
     one cache line             next cache line
```

## 2. Words, alignment, and endianness

A **word** is the natural integer size of the CPU. On x86\_64 it is 8 bytes
(64 bits). The CPU prefers to read aligned addresses: a load of an 8-byte
integer should sit on an 8-byte boundary.

```
   GOOD: 8-byte load at addr 0x1008          BAD: at 0x1009 (straddles)
   +------------------+                     +------------------+
   | 8B int at 0x1008 |                     | 7B... | 1B...    |
   +------------------+                     +------------------+
                                              spans two cache lines
                                              -> two loads internally
```

`malloc` on glibc returns memory aligned to `alignof(max_align_t)` = **16
bytes** on x86\_64. That is enough for any scalar type, plus SSE 128-bit
loads. For AVX-512 (64-byte vectors) you need `aligned_alloc(64, ...)` or
`posix_memalign`.

### Endianness

x86\_64 is **little-endian**: the least-significant byte sits at the lowest
address.

```
   uint32_t x = 0x11223344;        // value 287454020
   bytes at &x:   44 33 22 11        <- little-endian
                  ^         ^
           low addr   high addr
```

Run [`03_endianness.c`](03_endianness.c) to print the bytes of an int on your
machine.

## 3. Virtual memory vs physical memory

Every modern OS gives each process its **own** view of memory, an **address
space**. Two processes can both think they own address `0x400000` — and they
do, in their *virtual* world. The MMU (Memory Management Unit) translates
those virtual addresses to physical RAM frames at runtime.

```
   process A virtual         physical RAM           process B virtual
     0x400000  ----+              +----+              ----  0x400000
                   |  page tables |    |  page tables   |
                   +------------> | F1 | <------------- +
                                  |    |
     0x401000  ----+              +----+              ----  0x401000
                   |              |    |                |
                   +------------> | F2 |                +-> ... (different frame)
                                  +----+
```

Three vocabulary words you will see everywhere:

- **page**  — the virtual unit (4 KiB on x86\_64).
- **frame** — the physical unit (also 4 KiB; sometimes called *page frame*).
- **PFN**   — page frame number = `physical_addr >> 12`.

A page is **resident** when it is currently mapped to a frame, and **swapped
out** when its content lives on disk only. We will cover page tables and the
TLB in detail in [01_virtual_memory](../01_virtual_memory/).

## 4. The kernel/user split

Half of the canonical virtual address space is reserved for the kernel. On
x86\_64:

```
   0xFFFFFFFFFFFFFFFF +------------------+
                      |   kernel space   |   mapped in every process,
                      |   (CPL=0 only)   |   shared; user CPL=3 cannot read it
   0xFFFF800000000000 +------------------+
                      |  non-canonical   |   unmapped 47-bit hole
   0x0000800000000000 +------------------+
                      |   user space     |   per-process, your code
   0x0000000000000000 +------------------+
```

When you do a syscall (`fork`, `mmap`, `read`, ...) the CPU switches to ring 0
and the kernel half becomes addressable. The TLB does not have to be flushed
on every kernel entry — that is what the *global page* bit in the page tables
is for. We will see this again in [01_virtual_memory](../01_virtual_memory/).

## 5. The C type system and memory

```
   type          size on x86_64    alignment
   ----          --------------    ---------
   char             1                1
   short            2                2
   int              4                4
   long             8                8
   long long        8                8
   void *           8                8
   float            4                4
   double           8                8
   long double     16               16     (80-bit extended in 128-bit slot)
   _Complex double 16                8
   __m128          16               16     (SSE)
   __m256          32               32     (AVX)
   __m512          64               64     (AVX-512)
```

Use `_Alignof(T)` (C11) / `alignof(T)` to query, and `_Alignas(N)` /
`alignas(N)` to force.

```c
_Alignas(64) char line[64];     // force a single cache line of storage
```

Padding inside structs is inserted automatically:

```c
struct s { char a; int b; char c; };
//          ^      ^      ^
//   offset 0      4      8
//   sizeof = 12  (3 bytes of padding after a, 3 after c)
```

See [`02_word_alignment.c`](02_word_alignment.c) to inspect this with
`offsetof`.

## 6. Where to next?

```
   you are here
       |
       v
   00_fundamentals
       |
       v
   01_virtual_memory  --- next: how the MMU actually translates an address
       |
       v
   02_process_address_space
```

## Files in this folder

| File | What it shows |
|------|---------------|
| [`01_memory_hierarchy.c`](01_memory_hierarchy.c) | Measure L1 vs L2 vs DRAM by striding through arrays of growing size |
| [`02_word_alignment.c`](02_word_alignment.c)     | Print `sizeof`, `_Alignof`, and `offsetof` for a struct |
| [`03_endianness.c`](03_endianness.c)             | Print the bytes of an int to detect endianness |
| [`04_cacheline_falsesharing.c`](04_cacheline_falsesharing.c) | Show how 64-byte alignment removes false sharing |
