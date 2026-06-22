# Linux Memory Management in C — The Complete Guide

A deep, hands-on, single-stop guide to **how memory really works** on a Linux
system from the perspective of a C programmer. Every concept is explained with:

1. **Theory** — what the kernel does, what the CPU does, what `glibc` does.
2. **ASCII diagrams** — almost every page contains pictures so you can *see*
   what is happening.
3. **Compilable C code** — small, self-contained programs you can run, inspect
   with `/proc`, debug with `gdb`, and observe with `strace`/`perf`.

> **Audience.** You know C, you can compile and run programs, you have shaky
> intuition about *what* `malloc` does, *where* the stack lives, *why* `fork`
> doesn't duplicate physical RAM, and *how* signals interrupt your code. By the
> end of this guide you will be able to reason about every byte your process
> touches.

```
              +-----------------------------------------------+
              |   ONE-STOP GUIDE: LINUX MEMORY MANAGEMENT     |
              |          (kernel + glibc + hardware)          |
              +-----------------------------------------------+
                                |
        +-----------------+-----+-----+-----------------+
        |                 |           |                 |
   +----v-----+      +----v-----+ +---v------+    +-----v-----+
   |  CPU /   |      | virtual  | |  process |    | allocators|
   |  MMU /   |      |  memory  | |  address |    | (malloc,  |
   |  caches  |      |  pages   | |  space   |    |  mmap,    |
   |          |      |  TLB     | |  /proc   |    |  arenas)  |
   +----+-----+      +----+-----+ +---+------+    +-----+-----+
        |                 |           |                 |
        +------+----------+-----+-----+--------+--------+
               |                |              |
         +-----v------+   +-----v-----+   +----v-------+
         | syscalls   |   |  threads  |   |    IPC     |
         | fork/exec  |   |  pthreads |   |  shm/pipe  |
         | clone/wait |   |  TLS/futex|   |  socket    |
         +------------+   +-----------+   +------------+
```

---

## Table of Contents

| #  | Section                                                                                 | What you will learn |
|----|-----------------------------------------------------------------------------------------|---------------------|
| 00 | [Fundamentals](00_fundamentals/)                                                        | Memory hierarchy, cache lines, MMU, virtual vs physical, words & alignment, endianness |
| 01 | [Virtual Memory & Paging](01_virtual_memory/)                                           | 4-level x86\_64 page tables, TLB, page faults, demand paging |
| 02 | [Process Address Space](02_process_address_space/)                                      | text/data/bss/heap/stack/mmap segments, `/proc/self/maps`, ASLR, environment & argv |
| 03 | [The Stack](03_stack/)                                                                  | Frames, calling conventions, `alloca`, VLAs, guard pages, canaries, stack overflow |
| 04 | [The Heap & `malloc`](04_heap_malloc/)                                                  | `ptmalloc2` internals — chunks, bins, arenas, fastbins, tcache; `brk` vs `mmap`; UB demos |
| 05 | [`mmap`](05_mmap/)                                                                      | Anonymous & file mappings, `MAP_SHARED` / `MAP_PRIVATE`, `madvise`, `mprotect`, `mremap`, huge pages |
| 06 | [`brk` / `sbrk`](06_brk_sbrk/)                                                          | The program break, low-level heap, why `malloc` uses it |
| 07 | [Paging, Swapping & OOM](07_paging_swapping/)                                           | Page cache, swap, `mlock`, `madvise(DONTNEED)`, OOM killer, `oom_score_adj` |
| 08 | [Process Syscalls](08_process_syscalls/)                                                | `fork` / `vfork` / `clone` / `execve` / `wait*`, signals, zombies, orphans, daemonization |
| 09 | [Threads & Memory](09_threads_memory/)                                                  | Where thread stacks live, TLS (`__thread`, `thread_local`), futex, raw `clone` for threads |
| 10 | [IPC & Shared Memory](10_ipc_memory/)                                                   | POSIX & SysV `shm`, pipes, FIFOs, UNIX sockets, message queues, `memfd_create`, `eventfd` |
| 11 | [Memory Protection](11_memory_protection/)                                              | `mprotect`, NX / W^X, ASLR, stack canaries, `sigaltstack`, handling `SIGSEGV` |
| 12 | [Debugging & Observation](12_debugging_tools/)                                          | `/proc/self/{maps,smaps,status,stat}`, `mallinfo2`, `getrusage`, `strace`, `valgrind`, `perf` |
| 13 | [Advanced](13_advanced/)                                                                | NUMA, transparent & explicit huge pages, `userfaultfd`, `memfd`, anonymous file-backed memory |
| 14 | [Putting it Together: Custom Allocators](14_putting_it_together/)                       | Bump, pool, freelist, slab, arena, buddy — build your own `malloc` |
| C  | [CHEATSHEET.md](CHEATSHEET.md)                                                           | One-page reference of every syscall, every `/proc` file, every flag |

---

## The Big Picture: What lives where?

Before anything else, lock this picture in your head. Everything in this guide
is an elaboration of it.

```
   PHYSICAL RAM (a flat array of bytes, addressed by the memory controller)
   +-------------------------------------------------------------+
   |  frame 0 | frame 1 | frame 2 | ... | frame N (4 KiB each)   |
   +-------------------------------------------------------------+
                              ^
                              | the MMU maps virtual pages -> frames
                              | using per-process page tables
                              v
   PROCESS VIRTUAL ADDRESS SPACE (one per process, 256 TiB on x86_64)
   0xFFFFFFFFFFFFFFFF +----------------------------+   <- top of canonical addr
                      |   KERNEL SPACE             |      (not visible to user)
                      |   (mapped in every proc,   |
                      |    protected by CPL)       |
   0xFFFF800000000000 +----------------------------+
                      |        (non-canonical      |
                      |          hole)             |
   0x00007FFFFFFFFFFF +----------------------------+
                      |       STACK (grows down)   |   <- main thread stack
                      |          |  |              |
                      |          v  v              |
                      |                            |
                      |   (mmap region — libraries,|
                      |    anonymous mmap, thread  |
                      |    stacks live here)       |
                      |                            |
                      |          ^  ^              |
                      |          |  |              |
                      |       HEAP (grows up)      |   <- brk-managed
                      +----------------------------+
                      |    BSS  (zero-init data)   |
                      +----------------------------+
                      |    DATA (init globals)     |
                      +----------------------------+
                      |    TEXT (code, read-only)  |
   0x0000000000400000 +----------------------------+   <- typical ELF base
                      |  (unmapped, NULL guard)    |
   0x0000000000000000 +----------------------------+
```

Three big ideas:

1. **Virtual ≠ Physical.** Two processes can both have address `0x400000` and
   they point to *different* physical RAM (thanks to the MMU + per-process page
   tables).
2. **Memory is lazy.** `malloc(1 GiB)` does *not* allocate 1 GiB of RAM. It
   reserves virtual pages; physical frames are only attached when you touch
   them (a page fault).
3. **Everything is a file or a syscall.** Loading a program (`execve`),
   growing the heap (`brk`/`mmap`), creating a thread (`clone`), sharing
   memory (`mmap` of `/dev/shm/...`) — all of these are syscalls. There is no
   magic; just well-defined interfaces between you and the kernel.

---

## How to use this guide

```
   +----------------+        +---------------+        +-----------------+
   |  Read the      |   ->   |  Run the      |   ->   |  Inspect with   |
   |  section's     |        |  examples;    |        |  /proc, gdb,    |
   |  README first. |        |  edit them.   |        |  strace, perf.  |
   +----------------+        +---------------+        +-----------------+
                                       |
                                       v
                              +-----------------+
                              |  Sketch a       |
                              |  diagram of     |
                              |  what happened  |
                              |  in your head.  |
                              +-----------------+
```

Every program in this repo is **small on purpose**. They are not "real" code;
they are *demos* designed to make one concept visible at a time. Read the
source, run them, then poke them with `strace -f -e trace=memory ./a.out` or
`cat /proc/$$/maps` while the program waits in a `getchar()`.

### Building

This guide is written for **Linux x86\_64 with glibc**. Some examples (`brk`,
`/proc/self/maps`, `madvise(MADV_DONTNEED)`, ...) are Linux-specific and will
not run on macOS or BSD. To build everything:

```bash
make                 # from the repo root, builds every subdirectory
make -C 04_heap_malloc run   # run all heap demos
make clean
```

Each section has its own `Makefile`. Each example uses only the standard glibc
headers; no third-party libraries are required.

### Recommended compile flags while learning

```bash
gcc -std=gnu11 -Wall -Wextra -O0 -g3 prog.c -o prog        # readable asm
gcc -std=gnu11 -O0 -g3 -fsanitize=address prog.c -o prog   # heap bugs
gcc -std=gnu11 -O0 -g3 -fsanitize=undefined prog.c -o prog # UB
gcc -std=gnu11 -O2 -g  prog.c -o prog                      # what optimizers see
```

When learning **threads** the right sanitizer is `-fsanitize=thread`. When
learning **memory** the right one is `-fsanitize=address` (and Valgrind for
deeper inspection).

---

## Suggested learning path

```
  Beginner                    Intermediate                Advanced
  --------                    ------------                --------
  00_fundamentals
       |
       v
  02_process_address_space  ----> /proc/self/maps deep-dive
       |
       v
  03_stack  --------------+
       |                  |
       v                  v
  04_heap_malloc      05_mmap
       |                  |
       +--------+---------+
                |
                v
       06_brk_sbrk + 01_virtual_memory
                |
                v
       08_process_syscalls  --> 09_threads_memory
                |                       |
                v                       v
       10_ipc_memory            11_memory_protection
                |                       |
                +-----------+-----------+
                            |
                            v
                  07_paging_swapping
                            |
                            v
                  13_advanced   12_debugging_tools
                            |
                            v
                  14_putting_it_together
```

---

## The Four Mental Models

If you internalize these four pictures, everything else clicks.

### M1 — A `malloc` is two syscalls in disguise

```
   user code:        glibc:            kernel:
   --------          ------            -------
   malloc(40)  --->  ptmalloc          (no syscall — served from
                     finds free chunk   the cached arena)
                     in a bin
                     ^
                     | when no chunk fits, glibc grows the heap:
                     |
                     +-- small  --> brk(new_break)
                     +-- big    --> mmap(NULL, sz, ANON|PRIVATE, -1, 0)
```

### M2 — A page is the unit of allocation

```
   You ask for 1 byte;  kernel always gives you a whole page (4 KiB).
   You ask for 5 KiB ;  kernel gives two pages (8 KiB).
   You "free" memory ;  kernel may or may not actually take it back.

   page = 4 KiB on x86_64 (configurable: 2 MiB / 1 GiB huge pages)
```

### M3 — `fork` shares pages, copy-on-write

```
   BEFORE fork():                 AFTER fork() (zero copy yet):
   parent  -> [pg A][pg B][pg C]  parent  ->\
                                              [pg A][pg B][pg C]   (RO)
                                  child   ->/
                                  ^ both processes share the SAME frames,
                                    all marked read-only. Any write triggers
                                    a page fault and the kernel copies that
                                    one page. This is "copy on write".
```

### M4 — Threads share everything except stack + registers + TLS

```
   PROCESS                                                      kernel sees this as
   +----------------------------------------------------+      ONE address space,
   | heap, globals, code, fds, mmap regions  -- SHARED  |      multiple "tasks"
   |                                                    |      sharing it via the
   |  +-----------+   +-----------+   +-----------+     |      CLONE_VM flag of
   |  | thread 1  |   | thread 2  |   | thread 3  |     |      clone(2).
   |  |  stack    |   |  stack    |   |  stack    |     |
   |  |  regs/PC  |   |  regs/PC  |   |  regs/PC  |     |
   |  |  TLS      |   |  TLS      |   |  TLS      |     |
   |  +-----------+   +-----------+   +-----------+     |
   +----------------------------------------------------+
```

---

## Conventions used throughout

- `[X]` in a diagram = one page (4 KiB) of memory.
- `RW` / `R-` / `RX` next to a region = current permissions (write/read/execute).
- `^` and `v` arrows show the *direction of growth*.
- All numeric examples use **x86\_64** unless explicitly noted (`pagesize=4096`,
  `sizeof(void*)=8`).
- Source-line citations use the standard syntax `file.c:line`.

Happy hacking — and remember: **the kernel is on your side, but only if you
ask it the right questions**. Every example in this guide is, ultimately, a
demonstration of that single fact.
