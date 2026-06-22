# Linux Memory Cheat Sheet

A single-page, printable summary. Pair with the rest of this repo.

## 1. The address-space map

```
   high addr  +-----------------------+
              | kernel (not mapped to |   user code can't touch this
              | user)                 |
              +-----------------------+
              | stack                 |   grows down, guard page below
              |   |  |                |
              |   v  v                |
              +-----------------------+
              | mmap / libs / thread  |   anon mmap + shared libs +
              | stacks                |   non-main thread stacks
              +-----------------------+
              |   ^  ^                |
              |   |  |                |
              | heap (brk)            |   grows up, ptmalloc managed
              +-----------------------+
              | bss   (zero-init)     |
              | data  (init globals)  |
              | text  (code, RX)      |
   low addr   +-----------------------+
```

## 2. Every syscall touching memory

| Syscall            | What it does                                                   | Header |
|--------------------|----------------------------------------------------------------|--------|
| `brk(addr)`        | Set program break to `addr`                                    | `<unistd.h>` |
| `sbrk(incr)`       | Grow program break by `incr`, return old break                 | `<unistd.h>` |
| `mmap(...)`        | Map a region (anonymous or file backed)                        | `<sys/mman.h>` |
| `munmap(p, len)`   | Unmap a region                                                 | `<sys/mman.h>` |
| `mprotect(p,l,pr)` | Change page permissions (`PROT_READ|WRITE|EXEC|NONE`)          | `<sys/mman.h>` |
| `mremap(...)`      | Resize / move a mapping (Linux only)                           | `<sys/mman.h>` |
| `madvise(p,l,a)`   | Hint to kernel: `MADV_DONTNEED`, `WILLNEED`, `SEQUENTIAL`, ... | `<sys/mman.h>` |
| `mlock/munlock`    | Pin pages in RAM (prevent swap)                                | `<sys/mman.h>` |
| `mlockall`         | Pin entire address space                                       | `<sys/mman.h>` |
| `msync(p,l,fl)`    | Flush a `MAP_SHARED` file mapping to disk                      | `<sys/mman.h>` |
| `shm_open(name,…)` | Open POSIX shared-memory object                                | `<sys/mman.h>` |
| `shm_unlink(name)` | Remove POSIX SHM name                                          | `<sys/mman.h>` |
| `memfd_create(…)`  | Anonymous file in RAM, get an fd                               | `<sys/mman.h>` |
| `userfaultfd()`    | Get an fd that delivers page-fault events                      | `<sys/syscall.h>` |
| `fork()`           | Duplicate process (CoW)                                        | `<unistd.h>` |
| `vfork()`          | Like fork() but parent suspended; child shares VM              | `<unistd.h>` |
| `clone(...)`       | Underlying syscall; pick what to share via flags               | `<sched.h>` |
| `execve(...)`      | Replace address space with new program                         | `<unistd.h>` |
| `wait/waitpid/waitid` | Reap a child                                                | `<sys/wait.h>` |
| `pipe2(fds,fl)`    | Create pipe (in-kernel buffer)                                 | `<unistd.h>` |
| `socketpair(...)`  | Bidirectional in-kernel pair (UNIX domain)                     | `<sys/socket.h>` |
| `eventfd(c,fl)`    | Counter you can read/write/poll                                | `<sys/eventfd.h>` |
| `signalfd(...)`    | Receive signals through an fd                                  | `<sys/signalfd.h>` |
| `prctl(...)`       | Per-process knobs (e.g. `PR_SET_DUMPABLE`)                     | `<sys/prctl.h>` |

## 3. `mmap` cheat-sheet

```c
void *p = mmap(addr,             // hint, usually NULL
               len,              // bytes; rounded up to page size
               PROT_READ|PROT_WRITE,   // access (PROT_NONE for guard)
               MAP_PRIVATE|MAP_ANONYMOUS,
               -1, 0);
```

| Flag             | Meaning |
|------------------|---------|
| `MAP_PRIVATE`    | Writes are CoW; not shared, not flushed to file |
| `MAP_SHARED`     | Writes visible to others mapping the same object; flushed to file |
| `MAP_ANONYMOUS`  | Not backed by a file; zero-filled                                 |
| `MAP_FIXED`      | Address must be `addr`. Dangerous (overwrites existing mappings)  |
| `MAP_FIXED_NOREPLACE` | Like FIXED but fails instead of clobbering                    |
| `MAP_HUGETLB`    | Use huge pages (2 MiB or 1 GiB)                                   |
| `MAP_POPULATE`   | Prefault all pages (no on-demand)                                 |
| `MAP_NORESERVE`  | Do not reserve swap (overcommit-friendly)                         |
| `MAP_STACK`      | Suitable for stack (architectures may align)                      |
| `MAP_GROWSDOWN`  | Stack-style auto-growth (legacy; rarely used directly)            |
| `MAP_LOCKED`     | Mark pages `mlock`ed (need `CAP_IPC_LOCK`)                        |

## 4. `madvise` quick reference

| Advice                 | Meaning |
|------------------------|---------|
| `MADV_NORMAL`          | Default                                       |
| `MADV_RANDOM`          | Random access — disable readahead             |
| `MADV_SEQUENTIAL`      | Sequential — be aggressive with readahead     |
| `MADV_WILLNEED`        | Prefetch                                      |
| `MADV_DONTNEED`        | Drop pages — next read returns zeroed pages   |
| `MADV_FREE`            | Mark freeable; kernel may reclaim lazily      |
| `MADV_HUGEPAGE`        | Try to back with THP                          |
| `MADV_NOHUGEPAGE`      | Avoid THP                                     |
| `MADV_DONTFORK`        | Region not inherited by `fork()`              |
| `MADV_WIPEONFORK`      | Zero this region in the child                 |
| `MADV_COLD`/`PAGEOUT`  | Hint pages aren't hot / push to swap          |

## 5. `/proc` files you must know

| File                                | Contains |
|-------------------------------------|----------|
| `/proc/self/maps`                   | All VMAs of this process (addr, perms, offset, dev, inode, path)|
| `/proc/self/smaps`                  | Per-VMA detailed memory accounting (Rss, Pss, Swap, ...)        |
| `/proc/self/status`                 | Vm{Peak,Size,RSS,Data,Stk,Exe,Lib}, threads, signals            |
| `/proc/self/stat`                   | Single-line stats (state, utime, stime, vsize, rss, ...)        |
| `/proc/self/statm`                  | Compact memory stats (size, resident, shared, text, ...)        |
| `/proc/self/cmdline`                | NUL-separated argv                                              |
| `/proc/self/environ`                | NUL-separated env                                               |
| `/proc/self/limits`                 | rlimits in human-readable form                                  |
| `/proc/meminfo`                     | System-wide RAM, swap, dirty, slab, ...                         |
| `/proc/buddyinfo`                   | Buddy allocator free-list lengths per order                     |
| `/proc/slabinfo`                    | Kernel slab cache stats (root)                                  |
| `/proc/sys/vm/overcommit_memory`    | 0=heuristic, 1=always, 2=strict                                 |
| `/proc/sys/vm/swappiness`           | 0–100, how aggressively to swap                                 |
| `/proc/sys/vm/drop_caches`          | Write 1/2/3 to drop page cache (debug only)                     |
| `/proc/PID/oom_score_adj`           | -1000..1000, OOM killer bias                                    |

## 6. Sizes & limits

```
sizeof(void*)            8           (on x86_64)
pagesize                 4096        (getpagesize() / sysconf(_SC_PAGESIZE))
THP                      2 MiB       (transparent huge page on x86_64)
HugeTLB                  2 MiB / 1 GiB
default thread stack     8 MiB       (ulimit -s)  glibc may set 8 MiB
default main stack       8 MiB       (RLIMIT_STACK)
canonical user VA range  0 .. 0x7FFFFFFFFFFF  (47 bits)
```

## 7. The `malloc` family

| Function                                  | Notes |
|-------------------------------------------|-------|
| `void *malloc(size_t)`                    | Uninitialised; aligned to `alignof(max_align_t)` (16 on x86_64 glibc) |
| `void *calloc(size_t n, size_t s)`        | Zeroed; checks `n*s` for overflow                                    |
| `void *realloc(void *p, size_t s)`        | Resize; may return new pointer; `realloc(p, 0)` historically UB      |
| `void free(void *p)`                      | `free(NULL)` is OK                                                   |
| `void *aligned_alloc(size_t a, size_t s)` | C11; `s` must be multiple of `a`; `a` must be a power of 2           |
| `int posix_memalign(void **p, size_t a, size_t s)` | POSIX; returns errno on failure                             |
| `void *memalign(size_t a, size_t s)`      | glibc legacy                                                          |
| `void *valloc(size_t s)`                  | page-aligned (deprecated)                                            |
| `void *pvalloc(size_t s)`                 | page-aligned, page-rounded (deprecated)                              |
| `void *reallocarray(void *p, size_t n, size_t s)` | realloc with overflow-checked n*s                            |

```c
#include <malloc.h>     // glibc-specific
mallopt(M_MMAP_THRESHOLD, 128*1024);  // tune mmap cutoff
mallopt(M_ARENA_MAX, 1);              // single arena
struct mallinfo2 mi = mallinfo2();    // heap stats
malloc_trim(0);                       // hand back free top-of-heap to kernel
malloc_stats();                       // print to stderr
```

## 8. Threads — what is shared, what isn't

```
Shared across threads (one copy per process):
   heap, data, bss, text, fds, mmap'd regions, signal handlers
   (signal dispositions are per-process; masks are per-thread).

Per-thread (one copy per thread):
   stack, registers/PC, TLS (__thread / thread_local), errno,
   signal MASK, alternate signal stack.
```

`pthread_create` defaults (glibc x86_64): stack 8 MiB (`ulimit -s`), guard 4 KiB.

```c
pthread_attr_t a;
pthread_attr_init(&a);
pthread_attr_setstacksize(&a, 64*1024);
pthread_attr_setguardsize(&a, 4096);
pthread_create(&tid, &a, fn, arg);
pthread_attr_destroy(&a);
```

## 9. Process syscalls — what `fork`, `vfork`, `clone` share

| Resource     | `fork`        | `vfork`        | `clone(CLONE_VM)` (pthreads) |
|--------------|---------------|----------------|------------------------------|
| Address space| **copied**    | **shared**     | **shared**                   |
| Open fds     | copied        | shared         | shared                       |
| Signal handlers | copied     | shared         | shared by default; can split |
| Parent runs  | concurrently  | suspended until child `execve`/`_exit` | concurrently |
| Use case     | new process   | tiny shim before `execve` | thread             |

## 10. Signals you must know (memory-related)

| Signal     | Cause |
|------------|-------|
| `SIGSEGV`  | Bad page access (unmapped, wrong perms, write to RO)   |
| `SIGBUS`   | Bad alignment, file mapping past EOF, IO error in mmap |
| `SIGFPE`   | Arithmetic (div by 0)                                  |
| `SIGILL`   | Illegal instruction (running data as code w/o NX off)  |
| `SIGABRT`  | `abort()`, also glibc heap corruption detector         |

Install via `sigaction` (NOT `signal`):

```c
struct sigaction sa = {0};
sa.sa_sigaction = h;
sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
sigemptyset(&sa.sa_mask);
sigaction(SIGSEGV, &sa, NULL);
```

Use `sigaltstack` so handlers run even when the normal stack is corrupted.

## 11. Memory model knobs

```
overcommit_memory = 0   heuristic (default)
                  = 1   always allow (good for sparse mmap)
                  = 2   strict — never overcommit (commit limit = RAM*ratio + swap)

vm.swappiness     0..100 (default 60)
vm.dirty_ratio    % of RAM dirtied before sync writeback (default 20)
vm.dirty_background_ratio  default 10
```

## 12. Allocator decision tree

```
Need memory:
 |
 +-- ephemeral, small (<a few KiB), known scope -> stack: int buf[N];
 +-- large, scoped to a function                -> alloca / VLA (careful!)
 +-- variable lifetime, unstructured            -> malloc/free
 +-- many same-size objects, freed independently -> pool / slab
 +-- many objects all freed together            -> arena/bump allocator
 +-- shared with another process                -> shm_open + mmap or memfd
 +-- big buffer, want to give back to OS easily -> mmap (single munmap)
 +-- huge (>1 GiB), perf-critical               -> MAP_HUGETLB or THP
 +-- need power-of-2 alignment                  -> posix_memalign / aligned_alloc
```

## 13. Debug recipes

```bash
# Live memory map of a running process
cat /proc/<pid>/maps

# Detailed accounting (PSS, swap, hugepages)
cat /proc/<pid>/smaps

# Per-system memory
cat /proc/meminfo

# Watch malloc calls
strace -e trace=brk,mmap,munmap,mprotect ./prog

# Detect leaks / overflows
valgrind --tool=memcheck --leak-check=full ./prog

# Heap profile
valgrind --tool=massif ./prog && ms_print massif.out.*

# ASan (rebuild with -fsanitize=address)
ASAN_OPTIONS=detect_leaks=1 ./prog

# Page-fault counts
/usr/bin/time -v ./prog
perf stat -e page-faults,minor-faults,major-faults ./prog

# Resident set / VSZ
ps -o pid,vsz,rss,pmem,cmd -p <pid>
```

## 14. Five golden rules

1. **`free` what you `malloc`, exactly once, with the same pointer you got.**
   Pair every allocator with a deallocator (`malloc/free`, `mmap/munmap`,
   `shm_open/shm_unlink`, `fopen/fclose`, ...).
2. **Never trust `errno` unless the call you just made is documented to set
   it.** Most kernel calls return `-1` and set `errno`; check both.
3. **Page-granularity is real.** `mmap(0, 1)` reserves a whole page. `mprotect`
   must take page-aligned addresses. Allocate with the page in mind.
4. **The optimizer cannot read your mind.** Use `volatile` for MMIO,
   `_Atomic` for cross-thread, and memory barriers for cross-core ordering.
   Plain stores can be reordered, elided, hoisted out of loops.
5. **Look at `/proc/self/maps`.** When in doubt about *where* something lives,
   `pause()` your program and `cat /proc/<pid>/maps`. The kernel will tell you
   the truth.
