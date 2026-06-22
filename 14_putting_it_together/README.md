# 14 — Putting it Together: Write Your Own Allocators

You can't say you understand memory until you've written an allocator.
This section builds five classic allocators from scratch, on top of `mmap`.

```
   allocator          best for                       complexity
   -----------        ---------                      ----------
   bump (arena)       short-lived burst; freed all
                      at once                        ~30 lines
   pool (freelist)    fixed-size objects             ~60 lines
   slab               sets of same-size objects;
                      cache-line aware               ~120 lines
   freelist           variable-size; learning ptmalloc ~150 lines
   buddy              power-of-2 size; OS-level      ~200 lines
```

All examples are *educational*, not production-quality. They cover the
algorithms; thread safety / shrink / error checks are stubs at best.

## 1. Bump allocator (arena)

A *region* of memory; `alloc` just advances a pointer. There is no
individual `free`; you drop the whole region with `arena_destroy`.

```
   +-----------------------+
   | used                  | <- bump pointer advances ->|       free       |
   +-----------------------+----------------------------+------------------+
   ^                                                    ^                  ^
   base                                              top                  end
```

Pros: O(1) alloc, no metadata per object, perfect locality.
Cons: cannot free individual objects; only good for "one big phase".

See [`01_bump.c`](01_bump.c).

## 2. Pool allocator (fixed-size freelist)

If you allocate many objects of the same size (e.g. AST nodes, network
packet buffers), a *pool* is ideal:

```
   pool: array of N slots of size S, plus a freelist head

   +-----+ +-----+ +-----+ +-----+
   | s 0 | | s 1 | | s 2 | | s 3 |  <- the slab
   +-----+ +-----+ +-----+ +-----+
      ^
      free: 0 -> 2 -> NULL    (freelist threads through unused slots)
              ^- in use
```

Alloc = pop head. Free = push onto head. Both O(1).

See [`02_pool.c`](02_pool.c).

## 3. Slab allocator (Bonwick-style)

Each "cache" is a list of *slabs* (page-sized regions) carved into
fixed-size objects with a freelist per slab. Each slab additionally tracks
constructor/destructor hooks so reuse can preserve invariants.

This is essentially what the Linux kernel's `kmalloc` uses.

See [`03_slab.c`](03_slab.c).

## 4. Freelist allocator (mini-malloc)

A real `malloc`: variable sized, coalesces adjacent free chunks, uses a
first-fit or best-fit search. Each chunk has a header so `free(p)` can find
the size from `p - sizeof(header)`.

```
   +---------+-----------------+---------+-----------------+---------+
   | header  |  payload        | header  |  payload        | header  |
   | size, F |  (in use OR     | size, F | (in use OR      | size, F |
   |         |   free->fd/bk)  |         |  free->fd/bk)   |         |
   +---------+-----------------+---------+-----------------+---------+
   ^                                                                ^
   base                                                          top
```

See [`04_freelist.c`](04_freelist.c).

## 5. Buddy allocator

The classic OS allocator. Memory is a power-of-2; split into halves
recursively as needed; free coalesces "buddies".

```
   start: order 4 (16 pages)
   alloc 3 pages -> need order 2 (4 pages)
     split 16 -> 8|8
     split 8 (lower) -> 4|4
     return first 4 pages, mark used

   free those 4 -> recurse: buddy is its partner of order 2
     buddy free? yes -> merge to order 3 (8 pages)
                       buddy free? yes -> merge to order 4
                       etc.
```

See [`05_buddy.c`](05_buddy.c).

## 6. How real allocators are built

```
   glibc ptmalloc2     -- arenas + chunks + tcache (per-thread caches)
   jemalloc            -- per-thread arenas + size classes + slabs
   tcmalloc            -- thread-local caches + central cache + heap
   mimalloc            -- size classes + page-based + sharded
   musl                -- modest size-bucket freelist
   Linux kernel        -- SLUB (default) / SLAB / SLOB; buddy at page-frame level
```

The patterns repeat: **per-thread cache** for speed, **size classes** to
keep fragmentation bounded, **page-sized backing** so the OS can reclaim
in chunks.

## Files

| File | Lines | Demonstrates |
|------|------:|--------------|
| [`01_bump.c`](01_bump.c)         | ~40  | Linear arena allocator |
| [`02_pool.c`](02_pool.c)         | ~70  | Fixed-size pool / freelist |
| [`03_slab.c`](03_slab.c)         | ~140 | Bonwick-style slab |
| [`04_freelist.c`](04_freelist.c) | ~200 | Variable-size freelist with coalescing |
| [`05_buddy.c`](05_buddy.c)       | ~200 | Power-of-2 buddy allocator |
