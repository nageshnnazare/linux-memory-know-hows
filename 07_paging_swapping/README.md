# 07 — Paging, Page Cache, Swap, and the OOM Killer

So far we've talked about memory from the *process's* viewpoint. This section
looks at the system level: how the kernel rations RAM, what the **page
cache** is, how **swap** works, and why your program just got killed by the
OOM killer at 3am.

## 1. Where do physical frames live?

```
                            +------- PHYSICAL RAM -------+
                            |                            |
                            |  free pages                |
                            |    (kept on per-NUMA       |
                            |     buddy free lists)      |
                            |                            |
                            |  anonymous pages           | <- your malloc,
                            |    (process private RAM)   |    stacks, bss
                            |                            |
                            |  file-backed (page cache)  | <- mmap'd files,
                            |    clean / dirty           |    read() buffers
                            |                            |
                            |  slab / kernel             | <- inode caches,
                            |                            |    network buffers
                            |  reserved / kernel text    |
                            +----------------------------+
```

`cat /proc/meminfo` shows the breakdown:

```
   MemTotal:       16252160 kB     total RAM the kernel sees
   MemFree:         1234567 kB     unused, on buddy free lists
   MemAvailable:    8000000 kB     "you can probably allocate this much"
   Buffers:           45000 kB     page cache for block devices (raw)
   Cached:          6800000 kB     page cache for files (incl. tmpfs)
   SwapTotal:       4194304 kB
   SwapFree:        4150000 kB
   Dirty:               500 kB     dirty (not yet written to disk) page cache
   Writeback:             0 kB     currently being written
   AnonPages:       4500000 kB     non-file-backed (heap/stack/anon mmap)
   Mapped:           700000 kB     mapped via mmap somewhere
   Slab:             400000 kB     kernel slab (kmalloc internals)
   ...
```

## 2. The page cache

Every read or write to a regular file goes through the **page cache**:

```
   write(fd, buf, n)
        |
        v
   kernel copies buf -> page cache pages (RAM); marks them dirty
        |
        v
   returns immediately (no disk I/O yet)
        |
        v
   later: pdflush / kworker writes dirty pages to disk
   (sync() / fsync() forces it now)
```

`mmap(file, MAP_SHARED)` lets you skip the copy entirely — you read and write
the cache pages directly via your mapping.

### Why is `free` showing little free RAM?

Because Linux uses every byte it has. Cached file pages are not "in use" in
any meaningful sense — they will be evicted instantly when something else
needs memory. Look at `MemAvailable`, not `MemFree`.

### Dropping the page cache (debug only)

```bash
sync && echo 3 | sudo tee /proc/sys/vm/drop_caches
# 1 = clean page cache, 2 = slab, 3 = both
```

## 3. The buddy allocator

The kernel's physical-frame allocator is a **buddy system**.

```
   memory split into orders:
     order 0 = 1 page    = 4 KiB
     order 1 = 2 pages   = 8 KiB
     order 2 = 4 pages   = 16 KiB
     ...
     order 10 = 1024 pages = 4 MiB

   free list per (zone, order):
   +--------+--------+--------+ ...
   | order0 | order1 | order2 |
   +--------+--------+--------+
   | head | | head | | head | |
   |  v   | |  v   | |  v   | |
   | pg-> | | pg-> | | pg-> | |
   | pg   | | pg   | | pg   | |
   +------+ +------+ +------+
```

`cat /proc/buddyinfo`:

```
   Node 0, zone DMA32  1234  567  89  10  ...
                       ^     ^     ^   ^
                       order 0 (4K), 1 (8K), 2 (16K), 3 (32K), ...
```

When the kernel needs N contiguous pages, it asks for `2^ceil(log2(N))` and
splits a buddy bigger than that. Releases coalesce buddies recursively.

External fragmentation can be a problem: lots of free 4 KiB pages, no
contiguous 2 MiB block for THP. Hence **memory compaction**.

## 4. Swap

When RAM is exhausted (more precisely: when the kernel's LRU lists need to
evict pages), it picks **inactive anonymous** pages and writes them out to
the **swap device** (a swap partition or swapfile).

```
   anonymous page lifecycle:

     created (e.g. malloc + first touch)
           |
     +-----v-----+
     | active    |  recently used
     +-----------+
           |
           v
     +-----------+
     | inactive  |  not used in a while
     +-----------+
           |
           v
     written to swap; page reclaimed; PTE points to swap slot
           |
           v
     next access -> #PF (major); kernel reads from swap; re-resident
```

Two important sysctls:

- `vm.swappiness` (0..200, default 60). Higher = swap more eagerly.
- `vm.overcommit_memory`:
  - `0` (default): heuristic — refuses really obviously impossible allocations.
  - `1`: always say yes. `malloc` of 1 PiB succeeds; you OOM-kill when you
    touch it.
  - `2`: strict — `CommitLimit = swap + RAM × overcommit_ratio/100`.

### Why isn't *file-backed* memory swapped?

Because we can just throw it away and read it again from the file on next
access. Only anonymous (non-file) pages need to go through swap.

## 5. The OOM killer

When the kernel is **truly** out of memory and can't reclaim any more pages,
it invokes `oom_kill_process`. Each process has an OOM **score** (visible at
`/proc/PID/oom_score`):

```
   higher score = killed first.
   score = badness(memory_use, runtime, oom_score_adj, ...)
```

You can bias your process:

```c
int fd = open("/proc/self/oom_score_adj", O_WRONLY);
write(fd, "-1000", 5);   // never killed if any other option exists
close(fd);
```

Or with `choom -p PID -n N`.

When a process is killed, you see in `dmesg`:

```
   Out of memory: Killed process 1234 (foo) total-vm:8GB,
       anon-rss:7GB, file-rss:0kB, shmem-rss:0kB
```

## 6. `mlock` — pin pages in RAM

For real-time / security-sensitive code (cryptographic keys must not be
swapped to disk), use `mlock`:

```c
char *secret = malloc(64);
mlock(secret, 64);          // pin in RAM
// ... use secret ...
explicit_bzero(secret, 64);
munlock(secret, 64);
free(secret);
```

Or pin the entire process:

```c
mlockall(MCL_CURRENT | MCL_FUTURE);
```

Limited by `RLIMIT_MEMLOCK` for non-root.

## 7. Watching swap / faults / RSS

```bash
vmstat 1                # si/so columns = swap in/out per sec
free -h                 # human totals
sar -B 1                # paging stats
cat /proc/PID/status | grep -E 'Vm|Hugepages'
cat /proc/PID/smaps_rollup
/usr/bin/time -v ./prog # majfaults, minor faults, maxrss
perf stat -e page-faults,minor-faults,major-faults ./prog
```

## 8. Files

| File | Demonstrates |
|------|--------------|
| [`01_meminfo.c`](01_meminfo.c)             | Parse `/proc/meminfo` and print the headline numbers |
| [`02_mlock.c`](02_mlock.c)                 | Pin pages, observe `Locked:` in `smaps` |
| [`03_madvise_pageout.c`](03_madvise_pageout.c) | `MADV_PAGEOUT` to push pages to swap proactively |
| [`04_oom_score.c`](04_oom_score.c)         | Read/write `oom_score_adj` |
| [`05_overcommit.c`](05_overcommit.c)       | Allocate way more virtual memory than RAM with `MAP_NORESERVE` |
