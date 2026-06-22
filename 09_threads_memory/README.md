# 09 — Threads from a Memory Perspective

We covered threads from a *synchronisation* perspective in the sibling
[threading_guide](../../threading_guide/). Here we look at them strictly from
the **memory** side: where does a thread's stack live, what is TLS, how does
glibc actually create a thread, and what is a futex.

## 1. What gets shared

```
   Process address space (ONE PML4 / CR3 / page tables)
   +-------------------------------------------------------------+
   | text, rodata, data, bss   - shared                          |
   | heap (brk + arenas)       - shared                          |
   | mmap regions              - shared                          |
   | fds (kernel side)         - shared                          |
   | signal handlers           - shared (signal mask is PER-thread!) |
   |                                                             |
   | thread A: stack + regs + TLS                                |
   | thread B: stack + regs + TLS                                |
   | thread C: stack + regs + TLS                                |
   +-------------------------------------------------------------+
```

Concretely, every thread is its own `struct task_struct` (kernel) and Linux
schedules tasks, not processes. A "process" is a *thread group*: tasks
sharing a TGID (the process's PID).

## 2. The thread stack

The main thread's stack is the `[stack]` VMA up at the top of the address
space. Every additional thread gets its **own mmap'd region**:

- Default size: `RLIMIT_STACK`, typically 8 MiB.
- A 4 KiB **guard page** with `PROT_NONE` at the low end.
- The TCB (thread control block, glibc internal) lives at the *high* end.

```
   thread stack VMA (one per non-main thread)
   +-----------------------+   <- high addr; TCB + dtv + ... live here
   |    TCB (glibc)        |
   +-----------------------+
   |    TLS data           |
   +-----------------------+
   |    stack frames       |    grows down as the thread runs
   |    ...                |
   +-----------------------+
   |    free / unused      |
   +-----------------------+
   |    guard page (---p)  |   touching this -> SIGSEGV
   +-----------------------+
```

Set explicitly:

```c
pthread_attr_t a;
pthread_attr_init(&a);
pthread_attr_setstacksize(&a, 256 * 1024);
pthread_attr_setguardsize(&a, 4096);
pthread_create(&tid, &a, fn, NULL);
pthread_attr_destroy(&a);
```

## 3. Thread Local Storage (TLS)

TLS gives each thread its own copy of a variable.

```c
// C11
_Thread_local int per_thread_x;

// GCC
__thread int per_thread_y;

// C++
thread_local int per_thread_z;
```

How does the runtime find "my" copy? On x86\_64, the `FS` segment register
points to the **TCB**, which holds the offset of each TLS variable. So:

```
   mov  %fs:per_thread_x@tpoff, %eax     // load this thread's per_thread_x
```

There are *two* TLS models you'll encounter:

- **Initial-Exec / Local-Exec**: TLS slot allocated at load time, offsets
  computed by the static linker. Fast (just `%fs:offset`).
- **Global-Dynamic / Local-Dynamic**: TLS allocated for `dlopen`'d libraries.
  Slower (`__tls_get_addr` runtime resolver).

Use `-ftls-model=initial-exec` for hot per-thread state if you control
linkage.

```
   thread A's TCB              thread B's TCB
   +---------------+           +---------------+
   | dtv pointer   |           | dtv pointer   |
   +---------------+           +---------------+
   | per_thread_x  |           | per_thread_x  |   <- two distinct words
   +---------------+           +---------------+
   | per_thread_y  |           | per_thread_y  |
   +---------------+           +---------------+
   ...                         ...
   %fs in A points here        %fs in B points here
```

### Posix TLS (`pthread_key_t`)

Used pre-C11; supports a destructor per key:

```c
pthread_key_t key;
pthread_key_create(&key, free);    // destructor called at thread exit
pthread_setspecific(key, malloc(64));
void *p = pthread_getspecific(key);
```

Use compiler TLS (`__thread`) whenever possible — it's faster and cleaner.

## 4. The `pthread_create` lowdown

```
   pthread_create(&tid, attr, fn, arg)
       |
       v
   glibc:
       - allocate stack (mmap, with guard page)
       - allocate TCB at top of stack
       - set up signal mask, TLS
       - clone(CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|
               CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|
               CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID, stack_top,
               &tid, tls_desc, &child_tid)
                                ^                    ^
                                |                    |
                       stored in *tid       cleared on thread exit,
                                            then futex_wake -> pthread_join sees 0
```

`pthread_join(tid, &ret)` blocks on a futex (the same child_tid word) and
copies out the return value.

## 5. Futexes — fast userspace mutexes

A **futex** ("fast userspace mutex") is the primitive every pthread mutex,
condvar, and `std::atomic` wait is built on.

```
   user: an int (the "futex word") in shared memory
   user: atomic compare-and-set on it (fast path, no syscall)
   user: if uncontended: lock acquired -- never enter the kernel.
   user: if contended  : call futex(2):
            FUTEX_WAIT  -- check val matches expected, sleep if so
            FUTEX_WAKE  -- wake N waiters on the word
```

Pseudocode of a mutex:

```c
void lock(_Atomic int *m) {
    int expected = 0;
    if (atomic_compare_exchange_strong(m, &expected, 1)) return;       // uncontested
    while (atomic_exchange(m, 2) != 0) {                               // mark contended
        syscall(SYS_futex, m, FUTEX_WAIT, 2, NULL, NULL, 0);           // sleep
    }
}
void unlock(_Atomic int *m) {
    if (atomic_exchange(m, 0) != 1)                                    // had waiters
        syscall(SYS_futex, m, FUTEX_WAKE, 1, NULL, NULL, 0);
}
```

States of the futex word:
- `0` = unlocked
- `1` = locked, no waiters
- `2` = locked, possible waiters

The kernel keeps a hash table of waiters keyed by **physical address** of the
futex word, so the same futex can be used cross-process (when the word is in
shared memory like `mmap(MAP_SHARED)`).

See [`05_futex_basic.c`](05_futex_basic.c).

## 6. Memory ordering primer (cross-thread)

Two threads communicating via shared memory must agree on a *memory model*.
On x86\_64 the hardware is **TSO** (total store order): every core sees the
same global order of writes; the only reordering is "store followed by load
to different addresses".

In C, use **`_Atomic`** (C11) or **C11 atomics**:

```c
#include <stdatomic.h>
_Atomic int flag;
atomic_store_explicit(&flag, 1, memory_order_release);   // producer
while (!atomic_load_explicit(&flag, memory_order_acquire)) ; // consumer
// at this point consumer sees everything producer did before the store
```

Memory orders (`stdatomic.h`):

```
   memory_order_relaxed   only the operation is atomic; no ordering
   memory_order_acquire   pairs with release; later loads ordered after this load
   memory_order_release   pairs with acquire; prior stores ordered before this store
   memory_order_acq_rel   for RMW (fetch_add, exchange, CAS)
   memory_order_seq_cst   sequentially consistent (default)
```

Full coverage in the sibling [threading_guide](../../threading_guide/).

## 7. Files

| File | Demonstrates |
|------|--------------|
| [`01_pthread_create.c`](01_pthread_create.c) | Spawn threads, print stack ranges |
| [`02_tls_compiler.c`](02_tls_compiler.c)     | `__thread` / `_Thread_local` per-thread variables |
| [`03_tls_pthread_key.c`](03_tls_pthread_key.c) | `pthread_key_create` + destructor |
| [`04_pthread_join.c`](04_pthread_join.c)     | Join, get return value |
| [`05_futex_basic.c`](05_futex_basic.c)       | Build a tiny lock with `SYS_futex` |
| [`06_mutex_vs_atomic.c`](06_mutex_vs_atomic.c)| Benchmark uncontended pthread mutex vs atomic |
