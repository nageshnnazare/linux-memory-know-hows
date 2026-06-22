# 13 — Advanced Topics

If you've made it this far, the basics are solid. Here are the deeper /
newer / weirder corners of Linux memory.

## 1. NUMA — Non-Uniform Memory Access

Modern multi-socket servers have one or more **NUMA nodes** per socket; each
node has its own memory controller and "local" DRAM. Accessing remote DRAM
costs 1.5–2× more latency.

```
   CPU socket 0                      CPU socket 1
   +------------------+              +------------------+
   |  cores 0..15     | <----QPI---> |  cores 16..31    |
   |  L1 / L2 / L3    |              |  L1 / L2 / L3    |
   +-----+------------+              +-----+------------+
         | local DRAM                       | local DRAM
         v                                  v
   +----------------+                +----------------+
   | NUMA node 0    |   "remote"     | NUMA node 1    |
   | 96 GiB         |   accesses     | 96 GiB         |
   +----------------+   over QPI     +----------------+
```

### Inspecting

```bash
numactl --hardware
numactl --show
cat /proc/<pid>/numa_maps
```

### Controlling placement

```bash
numactl --membind=0 --cpunodebind=0 ./prog       # pin to node 0
numactl --interleave=all ./prog                   # round-robin across nodes
```

In C use `libnuma`:

```c
#include <numa.h>
numa_set_localalloc();         // default policy
numa_bind(numa_parse_nodestring("0"));
void *p = numa_alloc_onnode(sz, 0);
```

Or syscalls:

```c
mbind(addr, len, MPOL_BIND, &nodemask, maxnode, 0);
move_pages(pid, count, pages, nodes, status, MPOL_MF_MOVE);
```

### NUMA pitfalls

- **First-touch policy**: a page is allocated on the node of the CPU that
  first writes to it (with default policy). Initialise from the thread that
  will use the data.
- Migrating threads across nodes after pages are placed is expensive — pin
  threads (`taskset`, `pthread_setaffinity_np`).

## 2. Huge pages

Already touched in §05. Two flavours:

### Transparent Huge Pages (THP)

```bash
cat /sys/kernel/mm/transparent_hugepage/enabled
   [always] madvise never

echo madvise > /sys/kernel/mm/transparent_hugepage/enabled   # opt-in only
```

Use `madvise(p, len, MADV_HUGEPAGE)` to ask the kernel to promote your VMA
to 2 MiB pages. `khugepaged` does the merging in the background.

### Explicit HugeTLB

```bash
# reserve 64 huge pages (128 MiB) at boot or runtime:
echo 64 > /proc/sys/vm/nr_hugepages
cat /proc/meminfo | grep -i huge
```

```c
void *p = mmap(NULL, 2UL << 20,
               PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_2MB,
               -1, 0);
```

Or use `hugetlbfs`:

```bash
mount -t hugetlbfs hugetlbfs /mnt/huge -o pagesize=2M
```

then `open("/mnt/huge/myfile")` and mmap it.

See [`01_thp_madvise.c`](01_thp_madvise.c) and
[`02_hugetlb_mmap.c`](02_hugetlb_mmap.c).

## 3. `userfaultfd` — user-space page-fault handling

Get an fd that delivers a message every time the process touches an
unmapped page in a registered range. Used by:

- VM live migration (post-copy)
- Garbage collectors (read barriers)
- Sandboxes (intercept access to virtual memory)
- Databases (custom paging)

Pseudocode:

```c
int u = syscall(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK);
struct uffdio_api api = { .api = UFFD_API, .features = 0 };
ioctl(u, UFFDIO_API, &api);
struct uffdio_register reg = { .range = { .start = addr, .len = sz },
                                .mode = UFFDIO_REGISTER_MODE_MISSING };
ioctl(u, UFFDIO_REGISTER, &reg);

for (;;) {
    struct uffd_msg msg;
    read(u, &msg, sizeof msg);
    /* compute the page to provide, then: */
    struct uffdio_copy c = { .dst = msg.arg.pagefault.address & ~0xfff,
                              .src = (uintptr_t)page,
                              .len = 4096 };
    ioctl(u, UFFDIO_COPY, &c);
}
```

See [`03_userfaultfd.c`](03_userfaultfd.c) (Linux-only; needs unprivileged
mode if `vm.unprivileged_userfaultfd = 1`).

## 4. `memfd_create` — anonymous in-RAM files

Already in §10. Two extra superpowers:

- **F_SEAL_WRITE/F_SEAL_GROW/F_SEAL_SHRINK** — once set, can never be unset.
- Pass over `SCM_RIGHTS` so a receiver gets an fd to *exactly* the same
  pages.

Use case: a sandbox can hand its parent a memfd to act as a read-only
input channel; the parent seals it after writing.

## 5. `io_uring` — async I/O with shared rings

io_uring is a syscall family that gives userspace two mmap'd rings (SQ and
CQ) shared with the kernel:

```
   process VA                                 kernel
   +-----------------+   io_uring_setup       +-----------------+
   | SQ ring (mmap)  | <--shared mapping----> | sq_entries[]    |
   +-----------------+                        +-----------------+
   | CQ ring (mmap)  | <--shared mapping----> | cq_entries[]    |
   +-----------------+                        +-----------------+
```

You queue requests into the SQ (no syscall), tell the kernel "go" with
`io_uring_enter`, and harvest completions from the CQ. The same mechanism
can:

- read/write to fds (`IORING_OP_READ/WRITE`)
- do `fsync`, `splice`, `accept`, `connect`, ...
- do many syscalls in one shot (`io_uring_enter`)
- register buffers (avoid copy)
- use `IORING_OP_PROVIDE_BUFFERS` for true zero-copy I/O

It's not strictly a "memory" topic but it relies on shared-memory rings —
mmap is the basis. We don't include a full demo here (see
[`liburing`](https://github.com/axboe/liburing)).

## 6. DAX — Direct Access to persistent memory

Persistent memory (Intel Optane was the classic example) can be exposed as
`/dev/pmem0` or via `fsdax`. `mmap(MAP_SYNC | MAP_SHARED_VALIDATE, ...)` on
a DAX file gives **persistent stores** — `clflushopt` + `sfence` is enough,
no need for `msync`.

Not in mainstream usage today (Optane is EOL) but the kernel mechanism
remains and is used by CXL persistent memory.

## 7. `mremap` magic

Beyond simple grow/shrink, `mremap` can:

- **swap two regions** of equal size with `MREMAP_FIXED | MREMAP_DONTUNMAP`
  (Linux 5.7+) — used by GCs that flip between heaps.
- **move a mapping** somewhere else without copying — just rewires PTEs.

## 8. Kernel same-page merging (KSM)

`madvise(p, len, MADV_MERGEABLE)` registers a region with KSM, the
deduplication daemon. When two pages of two processes have identical
content, KSM merges them to a single CoW frame. Big win on KVM hosts with
many similar VMs.

```bash
echo 1 > /sys/kernel/mm/ksm/run
cat /sys/kernel/mm/ksm/pages_shared
```

## 9. Files

| File | Demonstrates |
|------|--------------|
| [`01_thp_madvise.c`](01_thp_madvise.c)   | `MADV_HUGEPAGE` on a large region |
| [`02_hugetlb_mmap.c`](02_hugetlb_mmap.c) | `MAP_HUGETLB \| MAP_HUGE_2MB` (needs hugepages pre-reserved) |
| [`03_userfaultfd.c`](03_userfaultfd.c)   | Skeleton of user-space page-fault handling |
| [`04_numa_inspect.c`](04_numa_inspect.c) | Print `/proc/self/numa_maps` |
| [`05_ksm_demo.c`](05_ksm_demo.c)         | Mark a region MADV_MERGEABLE |
