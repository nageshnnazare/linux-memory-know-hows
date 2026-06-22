# 12 — Debugging & Observation

You now know the theory. This section is the toolbox: how to *look* at a
running process's memory, how to make a problem reproducible, and which tool
to reach for first.

```
   What do I want to know?           Reach for...
   -------------------------         -------------
   "Where do my pages live?"         /proc/self/maps, smaps
   "How much RAM am I using?"        /proc/self/status, getrusage
   "Is malloc behaving?"             mallinfo2, malloc_stats, malloc_info
   "What syscalls am I making?"      strace -e trace=memory ./prog
   "Memory leak / heap bug?"         valgrind --tool=memcheck, ASan
   "Where is the heap growing?"      valgrind --tool=massif + ms_print
   "Cache misses? branch mispred?"   perf stat -e cache-misses,branch-misses
   "Stack & symbols at crash?"       gdb, addr2line, eu-stack, coredumpctl
   "Threads stuck?"                  gdb thread apply all bt
```

## 1. `/proc/self` — the kernel telling on you

| File                | What |
|---------------------|------|
| `maps`              | every VMA, in human form (start-end perms offset dev:inode path) |
| `smaps`             | per-VMA detailed accounting — Rss, Pss, Swap, ... |
| `smaps_rollup`      | sum across all VMAs (5.0+) |
| `status`            | one-line-per-field VmSize, VmRSS, Vm{Data,Stk,Exe,Lib}, threads |
| `stat`              | compact single-line (parseable) version of the above |
| `statm`             | size, resident, share, text, lib, data, dt — all in pages |
| `cmdline`           | NUL-separated argv |
| `environ`           | NUL-separated env |
| `limits`            | rlimits in human form |
| `cgroup`            | cgroup memberships |
| `fd/`               | each open fd as a symlink |
| `task/<tid>/...`    | per-thread mirror of everything above |

### Useful one-liners

```bash
# what's eating my RAM?
awk '/^Pss:/{p+=$2} END{print p,"KiB"}' /proc/<pid>/smaps

# how many private dirty bytes do I have?
awk '/^Private_Dirty:/{p+=$2} END{print p,"KiB"}' /proc/<pid>/smaps

# which library is biggest in my process?
sort -k3n /proc/<pid>/smaps | head -50
```

## 2. `getrusage`

```c
struct rusage r;
getrusage(RUSAGE_SELF, &r);

r.ru_utime / r.ru_stime    // CPU user/sys time
r.ru_maxrss                // peak RSS in KiB
r.ru_minflt                // minor faults
r.ru_majflt                // major faults
r.ru_inblock / r.ru_oublock// block I/O
r.ru_nvcsw  / r.ru_nivcsw  // voluntary / involuntary context switches
```

`time -v ./prog` (the *GNU* time, not the shell built-in) prints this nicely.

## 3. `mallinfo2` and friends

```c
#include <malloc.h>
struct mallinfo2 mi = mallinfo2();
malloc_stats();           // stderr dump per arena
char *xml; size_t len;
malloc_info(0, fmemopen ?: stderr);   // detailed XML dump
malloc_trim(0);
```

## 4. `strace` recipes

```bash
strace -f -e trace=memory ./prog 2>&1 | grep -E 'mmap|brk|munmap|mprotect'
strace -f -e trace=signal ./prog
strace -f -c ./prog       # summary of syscalls
strace -p <pid>           # attach
```

## 5. `valgrind`

```bash
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./prog
valgrind --tool=massif ./prog && ms_print massif.out.<pid>
valgrind --tool=cachegrind ./prog
valgrind --tool=helgrind ./prog     # race detector (alternative to TSan)
```

Valgrind makes the program 10-100x slower but reports almost every memory
sin imaginable. For production-speed checking use ASan/TSan/UBSan (compile-
time instrumentation).

## 6. `perf`

```bash
perf stat -e cycles,instructions,page-faults,minor-faults,major-faults,\
cache-references,cache-misses,branch-misses ./prog

perf record -F 99 -g ./prog          # CPU profile with callstacks
perf report

perf trace -p <pid>                  # cheap strace alternative

perf stat -e dTLB-load-misses ./prog # TLB pressure
```

## 7. `gdb` cheatsheet

```
gdb ./prog
(gdb) run [args]
(gdb) bt                    # backtrace
(gdb) thread apply all bt   # backtraces for every thread
(gdb) info proc mappings    # equivalent to /proc/<pid>/maps
(gdb) info threads
(gdb) print/x  *(unsigned long*)0x7fff...
(gdb) x/16xb   0x7fff...
(gdb) watch *(int*)0xADDR
(gdb) catch syscall mmap
(gdb) run
```

For a core dump:
```bash
ulimit -c unlimited
./prog          # crashes; coredumpctl info / gdb -c core.<pid>
```

## 8. ASan / UBSan / TSan / MSan

| Sanitizer  | What it detects | Flag |
|------------|-----------------|------|
| ASan       | heap/stack/global overflow, UAF, double-free | `-fsanitize=address` |
| UBSan      | undefined behaviour (alignment, overflow, etc.) | `-fsanitize=undefined` |
| TSan       | data races, deadlocks | `-fsanitize=thread` |
| MSan       | uses of uninitialised memory (Clang only) | `-fsanitize=memory` |
| LSan       | leaks (subset of ASan) | `-fsanitize=leak` |
| HWASan     | ASan with hw tagging (ARM64) | `-fsanitize=hwaddress` |
| MTE        | hardware tag-based on ARMv8.5+ | (Android NDK) |

All require `-O1+ -g`. Always rebuild dependencies; sanitizers don't compose
across libs not built with them.

## 9. Files

| File | Demonstrates |
|------|--------------|
| [`01_proc_maps_dump.c`](01_proc_maps_dump.c) | Pretty-print `/proc/self/maps` |
| [`02_proc_smaps.c`](02_proc_smaps.c)         | Sum Pss/Rss/Swap across all VMAs |
| [`03_status_summary.c`](03_status_summary.c) | Extract VmSize/VmRSS/Vm{Data,Stk,Exe,Lib} |
| [`04_getrusage_demo.c`](04_getrusage_demo.c) | Run a small workload, print before/after rusage |
| [`05_mallinfo_dump.c`](05_mallinfo_dump.c)   | mallinfo2 + malloc_stats + malloc_info |
