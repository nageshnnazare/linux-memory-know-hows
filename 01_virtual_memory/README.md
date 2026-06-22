# 01 — Virtual Memory, Paging, and the MMU

This is the engine room. Every memory access your C program makes goes through
the **MMU** (Memory Management Unit), a piece of silicon that converts a
**virtual address** (what your pointer holds) into a **physical address**
(actual DRAM bytes). The translation table is the **page table**.

We focus on **x86\_64** with 4 KiB pages and 4 levels of paging, which is what
desktop / server Linux runs on. Concepts (huge pages, levels, TLB) apply on
ARM64 too.

## 1. The virtual address split

A 64-bit virtual address is not really 64 bits. Today's x86\_64 implementations
use **48-bit** canonical addresses (the upper 16 bits must be a sign-extension
of bit 47). Bit-by-bit:

```
   63                 47       39       30       21       12       0
   |                  |        |        |        |        |        |
   +---------+--------+--------+--------+--------+--------+--------+
   | sign ext| PML4   | PDPT   | PD     | PT     | page offset    |
   | bits 63..48      | 9 bits | 9 bits | 9 bits | 9 bits | 12 b  |
   +------------------+--------+--------+--------+--------+--------+
        |                  |       |       |       |          |
        +-- must equal     |       |       |       |          +--> byte within
            bit 47         |       |       |       |               4 KiB page
                           |       |       |       +-> index into PT
                           |       |       +-> index into Page-Dir
                           |       +-> index into PDPT
                           +-> index into top-level PML4
```

- **PML4** = Page Map Level 4. The root table. 512 entries × 8 bytes = 4 KiB.
- Each entry points to the next-level table's physical address (and has
  permission bits).
- A leaf entry's physical frame base + offset[11:0] = physical address.

### Page-table entry (PTE) layout (simplified)

```
   bit  63 62..52 51 .... 12 11..9   8  7  6  5  4  3  2  1  0
        +---+------+--------+-----+--+--+--+--+--+--+--+--+--+
        | NX|  hw  |  PFN   | sw  | G|PS| D| A|PCD|PWT|U/S|R/W|P|
        +---+------+--------+-----+--+--+--+--+--+--+--+--+--+
         |    res  | phys   | OS  |  |  |  |  |   |   |    |  |
         |         | frame# | use |  |  |  |  |   |   |    |  +-> Present
         |                  |     |  |  |  |  |   |   |    +-> Read/Write
         |                  |     |  |  |  |  |   |   +-> User/Supervisor
         |                  |     |  |  |  |  |   +-> Cache Disable
         |                  |     |  |  |  |  +-> Write-Through
         |                  |     |  |  |  +-> Accessed (set by HW)
         |                  |     |  |  +-> Dirty   (set by HW on write)
         |                  |     |  +-> Page Size (1 = huge page here)
         |                  |     +-> Global (don't flush on CR3 reload)
         +-> No-eXecute (1 = data, faults if you jump here)
```

`U/S=0` means kernel-only — touching from CPL=3 raises `#PF` (page fault).
`R/W=0` makes the page read-only. `NX=1` makes the page non-executable. Linux
sets these flags when you call `mprotect` (see
[`../11_memory_protection`](../11_memory_protection/)).

## 2. Walking the page tables

```
   virtual addr  0x00007FFE_DEAD_C0DE
        |
        |  CR3 register holds the physical address of THIS process's PML4.
        v
   +----------------+  PML4[idx9] -> PDPT physical addr
   |  PML4 (4 KiB)  | --------------------------------------+
   +----------------+                                       |
                                                            v
                                       +----------------+ PDPT[idx9] -> PD addr
                                       |  PDPT (4 KiB)  | -------------+
                                       +----------------+              |
                                                                       v
                                                  +----------------+ PD[idx9] -> PT
                                                  |  PD  (4 KiB)   | -----+
                                                  +----------------+      |
                                                                          v
                                              +----------------+ PT[idx9] -> frame
                                              |  PT  (4 KiB)   | -----+
                                              +----------------+      |
                                                                      v
                                                              +----------------+
                                                              | 4 KiB frame    |
                                                              +-+--------------+
                                                                |
                                                                v
                                                       offset[11:0] is added
                                                       to get physical addr
```

So each load/store the CPU *might* do 4 extra memory accesses to walk the
tables. That would be catastrophic, which is why we have the…

## 3. TLB — Translation Lookaside Buffer

A small fully-associative cache **inside the MMU** that maps recently used
virtual page numbers → physical frame numbers.

```
   CPU asks for VA -> MMU
        |
        v
   +------------+   hit (~0 extra cycles)
   |    TLB     | -----> PA -> data
   +------------+
        | miss
        v
   page-table walk (the diagram above; uses caches but can DRAM-bounce)
        |
        v
   install translation in TLB; retry the access
```

- L1 dTLB on a modern x86 is ~64 entries for 4 KiB pages, ~32 for 2 MiB.
- An L2 unified TLB can hold ~1500+ entries.
- A **TLB flush** happens when CR3 changes (context switch) — except for
  **global** pages (kernel mappings) — or when you call `invlpg`/`mprotect`
  /`munmap` to invalidate a range.

```
   TLB lifetime:
     write to CR3   ......... TLB flushed (except global)
     INVLPG addr    ......... evict one entry
     munmap, mprotect ......  kernel issues INVLPG (and IPI on SMP)
```

## 4. Huge pages

Instead of `(level3 leaf) -> 4 KiB frame`, the CPU can stop at level 2 with
`PS=1` and treat the next 21 bits as the offset into a **2 MiB frame**.
Similarly level 1 + `PS=1` = 1 GiB page.

```
   4 KiB page (default):  walk 4 levels, 4 KiB granularity
   2 MiB page:            walk 3 levels, 2 MiB granularity, fewer TLB entries
   1 GiB page:            walk 2 levels, 1 GiB granularity, almost free TLB
```

Two ways to get them on Linux:

1. **Transparent Huge Pages (THP)** — kernel quietly promotes whole 2 MiB
   regions into huge pages when possible. `cat /sys/kernel/mm/transparent_hugepage/enabled`.
2. **Explicit HugeTLB** — `mmap(..., MAP_HUGETLB, ...)` or `hugetlbfs`. You
   must reserve them up front via `/proc/sys/vm/nr_hugepages`.

See [`../13_advanced/02_huge_tlb.c`](../13_advanced/02_huge_tlb.c).

## 5. Page faults

A page fault (`#PF` on x86) happens when the MMU cannot complete a translation
because the PTE is "not present" or the access is not allowed by the
permission bits.

```
   CPU access  -> MMU walk -> PTE present?    perms ok?     => OK, do it
        |          |              | no            | no
        |          v              v               v
        |  raise #PF (CR2 = faulting VA, error code = R/W, U/S, ...)
        |          |
        |          v
        |  kernel's do_page_fault() runs
        |          |
        |          +-- valid VMA?  (was it ever mmap'd / brk'd?)
        |          |     no  -> SIGSEGV to the process
        |          |     yes
        |          +-- access permitted by VMA flags?
        |          |     no  -> SIGSEGV or SIGBUS
        |          |     yes
        |          +-- "soft" path:
        |          |     minor fault: allocate a frame (or share zero page),
        |          |     fill in PTE, return
        |          +-- "swapped"     -> read from swap (major fault)
        |          +-- "file-backed" -> read from file (major fault)
        +-> instruction restarted
```

Two kinds of "successful" faults:

- **Minor fault** = no disk I/O. The kernel allocates a zero page or maps the
  shared zero page. `malloc`'d pages produce minor faults on first touch.
- **Major fault** = disk I/O was required (swap-in or first read of an
  mmap'd file).

Count them with `getrusage(RUSAGE_SELF, &r)`:

```c
struct rusage r;
getrusage(RUSAGE_SELF, &r);
printf("minor=%ld major=%ld\n", r.ru_minflt, r.ru_majflt);
```

See [`03_count_page_faults.c`](03_count_page_faults.c).

## 6. The zero page

Anonymous pages (e.g. fresh `mmap(NULL, sz, ANON|PRIVATE)` or freshly-bss'd
globals) are *not* allocated until you read or write them. On the first
**read**, Linux maps the global **zero page** read-only — a single 4 KiB frame
full of zeros, shared by every process. On the first **write**, the kernel
allocates a real frame and updates the PTE.

```
   mmap(NULL, 4 GiB, PROT_RW, ANON|PRIVATE, -1, 0);

   step 1 — return from mmap:    VMA created, but no PTEs
                                 (Vsize += 4 GiB, RSS unchanged)
   step 2 — read p[0]:           PTE installed pointing to the zero page,
                                 R-O, shared across processes
                                 (RSS unchanged; reads of zero are "free")
   step 3 — write p[0]:          CoW -> new private frame allocated,
                                 PTE updated, R/W
                                 (RSS += 4 KiB)
```

This is how Linux supports `malloc(1 GiB)` on a 512 MiB machine: it only
breaks when you actually *touch* enough pages.

## 7. Copy-on-write

The other place pages become read-only-shared is right after `fork`. We will
detail it in [`../08_process_syscalls/01_fork.c`](../08_process_syscalls/),
but the picture is:

```
   before fork:
     parent VMA -> [F1][F2][F3]    (PTEs RW)

   after fork (no copies yet):
     parent VMA -> [F1][F2][F3]    (PTEs RO, refcount=2 per frame)
     child  VMA -> [F1][F2][F3]    (PTEs RO, refcount=2 per frame)

   child writes to page F2 -> #PF -> kernel allocates F2' for child:
     parent VMA -> [F1][F2][F3]
     child  VMA -> [F1][F2'][F3]
```

## 8. Per-process page tables

Each process has its **own** PML4. `CR3` is loaded at every context switch.
Two processes can share *some* frames (CoW, `MAP_SHARED`, shared libraries)
but each has its own page tables.

```
   process A
     CR3 -> PML4 A -> ... -> [F7]
                              ^
                              | refcnt=2 because shared lib
                              v
   process B
     CR3 -> PML4 B -> ... -> [F7]
```

## 9. Walking your own page tables (Linux-only)

Linux exposes a partial view of your page tables via `/proc/self/pagemap`
(read-only since 4.0). Each VA maps to a 64-bit entry:

```
   bits 0..54   physical frame number (if present)
   bit 55       swap?
   bit 56       file/shared?
   bit 61       page is exclusively mapped?
   bit 62       swapped out?
   bit 63       present?
```

See [`04_pagemap.c`](04_pagemap.c) to translate the address of a local
variable to a PFN.

## 10. Pointers to remember

```
   * 4 KiB page, 64-byte cache line, 8-byte word -- on x86_64
   * Virtual addr -> 4-level table -> physical frame
   * TLB caches translations; flushed on CR3 reload, INVLPG, munmap, mprotect
   * Page fault = MMU asks kernel for help
   * Minor faults are cheap (frame alloc); major faults touch disk
   * fresh anonymous pages are MAPPED LAZILY to the zero page
   * fork is cheap because of copy-on-write
```

## Files in this folder

| File | Demonstrates |
|------|--------------|
| [`01_pagesize.c`](01_pagesize.c)             | `sysconf(_SC_PAGESIZE)`, page-aligned mmap regions |
| [`02_lazy_allocation.c`](02_lazy_allocation.c)| `mmap` 1 GiB and watch RSS climb only on first write |
| [`03_count_page_faults.c`](03_count_page_faults.c) | `getrusage` to count minor / major faults |
| [`04_pagemap.c`](04_pagemap.c)               | Translate a virtual addr to its PFN via `/proc/self/pagemap` |
