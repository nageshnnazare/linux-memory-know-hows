# 06 — `brk` and `sbrk` — the program break

`brk(2)` and `sbrk(2)` are the two oldest memory syscalls in UNIX. They set
the **program break** — the highest virtual address used by the data segment
(heap). `malloc` was originally just a thin wrapper around them. Modern
glibc still uses `brk` for the main arena.

## 1. The picture

```
   process VA (excerpt around heap)
        ...
        +-------------------------+   <- start_brk  (top of .bss/.data)
        |       in-use heap       |
        |                         |
        |                         |
        +-------------------------+   <- brk(0) / sbrk(0) -- current break
        |                         |
        |                         |
        +-------------------------+   <- never used yet (still virtual hole)
        ...
```

- `brk(addr)`: set the break to `addr`. Return 0 on success, -1 on error.
- `sbrk(incr)`: increase break by `incr` bytes, return the **old** break.
  `sbrk(0)` returns the current break without changing it.

In `/proc/self/maps`:

```
   555a3b9b1000-555a3b9d2000 rw-p 00000000 00:00 0      [heap]
                  ^                                       ^
                  start_brk                              current break
```

## 2. Why is `sbrk` "deprecated"?

It isn't *technically* deprecated, but:

- It's not thread-safe. Two threads calling `sbrk` simultaneously could
  collide. glibc takes the malloc arena lock around it.
- It only grows in one direction (up). You can shrink with `brk(smaller)`,
  but only if everything above the new break is unused.
- It only grows one **contiguous** region. If something (`mmap`'d library
  segment, thread arena) ends up directly above the heap, `brk` can't grow
  further — `malloc` will fall back to `mmap`.

For these reasons, modern allocators (jemalloc, mimalloc, tcmalloc) avoid
`brk` entirely. glibc keeps using it for the main arena only.

## 3. Worked example

```c
void *base = sbrk(0);
sbrk(4096);                   // grow break by one page
*(char *)base = 'A';          // valid: that page belongs to us
sbrk(-4096);                  // shrink back
*(char *)base = 'A';          // !!! UB: page no longer owned
```

The kernel doesn't necessarily unmap on `brk(smaller)` — it may keep the
pages mapped until pressure forces a reclaim — but reading from them is UB.

## 4. Sequence diagram

```
   user             glibc                     kernel
   ----             -----                     ------
   malloc(N)
     |--->          ptmalloc lock
                    no chunk fits
                    -- need to grow main arena --
                    sbrk(M)   ----------------> set break += M
                    <----------------- old break (= chunk addr)
                    initialise chunk header
                    unlock
   <----------------  payload
```

## 5. Files

| File | Demonstrates |
|------|--------------|
| [`01_program_break.c`](01_program_break.c) | Print `sbrk(0)` before/after small/large `malloc` |
| [`02_brk_directly.c`](02_brk_directly.c)   | Allocate memory by calling `brk` directly (no malloc) |
