# 04 — The Heap and `malloc`

This is the longest chapter and arguably the most important one. `malloc` is
where most programmers get their mental model of memory, and yet most
programmers do not know that:

- `malloc` is **not a syscall**. It's a library routine in glibc.
- `malloc(big)` and `malloc(small)` take *completely different code paths*.
- `free` rarely returns memory to the kernel — it usually just hands the
  chunk back to a per-process freelist for reuse.
- A "buffer overflow" doesn't only overflow your buffer — it corrupts the
  glibc *heap metadata* that lives **right next to** your data.

The glibc allocator is called **ptmalloc2** and is based on Doug Lea's
`dlmalloc`. We'll dissect it.

## 1. The big picture

```
   ┌─────────────────── glibc user-facing ────────────────────┐
   │   malloc(size) ─┐                                        │
   │   free(p)       │                                        │
   │   calloc(n,s)   │    pthread-aware (per-thread arenas)   │
   │   realloc(p,s)  │                                        │
   │   aligned_alloc │                                        │
   └─────────────────┴────────────────────────────────────────┘
            │                                          │
            │ small / mid-size                         │ big (default >=128 KiB)
            v                                          v
   ┌──────────────────────┐                ┌──────────────────────────┐
   │  ptmalloc arena      │                │  mmap()ed region         │
   │  (carved with brk)   │                │  -- one per allocation   │
   │                      │                │  -- whole pages          │
   │   freelists (bins)   │                │  -- munmap'd on free     │
   │   chunks <-> chunks  │                └──────────────────────────┘
   └──────────────────────┘                       │
            │                                     │
            v                                     v
        sbrk(2)                                mmap(2)
        kernel                                 kernel
```

Take-aways:
- **Big** allocations (`> M_MMAP_THRESHOLD`, default 128 KiB) go via `mmap` and
  are returned to the kernel on `free` (a real `munmap`).
- **Small/medium** allocations are carved out of an *arena* — a chunk of
  memory the allocator obtained earlier with `sbrk` (for the main arena) or
  `mmap` (for thread arenas).

## 2. A chunk

`ptmalloc` keeps a tiny **header** before every chunk it gives you. The
pointer `malloc` returns points to the **payload**, but the header (and
sometimes a footer / inline metadata) live *just before* and *just after*
that payload.

```
                    +-----------------+   <- chunk start
                    | prev_size (8B)  |       only used if previous chunk is free
                    +-----------------+
                    | size  | A M P   |       size of THIS chunk (incl. header),
                    |       | flags   |       low 3 bits hold A/M/P flags:
                    +-----------------+         A = arena (non-main)
   user pointer ->  | payload         |         M = mmap'd
                    | (N bytes)       |         P = previous chunk in use
                    | ...             |
                    | ...             |
                    | padding to 16B  |
                    +-----------------+   <- next chunk's `prev_size`
```

So `sizeof(metadata)` is **16 bytes** per allocation on x86\_64 (8B prev_size,
8B size+flags). Always.

> Mental model: every `malloc(N)` actually uses `N + 16` bytes, rounded up to
> a 16-byte multiple, with a minimum chunk size of 32 bytes.

Free chunks also store forward/backward pointers (`fd`, `bk`) **inside the
payload area** so the allocator can chain them in a doubly-linked list.

```
   FREE chunk (in a bin):
   +-----------------+
   | prev_size       |
   +-----------------+
   | size | A 0 P    |
   +-----------------+
   | fd (next free)  |
   +-----------------+
   | bk (prev free)  |
   +-----------------+
   | unused space    |
   |                 |
   +-----------------+
   | size            |   footer, repeated, for boundary tags
   +-----------------+
```

## 3. Bins — the freelist taxonomy

When you `free(p)`, the chunk lands in a **bin** according to its size.
There are five families:

```
   chunk freed ── how big is it?
       │
       │  16..88 B           ─► fastbin (single-linked LIFO, no coalesce)
       │  16..512 B          ─► tcache  (per-thread, single-linked, glibc 2.26+)
       │  16..512 B          ─► smallbin (doubly-linked, exact size)
       │  >= 1024 B          ─► largebin (size-ordered)
       │  any sizes leftover ─► unsorted bin (just freed, scanned on next malloc)
       │  top of arena (wilderness) ─► top chunk
```

### Tcache (glibc ≥ 2.26)

A **per-thread** array of 64 single-linked LIFO lists, one per size class.
Default: 7 chunks per bin, sizes from 24 to 1032 bytes. Tcache makes
`malloc`/`free` of small objects essentially lock-free.

```
   tcache[0]  ->  [chunk]->[chunk]->NULL   (16B chunks)
   tcache[1]  ->  [chunk]->NULL            (32B)
   tcache[2]  ->  NULL                     (48B)
   ...
   tcache[63] ->  ...
```

### Fastbins

Slightly larger sizes, also single-linked LIFO, but per-arena (not
per-thread). They are *not coalesced* immediately — they trade some heap
fragmentation for speed. The allocator periodically runs
`malloc_consolidate()` to merge fastbin chunks with neighbours.

### Smallbins / Largebins

Doubly-linked lists. Smallbins hold exact-size chunks (one bin per size from
16 to 504 bytes). Largebins (size ≥ 1024) hold size-ordered lists for
best-fit.

### Top chunk

The free region at the very top of the heap is called the **top chunk** (a.k.a.
the "wilderness"). It's the only chunk that can be enlarged by `sbrk` /
`mmap`-extension.

```
   heap (after a while of malloc/free):

   +----------+----------+----------+----------+----------+--------------------+
   | in-use   | FREE     | in-use   | in-use   | FREE     |   TOP CHUNK        |
   +----------+----------+----------+----------+----------+--------------------+
                                                          ^                    ^
                                                          |                    |
                                                       end of last           program break
                                                       in-use chunk          (sbrk(0))
```

## 4. Arenas — multi-threaded heaps

In single-thread land, there is **one arena**: the *main arena*, grown by
`sbrk`. When threads appear, contention on the main arena's lock becomes a
bottleneck, so glibc creates **per-thread arenas**:

- First time thread T calls `malloc`, glibc may `mmap` a fresh arena (64 MiB
  default) and use it for T thereafter.
- The number of arenas is capped at `8 * ncpu` (configurable with
  `M_ARENA_MAX` or env var `MALLOC_ARENA_MAX`).
- Each arena has its own mutex.

```
            [main arena]               [arena 1]              [arena 2]
            +----------+               +----------+           +----------+
            | freelist |               | freelist |           | freelist |
            +----------+               +----------+           +----------+
            | top chunk|               | top chunk|           | top chunk|
            +----------+               +----------+           +----------+
            grown by brk               grown by mmap          grown by mmap

         thread A, main thread        thread B               thread C
         lock(main arena)             lock(arena 1)          lock(arena 2)
```

A thread is "pinned" to its arena but if its arena is contended it may try
the next one. Hence: a malloc-heavy multi-thread workload may show VSZ several
times larger than working set, simply because of arenas.

## 5. Big allocations — `mmap` path

When `size + headers > M_MMAP_THRESHOLD` (default 128 KiB but **dynamic** in
modern glibc), `malloc` skips the arenas and calls `mmap`:

```c
mmap(NULL, request + 16, PROT_RW, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
```

`free` on such a chunk calls `munmap`. Two consequences:

- Large allocations cost a syscall each, plus 1+ page faults on first touch.
- Large allocations are returned to the kernel immediately on `free` — VSZ
  drops.

Tune with `mallopt(M_MMAP_THRESHOLD, n)` or `MALLOC_MMAP_THRESHOLD_=N`.

## 6. How `malloc(N)` actually decides

```
   malloc(N):
   ┌─────────────────────────────────────────────────────────────┐
   │ 1. round N up to chunk size (header + 16-byte align)        │
   │ 2. if size > M_MMAP_THRESHOLD:                              │
   │       p = mmap(...); flag chunk as M; return p              │
   │ 3. else if size fits in tcache and tcache bin not empty:    │
   │       pop from tcache; return                               │
   │ 4. else lock current arena                                  │
   │ 5. if size fits a fastbin and bin not empty: pop, unlock    │
   │ 6. scan smallbin for exact size; pop, unlock                │
   │ 7. scan unsorted bin; pull chunks into smallbin/largebin    │
   │ 8. scan largebin for best fit; split chunk                  │
   │ 9. otherwise grab from top chunk; if too small, sbrk/mmap   │
   │ 10. unlock; return payload                                  │
   └─────────────────────────────────────────────────────────────┘
```

`free(p)`:

```
   free(p):
   ┌─────────────────────────────────────────────────────────────┐
   │ 1. read chunk header at p - 16                              │
   │ 2. if M flag set (mmap'd): munmap; done                     │
   │ 3. if size fits tcache: push onto tcache; done              │
   │ 4. lock arena                                               │
   │ 5. if size fits a fastbin: push; unlock; done               │
   │ 6. coalesce with prev/next free chunks (boundary tags)      │
   │ 7. if neighbour is top chunk: merge; if heap >> threshold,  │
   │    optionally trim with sbrk(neg) or madvise(DONTNEED)      │
   │ 8. push onto unsorted bin                                   │
   │ 9. unlock                                                   │
   └─────────────────────────────────────────────────────────────┘
```

## 7. `brk` vs `mmap` — why both?

```
   heap grown with brk           heap grown with mmap
   +-----------+                 +-----------+   +-----------+   +-----+
   |  in-use   |                 |  alloc 1  |   |  alloc 2  |   ...
   |  ----     |                 +-----------+   +-----------+   +-----+
   |  in-use   |                 free => munmap immediately
   |  free     |
   |  top      |
   +-----------+
   <- one contiguous region ->   <- many separate VMAs ->
```

Pros of `brk`:
- Cheap to extend (no new VMA per allocation).
- Compact metadata (one VMA for the whole heap).

Cons of `brk`:
- Can only shrink the *top*. A single in-use chunk near the top prevents the
  heap from being returned to the OS, even if everything else is free.

That's why ptmalloc uses `mmap` for big chunks: each one is returnable on its
own.

## 8. Alignment

- `malloc` returns memory aligned to **16 bytes** on x86\_64 (= `alignof
  (max_align_t)`).
- C11 `aligned_alloc(a, sz)` lets you ask for larger alignment; `a` must be
  a power of two and `sz` a multiple of `a`.
- POSIX `posix_memalign(&p, a, sz)` is preferable in pre-C11 codebases.

```c
void *p; posix_memalign(&p, 64, 1024);  // 64-byte aligned, 1 KiB
free(p);
```

`aligned_alloc(64, 100)` is **UB** because 100 is not a multiple of 64; pass
128 instead.

## 9. The classic heap UB demos

These are not theoretical — they cause CVEs every year.

```
   double-free:
   p = malloc(32); free(p); free(p);
   -> tcache push twice -> tcache poisoning -> next malloc returns *your* pointer.

   use-after-free:
   p = malloc(32); free(p);
   strcpy(p, "still writing");
   -> overwrites freelist link; eventual crash or RCE.

   heap-buffer-overflow:
   p = malloc(16); memcpy(p, src, 32);
   -> trashes next chunk's metadata; eventual "malloc(): memory corruption"
      abort, or worse if the attacker chose carefully.

   invalid free:
   free(p + 1);                  // not a chunk pointer
   free(addr_in_text_segment);   // not a chunk pointer
   -> "free(): invalid pointer" abort.

   realloc(p, 0):
   historically equivalent to free(p), returning NULL; in C2x this is UB.
```

glibc has built-in detectors: `glibc.malloc.check`, `MALLOC_CHECK_=3`,
tcache double-free protection (a per-slot "key" tag), unlink macro checks.
They are not security boundaries.

For real protection use **ASan**: `gcc -fsanitize=address -g prog.c`.

## 10. Tuning — `mallopt` and friends

| Knob                 | Env var                 | Meaning |
|----------------------|-------------------------|---------|
| `M_MMAP_THRESHOLD`   | `MALLOC_MMAP_THRESHOLD_` | bytes; bigger -> use mmap |
| `M_MMAP_MAX`         | `MALLOC_MMAP_MAX_`       | max simultaneous mmap chunks |
| `M_TRIM_THRESHOLD`   | `MALLOC_TRIM_THRESHOLD_` | free top-chunk bigger than this -> sbrk back |
| `M_TOP_PAD`          | `MALLOC_TOP_PAD_`        | extra padding on heap grow |
| `M_ARENA_MAX`        | `MALLOC_ARENA_MAX`       | cap on per-thread arenas (1 == single arena) |
| `M_PERTURB`          | `MALLOC_PERTURB_`        | byte to splat in freed chunks (debug) |
| `M_CHECK_ACTION`     | `MALLOC_CHECK_`          | 0..3, abort on heap corruption |

`malloc_stats()` prints a per-arena summary to stderr.

`malloc_trim(0)` asks the allocator to release as much heap to the OS as it
can right now.

```c
mallopt(M_MMAP_THRESHOLD, 4096);
mallopt(M_ARENA_MAX, 1);
malloc_trim(0);
malloc_stats();
```

## 11. Quick exercise

After reading this, look at:

```c
#include <malloc.h>
int main(void) {
    for (int i = 0; i < 1000000; ++i) {
        free(malloc(32));
    }
    malloc_stats();
}
```

You will see *one* arena, *no* heap growth past the first chunk, and an RSS
that stays flat. tcache + fastbins are doing the work.

## 12. Files in this folder

| File | Demonstrates |
|------|--------------|
| [`01_malloc_basics.c`](01_malloc_basics.c)              | `malloc`, `calloc`, `realloc`, `free`, addresses, sizes |
| [`02_chunk_layout.c`](02_chunk_layout.c)                | Print the 16-byte header next to a `malloc`'d pointer |
| [`03_brk_vs_mmap.c`](03_brk_vs_mmap.c)                  | Show small/large `malloc`s landing in heap vs mmap |
| [`04_mallopt_tuning.c`](04_mallopt_tuning.c)            | Tune `M_MMAP_THRESHOLD`, `M_ARENA_MAX`, `M_PERTURB` |
| [`05_mallinfo.c`](05_mallinfo.c)                        | Print `mallinfo2()` and `malloc_stats()` |
| [`06_alignment.c`](06_alignment.c)                      | `aligned_alloc`, `posix_memalign`, default 16-byte alignment |
| [`07_calloc_realloc.c`](07_calloc_realloc.c)            | Zeroing, in-place growth, copy-on-grow |
| [`08_arenas_threads.c`](08_arenas_threads.c)            | Watch arenas multiply when you `malloc` from N threads |
| [`09_tcache_demo.c`](09_tcache_demo.c)                  | Show LIFO behaviour of tcache |
| [`10_double_free.c`](10_double_free.c)                  | Trigger glibc's double-free detector |
| [`11_use_after_free.c`](11_use_after_free.c)            | Show how UaF corrupts the freelist |
| [`12_heap_overflow.c`](12_heap_overflow.c)              | Overflow into the next chunk's metadata |
| [`13_malloc_trim.c`](13_malloc_trim.c)                  | Return heap to OS, watch RSS drop |
