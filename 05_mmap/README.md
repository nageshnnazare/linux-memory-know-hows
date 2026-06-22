# 05 — `mmap` — the swiss army knife of memory

`mmap(2)` is the single most powerful memory syscall on Linux. With it you can:

- get raw zero-filled pages (the basis of `malloc`'s big allocations),
- map files into your address space (so reading is just pointer arithmetic),
- share memory between processes,
- request huge pages,
- enforce strict permissions,
- and grow / shrink / move regions.

```
   mmap is to memory what open() is to files.
```

## 1. The signature

```c
#include <sys/mman.h>
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t off);
int   munmap(void *addr, size_t length);
```

| arg | meaning |
|-----|---------|
| `addr`   | hint (`NULL` lets kernel pick). With `MAP_FIXED` it's mandatory. |
| `length` | bytes; **rounded up to a page** (4 KiB). |
| `prot`   | `PROT_READ`, `PROT_WRITE`, `PROT_EXEC`, `PROT_NONE` (bitwise OR). |
| `flags`  | see below. Pick exactly one of `MAP_PRIVATE` / `MAP_SHARED`. |
| `fd`     | file descriptor; `-1` for anonymous. |
| `off`    | must be a page multiple. |

## 2. Four basic combinations

```
                     anonymous (fd=-1, MAP_ANONYMOUS)     file-backed (fd>=0)
                    +----------------------------------+ +-----------------------------+
   MAP_PRIVATE      |  zero pages, CoW                 | |  read file contents lazily; |
                    |  good for malloc-style memory    | |  writes invisible to others |
                    |  freed on munmap; never on disk  | |  and discarded at munmap    |
                    +----------------------------------+ +-----------------------------+
   MAP_SHARED       |  shared between fork'd processes | |  writes go back to the file |
                    |  (legacy MAP_SHARED|ANONYMOUS)   | |  shared with everyone who   |
                    |                                  | |  mmaps the same inode       |
                    +----------------------------------+ +-----------------------------+
```

- **MAP_PRIVATE | MAP_ANONYMOUS** = "give me N bytes of zero". Used by `malloc`
  for big allocs, and by everyone for arbitrary memory.
- **MAP_SHARED | file** = "let me read/write this file through a pointer".
  Used by `mmap I/O`, databases (SQLite, LMDB), shared libraries (`r-xp`).
- **MAP_PRIVATE | file** = "show me the file, but my writes are mine alone."
  Used to load executables: the loader maps `.text` with `r-xp` and `.data`
  with `rw-p` MAP_PRIVATE so each process gets its own copy on first write.
- **MAP_SHARED | MAP_ANONYMOUS** = shareable memory after `fork()`. Older
  pattern; modern code prefers `memfd_create` or `shm_open`.

## 3. The flags zoo

| Flag | Effect |
|------|--------|
| `MAP_PRIVATE`       | CoW; private to this mapping |
| `MAP_SHARED`        | writes are visible to everyone mapping the same object |
| `MAP_SHARED_VALIDATE` | like SHARED but error if any flag is unknown |
| `MAP_ANONYMOUS`     | not backed by a file; zero-filled; `fd` must be -1 |
| `MAP_FIXED`         | place exactly at `addr` (clobbers existing mappings; dangerous) |
| `MAP_FIXED_NOREPLACE` | like FIXED but fail instead of clobber (Linux 4.17+) |
| `MAP_POPULATE`      | pre-fault all pages so they're already resident |
| `MAP_NORESERVE`     | don't reserve swap; allows over-commit on private mappings |
| `MAP_LOCKED`        | call `mlock` after mapping |
| `MAP_HUGETLB`       | use huge pages (need `huge_page_count >0`) |
| `MAP_HUGE_2MB` / `MAP_HUGE_1GB` | pick page size with HUGETLB |
| `MAP_STACK`         | mark as suitable for a stack (some arches align) |
| `MAP_GROWSDOWN`     | legacy stack flag |
| `MAP_UNINITIALIZED` | embedded only; skip zero-fill |
| `MAP_SYNC`          | (DAX) writes guaranteed durable on return |

## 4. The lifecycle

```
   void *p = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
                |
                |  kernel: create VMA at chosen address, no PTEs yet
                v
   touch p[i]
                |  page fault, kernel allocates frame (or maps zero page on read),
                |  installs PTE
                v
   madvise(p, len, MADV_DONTNEED)
                |  kernel drops the frames; VMA stays
                v
   mprotect(p, len, PROT_READ)
                |  PTEs marked RO; writes now fault SIGSEGV
                v
   munmap(p, len)
                |  VMA destroyed; PTEs cleared
```

## 5. File mapping demystified

```
   open("img.bmp", O_RDONLY)         -- get an fd
   stat(... &st)                     -- learn size
   p = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
   close(fd);                        -- safe; mapping holds its own ref
   ...
   uint32_t pixel = ((uint32_t *)p)[42];
   munmap(p, st.st_size);
```

Visual:

```
   disk file img.bmp
   +-----------------+-----------------+-----------------+
   | page 0 (4 KiB)  | page 1          | page 2          |
   +--------+--------+--------+--------+--------+--------+
            |                 |                 |
            v                 v                 v
   process A virtual address space:
   p ->  +---+    p+4K -> +---+   p+8K -> +---+
         |   |             |   |          |   |
         +---+             +---+          +---+
```

- The kernel does *not* read every page on `mmap`. Pages are pulled in on
  demand from the page cache (a major fault, then probably hits cache and is
  free).
- If the file is also being read by another process via `read()`, you share
  the same page cache pages.

### Modifying a file via mmap

```c
fd = open("data", O_RDWR);
p  = mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
((char *)p)[100] = 'X';        // dirty the page
msync(p, sz, MS_SYNC);         // optional: flush now
munmap(p, sz);
```

`MAP_SHARED` writes mark the page dirty. `pdflush`/`kworker` will eventually
write them back; `msync` forces it.

## 6. `mprotect` — change permissions

```c
mprotect(p, len, PROT_READ);                  // make RO
mprotect(p, len, PROT_NONE);                  // unmap-but-keep-VMA, for guard pages
mprotect(p, len, PROT_READ|PROT_EXEC);        // RX for JIT
```

Requires page-aligned `p` and `len`. Useful for:
- Guard pages around buffers ([`05_mprotect_guard.c`](05_mprotect_guard.c)).
- JIT compilers: write code as RW, flip to RX before executing.

## 7. `madvise` — hints

```c
madvise(p, len, MADV_WILLNEED);     // prefault, please read ahead
madvise(p, len, MADV_DONTNEED);     // drop frames, future reads zero
madvise(p, len, MADV_SEQUENTIAL);   // aggressive readahead
madvise(p, len, MADV_RANDOM);       // disable readahead
madvise(p, len, MADV_FREE);         // lazy free; kernel may reclaim
madvise(p, len, MADV_HUGEPAGE);     // try to promote to THP
madvise(p, len, MADV_DONTFORK);     // child doesn't inherit this region
madvise(p, len, MADV_WIPEONFORK);   // child sees zeroed copy
```

`MADV_DONTNEED` is the **immediate** version of `MADV_FREE`: pages are gone
right now and counts decrement.

## 8. `mremap` — grow / move

```c
void *q = mremap(p, old_len, new_len, MREMAP_MAYMOVE);
```

- Grows or shrinks in place if possible.
- With `MREMAP_MAYMOVE`, kernel may pick a new VA and move the VMA cheaply
  (it doesn't copy pages — it relinks page tables).
- With `MREMAP_FIXED|MREMAP_MAYMOVE`, places at a specific address.

This is faster than `mmap` + `memcpy` + `munmap` because no data is copied;
the kernel just rewires the page tables.

## 9. Huge pages via mmap

```c
void *p = mmap(NULL, 2UL*1024*1024,
               PROT_READ|PROT_WRITE,
               MAP_PRIVATE|MAP_ANONYMOUS|MAP_HUGETLB|MAP_HUGE_2MB, -1, 0);
```

Need `sudo sh -c 'echo 64 > /proc/sys/vm/nr_hugepages'` first (reserves 64
huge pages × 2 MiB = 128 MiB).

Why? 1 TLB entry covers 2 MiB (or 1 GiB) instead of 4 KiB. For workloads with
huge working sets (databases, ML), TLB pressure dominates.

See [`../13_advanced/02_huge_tlb.c`](../13_advanced/02_huge_tlb.c).

## 10. Files in this folder

| File | Demonstrates |
|------|--------------|
| [`01_anon_mmap.c`](01_anon_mmap.c)               | `MAP_PRIVATE\|MAP_ANONYMOUS` — like a giant `calloc` |
| [`02_file_mmap.c`](02_file_mmap.c)               | Read a file via pointer (`MAP_PRIVATE`, file-backed) |
| [`03_shared_mmap.c`](03_shared_mmap.c)           | Two processes sharing memory via `MAP_SHARED\|MAP_ANONYMOUS` after fork |
| [`04_file_share_write.c`](04_file_share_write.c) | Modify a file through `MAP_SHARED`, observe with `cat` |
| [`05_mprotect_guard.c`](05_mprotect_guard.c)     | Surround a buffer with `PROT_NONE` guard pages |
| [`06_madvise_dontneed.c`](06_madvise_dontneed.c) | Drop pages, see RSS shrink |
| [`07_mremap.c`](07_mremap.c)                     | Grow / shrink / move a mapping |
| [`08_map_populate.c`](08_map_populate.c)         | `MAP_POPULATE` vs lazy faults |
| [`09_map_fixed_noreplace.c`](09_map_fixed_noreplace.c) | Place at a specific address, fail if occupied |
