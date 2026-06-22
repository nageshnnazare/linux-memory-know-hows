# 02 — The Process Address Space

When the kernel does `execve("a.out", argv, envp)` it constructs a brand-new
virtual address space for your process out of pieces of the ELF file, libc,
the kernel's `vdso`, and a generous helping of zero-filled anonymous memory.
This section is the *map* of that space.

## 1. ELF — what's in the file

```
   a.out
   +--------------------------------+
   | ELF header  (entry point, ...) |
   +--------------------------------+
   | program headers  (LOAD, ...)   |  <- the loader reads these
   +--------------------------------+
   | .text     (code, RX)           |
   | .rodata   (strings, R-)        |  packed into a "LOAD" segment
   +--------------------------------+
   | .data     (init globals, RW)   |  another LOAD segment
   | .bss      (zero-init, RW)      |  zero-filled by kernel, not on disk
   +--------------------------------+
   | section headers (linker info)  |
   +--------------------------------+
```

The kernel honours the **LOAD program headers**, not the sections. Run
`readelf -l a.out` to see them:

```
   LOAD off=0x000000 vaddr=0x400000 filesz=0x4f8 memsz=0x4f8 flags=R E
   LOAD off=0x000e10 vaddr=0x401e10 filesz=0x230 memsz=0x238 flags=RW
                                                       ^         ^
                                                       |         |
                                              filesz=on-disk    memsz=in-mem
                                              (the extra 8 bytes
                                               are .bss, kernel zeros them)
```

## 2. The standard segments after `execve`

```
   high addr  +---------------------------------+ 0x7FFFFFFFFFFF (canonical max)
              |  kernel half (not mapped to us) |
              +---------------------------------+ 0xFFFF800000000000 - 1
              |   non-canonical hole            |
              +---------------------------------+
              |  [stack]                        |   main thread stack (rw-)
              |     |                           |
              |     v                           |
              +---------------------------------+
              |  args + env + auxv at very top  |
              |  of the stack region            |
              +---------------------------------+
              |  ...                            |
              |  [vdso]    kernel shim          |
              |  [vvar]    kernel data          |
              |  [vsyscall] (legacy)            |
              |  /lib/libc.so.6 segments        |
              |  /lib/ld-linux.so.2 segments    |
              |  arbitrary anonymous mmap       |   thread stacks land here too
              |  ...                            |
              +---------------------------------+
              |  [heap]                         |   ptmalloc grows this with brk
              |     ^                           |
              |     |                           |
              +---------------------------------+
              |  .bss   (zero-filled)           |
              |  .data  (init globals)          |
              |  .rodata (strings, RO)          |
              |  .text  (code, RX)              |
   low addr   +---------------------------------+ 0x400000 (typical PIE/non-PIE)
              |  guard page at addr 0           |  *PROT_NONE*, NULL deref => SIGSEGV
              +---------------------------------+ 0
```

(With **PIE** — Position Independent Executable — the base is randomised by
ASLR; without PIE, it is `0x400000` on x86\_64.)

## 3. The auxiliary vector (auxv)

When the kernel sets up the stack it pushes:

```
   top of stack -----+
                     |  envp[]
                     |  argv[]
                     |  argc
                     |  ...
                     |  auxv[]  (key/value pairs the kernel gives us)
                     |  envp strings
                     |  argv strings
                     |  random data, AT_RANDOM, padding
```

You can read it via `getauxval(3)`:

```c
#include <sys/auxv.h>
unsigned long pgsz = getauxval(AT_PAGESZ);
unsigned long base = getauxval(AT_BASE);     /* ld.so load addr */
const char *exe   = (const char *)getauxval(AT_EXECFN);
```

## 4. `/proc/self/maps` — the truth

Every line is one **VMA** (Virtual Memory Area). Example:

```
   address                   perms offset   dev   inode      pathname
   ----------------          ----- ------- ----- ------     ---------------
   55b6a4400000-55b6a4401000 r--p  00000000  fd:01  393425   /home/x/a.out
   55b6a4401000-55b6a4402000 r-xp  00001000  fd:01  393425   /home/x/a.out
   55b6a4402000-55b6a4403000 r--p  00002000  fd:01  393425   /home/x/a.out
   55b6a4403000-55b6a4404000 r--p  00002000  fd:01  393425   /home/x/a.out
   55b6a4404000-55b6a4405000 rw-p  00003000  fd:01  393425   /home/x/a.out
   55b6a5b65000-55b6a5b86000 rw-p  00000000  00:00  0        [heap]
   7f9e10000000-7f9e10021000 rw-p  00000000  00:00  0
   7f9e10021000-7f9e14000000 ---p  00000000  00:00  0
   7f9e17af1000-7f9e17b13000 r--p  00000000  fd:01  524332   /lib/x86_64-linux-gnu/libc.so.6
   ...
   7ffd9e22d000-7ffd9e24e000 rw-p  00000000  00:00  0        [stack]
   7ffd9e2ad000-7ffd9e2b1000 r--p  00000000  00:00  0        [vvar]
   7ffd9e2b1000-7ffd9e2b3000 r-xp  00000000  00:00  0        [vdso]
   ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0   [vsyscall]
```

Columns:

| col | meaning |
|-----|---------|
| `address` | `start-end`, half-open `[start, end)` |
| `perms`   | `rwxp` or `rwxs` (`p`=private, `s`=shared); `-` = none |
| `offset`  | offset into the backing file (or 0 for anon) |
| `dev`     | `major:minor` of the device (or `00:00` for anon) |
| `inode`   | inode of the backing file (or `0` for anon) |
| `pathname`| `/path/to/file`, `[heap]`, `[stack]`, `[vdso]`, ... |

> Pro tip: pause your program with `pause()` or `getchar()` and inspect from
> another terminal. See [`03_dump_maps.c`](03_dump_maps.c).

### Why so many segments per ELF file?

Modern ELF + ld separate read-only data (`r--p`), code (`r-xp`), relocations
(another `r--p`), and writable data (`rw-p`) so each can get the tightest
permissions. The kernel maps each LOAD header as its own VMA.

## 5. ASLR — Address Space Layout Randomisation

By default Linux randomises:

1. The mmap base (libraries, thread stacks).
2. The brk base (small offset above `.bss` if `randomize_va_space >= 1`).
3. The main stack base.
4. The PIE executable base (`/proc/sys/kernel/randomize_va_space=2`).

Disable for one process: `setarch $(uname -m) -R ./prog`
System-wide: `sudo sysctl -w kernel.randomize_va_space=0` (don't do this in
production!).

See [`04_aslr.c`](04_aslr.c) which prints addresses across multiple runs.

## 6. Where things actually live — empirically

Run [`01_segments_addresses.c`](01_segments_addresses.c) and you'll see
something like:

```
   .text     (code)     0x55a8a8bb6149    <-- low
   .rodata   (const)    0x55a8a8bb7008
   .data     (globals)  0x55a8a8bb9020
   .bss      (uninit)   0x55a8a8bb9034
   heap      (malloc)   0x55a8aa3b1010   <-- brk-managed
   mmap anon            0x7f23e6cb5000
   stack     (locals)   0x7ffd1233a55c   <-- high
```

Notice the **giant** gap between heap and mmap — that's the address-space
hole reserved so the heap can grow up and mmap can grow down without
colliding (until they meet — which is when `brk(2)` will fail).

## 7. The `vdso` and `vvar`

To make `gettimeofday`, `clock_gettime`, and a few other "trivial" syscalls
faster, the kernel maps a small ELF (the **vDSO**) into every process. Calling
`clock_gettime(CLOCK_MONOTONIC, ...)` does not actually trap into the kernel
— it reads a counter out of the `[vvar]` page.

```
   user calls clock_gettime(...)
        |
        v
   glibc resolves via auxv (AT_SYSINFO_EHDR) -> [vdso] entry
        |
        v
   [vdso] code reads [vvar] (no syscall!) and returns
```

This is one reason measuring `clock_gettime` shows < 30 ns per call.

## 8. Files in this folder

| File | Demonstrates |
|------|--------------|
| [`01_segments_addresses.c`](01_segments_addresses.c) | Print addresses of variables in text/rodata/data/bss/heap/mmap/stack |
| [`02_environ.c`](02_environ.c)                       | Walk `environ`, `argv`, and the auxv |
| [`03_dump_maps.c`](03_dump_maps.c)                   | Read and pretty-print `/proc/self/maps` |
| [`04_aslr.c`](04_aslr.c)                             | Show ASLR by printing key addresses each run |
| [`05_rlimit.c`](05_rlimit.c)                         | Read RLIMIT_STACK, RLIMIT_AS, RLIMIT_DATA, RLIMIT_NOFILE |
