# 03 — The Stack

The stack is the single most important data structure in any C program. Every
function call uses it; every local variable lives in it; every argument is
passed via it (or via registers, on x86\_64). Get this right and you'll
understand crashes, security bugs, and threading from first principles.

## 1. What the stack actually is

On Linux x86\_64 the main-thread stack is a **single VMA** at a high address,
created by the kernel at `execve` time. It has:

- A fixed reserve (`RLIMIT_STACK`, typically 8 MiB).
- A **guard page** of `PROT_NONE` just below it. Touch it -> `SIGSEGV`.
- The `MAP_GROWSDOWN` flag (historically) so the kernel can extend it on
  demand if you keep pushing.

```
   high addr  +-------------------------+
              |   args / env / auxv     |   set up by kernel at execve
              +-------------------------+
              |        ...  (in use)    |
              |   stack frame for main  |   <- %rbp
              |   ---- ret addr ----    |
              |   stack frame for foo   |   <- pushed by call
              |   ---- ret addr ----    |
              |   stack frame for bar   |
              |   ...                   |
   %rsp ----> +-------------------------+
              |   (unused, but mapped)  |
              |   ...                   |
              +-------------------------+
              |   guard page PROT_NONE  |  <-- touching this -> SIGSEGV
              +-------------------------+
   low addr   ...
```

## 2. Anatomy of a single stack frame (System V x86\_64 ABI)

```
   higher addr
   +------------------------------+
   |   caller's args 7,8,9...     |   (first 6 go in regs: rdi,rsi,rdx,rcx,r8,r9)
   +------------------------------+
   |   return address (8B)        |   pushed by `call`
   +------------------------------+
   |   saved RBP of caller (8B)   |   pushed by callee:  push %rbp
   +------------------------------+  <- %rbp of callee (the "frame pointer")
   |   callee-saved registers     |   only if used: rbx, r12-r15
   +------------------------------+
   |   local variables            |
   |     ...                      |
   |   alignment padding          |
   +------------------------------+  <- %rsp (always 16-byte aligned before call)
   lower addr
```

Calling conventions in one sentence: **integer args go in
`rdi, rsi, rdx, rcx, r8, r9`; FP args in `xmm0..xmm7`; everything past that
goes on the stack; return value in `rax`; the stack must be 16-byte aligned
just before a `call`.**

### Variable lifetime

```
   void f(void) {
       int a = 1;        // a is at, say, %rbp-4
       int b = 2;        // b is at, say, %rbp-8
       g();              // g() pushes more stuff, but f's frame is preserved
   }                     // f returns: leave; ret -- %rsp now points back to caller
```

Note: in optimised builds the compiler often **omits the frame pointer**
(`-fomit-frame-pointer`, default at `-O1+`). Then `%rbp` is just a general
register and the unwinder relies on `.eh_frame` DWARF tables.

## 3. Direction of growth

x86\_64: stack grows **downward** (subtract from `rsp`).

```
   void recurse(int n) {
       int local;
       printf("depth=%d, &local=%p\n", n, &local);
       if (n > 0) recurse(n - 1);
   }

   output:
       depth=3, &local=0x7ffe...c
       depth=2, &local=0x7ffe...0   (down 12 or 16 bytes, depending on layout)
       depth=1, &local=0x7ffe...4
       depth=0, &local=0x7ffe...8
```

See [`02_recursion_depth.c`](02_recursion_depth.c) to see exactly how much
stack each call uses.

## 4. Stack overflow

The stack is `RLIMIT_STACK` big. Recurse too deeply (or allocate too large a
local with `alloca`/VLA) and you'll **fall off the end** — usually a
`SIGSEGV` because you wrote into the guard page.

```
   stack VMA
   +------------------------------+
   |       (used by frames)       |
   |   .                          |
   |   .                          |
   |   .                          |
   +-- guard page PROT_NONE ------+    <- touching this kills you
```

To detect it gracefully, install a `SIGSEGV` handler **on an alternate stack**
with `sigaltstack`. We do that in
[`../11_memory_protection/04_segfault_handler.c`](../11_memory_protection/).

See [`04_stack_overflow.c`](04_stack_overflow.c) for a controlled crash demo.

## 5. `alloca`, VLAs, large locals

```c
void f(size_t n) {
    char buf[n];          // VLA      -- allocated on stack
    char *q = alloca(n);  // alloca   -- like VLA, freed at function return
    ...
}
```

Both grow the stack by `n` bytes. There is **no bounds check**; calling
`alloca(SIZE_MAX)` just smashes your stack. Worse, the kernel only extends the
stack by one page per fault, so a single huge `alloca` may jump *past* the
guard page and land in another VMA — a security catastrophe. That's why GCC
inserts **`-fstack-clash-protection`**: a probe instruction every page so the
guard page is always hit first.

```
   bad (no clash protection):              good (with clash protection):
   sub  $0x100000, %rsp                   sub  $0x1000, %rsp; orq $0, (%rsp)
   ; uh oh, may have stepped over guard   ; repeat -- each page is probed
```

See [`03_alloca_vs_vla.c`](03_alloca_vs_vla.c).

## 6. Stack canaries

Compile with `-fstack-protector-strong` (default on most distributions). GCC
inserts a random value, the **canary**, between the locals and the saved
return address. If a buffer overflow overwrites the return address, it also
overwrites the canary; the function epilogue checks it and calls
`__stack_chk_fail` (aborts with a diagnostic).

```
   without canary:                with canary:
   +-------------------+          +-------------------+
   | return addr       |          | return addr       |
   +-------------------+          +-------------------+
   | saved rbp         |          | saved rbp         |
   +-------------------+          +-------------------+
   |                   |          | CANARY (random)   |  <- checked before ret
   | locals / buffers  |          +-------------------+
   |                   |          |                   |
   +-------------------+          | locals / buffers  |
                                  |                   |
                                  +-------------------+
```

See [`06_stack_canary.c`](06_stack_canary.c).

## 7. Thread stacks

The **main** thread's stack is the VMA above. Each `pthread_create` (or raw
`clone(CLONE_VM|...)`) thread gets its **own** stack — by default an
8 MiB anonymous mmap with a 4 KiB guard page at the low end.

```
   process address space (high addr)
   +---------------------------+
   |  main thread stack        |
   +---------------------------+
   |  (mmap region)            |
   |    thread 3 stack         |
   |    thread 2 stack         |
   |    thread 1 stack         |
   |  ...                      |
```

Set explicitly:

```c
pthread_attr_t a; pthread_attr_init(&a);
pthread_attr_setstacksize(&a, 64 * 1024);
pthread_attr_setguardsize(&a, 4096);
pthread_create(&tid, &a, fn, NULL);
pthread_attr_destroy(&a);
```

See [`07_pthread_stack.c`](07_pthread_stack.c).

## 8. What goes wrong on the stack — a checklist

```
   * recursion w/o a base case        -> stack overflow
   * VLA / alloca of attacker-sized n -> stack clash, RCE
   * returning &local                 -> use-after-return (UB)
   * buffer overflow in local buf[]   -> overwrite return addr (CWE-121)
   * memcpy(buf, src, attacker_len)   -> same; canary helps detect
   * very large local arrays          -> RLIMIT_STACK; move them to heap
```

Returning a pointer to a local **never** works; the storage is gone the
instant the function returns:

```c
char *bad(void) { char buf[16]; return buf; }   // dangling pointer
```

See [`05_dangling_local.c`](05_dangling_local.c) (UB demo; behaviour depends
on optimiser/aslr).

## 9. The red zone

x86\_64 ABI guarantees the **128 bytes below `%rsp`** are safe to use by leaf
functions without adjusting the stack pointer. That's the *red zone*. It
saves an `add/sub rsp` for tiny leaf functions. Disabled in kernel code
(`-mno-red-zone`).

```
   %rsp -------> +-----------------------+
                 |  red zone, 128 bytes  |  safe for leaf functions
                 +-----------------------+
                 |  rest of stack...     |
```

## 10. Files in this folder

| File | Demonstrates |
|------|--------------|
| [`01_stack_growth.c`](01_stack_growth.c)         | Print `&local` at increasing depth to see growth direction |
| [`02_recursion_depth.c`](02_recursion_depth.c)   | Measure how deep you can recurse before SIGSEGV |
| [`03_alloca_vs_vla.c`](03_alloca_vs_vla.c)       | Compare `alloca`, VLA, and `malloc` |
| [`04_stack_overflow.c`](04_stack_overflow.c)     | Cause a controlled stack overflow (caught with sigaltstack) |
| [`05_dangling_local.c`](05_dangling_local.c)     | Return a pointer to a stack local (UB) |
| [`06_stack_canary.c`](06_stack_canary.c)         | Show `__stack_chk_fail` triggering on overflow |
| [`07_pthread_stack.c`](07_pthread_stack.c)       | Show a thread's stack address, set custom stack size |
