# 11 — Memory Protection

The 1980s ended thirty-some years ago. Memory accesses in user space are
now policed by a stack of defences:

```
   +-----------------------------+
   |  ASLR (random layout)       |   makes addresses unpredictable
   +-----------------------------+
   |  NX / W^X                   |   data pages are non-executable
   +-----------------------------+
   |  stack canaries             |   detect overflow before return
   +-----------------------------+
   |  RELRO + Full RELRO         |   GOT/PLT made read-only after load
   +-----------------------------+
   |  Fortify (libc)             |   compile-time + runtime checks of
   |                             |   memcpy, sprintf, ...
   +-----------------------------+
   |  PIE                        |   executable's own base also randomised
   +-----------------------------+
   |  Sanitizers (ASan/UBSan/    |   developer tools; not production
   |  MSan/TSan)                 |
   +-----------------------------+
```

This section explains each one and shows how to use it (or break it for
testing).

## 1. `mprotect` revisited

We've used it for guard pages. It changes the perm bits in the PTE for the
requested range.

```c
mprotect(addr, len, PROT_READ);                 // RO
mprotect(addr, len, PROT_READ|PROT_WRITE);      // RW
mprotect(addr, len, PROT_READ|PROT_EXEC);       // RX (JIT)
mprotect(addr, len, PROT_NONE);                 // no access (guard)
```

Requirements:
- `addr` must be page-aligned.
- `len` is rounded up.
- Removing all perms doesn't unmap, just makes the pages inaccessible until
  you change them back.

## 2. NX / W^X / DEP

The CPU's NX bit (bit 63 of PTE) marks pages non-executable. Any
fetch-as-instruction from such a page raises `#PF`.

```
   ROP attack tries: jmp into a writable buffer  -> trap.
   Old worms put shellcode on the stack and jumped to it; NX defeats that.
```

Linux respects the NX bit when it's available; the ELF `GNU_STACK` segment
flags decide whether the stack is X. By default it's `RW`, no `X` —
"executable stack" requires explicit linker flags (`-z execstack`) and is
visible in `readelf -l a.out`.

## 3. ASLR — three levels

```
   /proc/sys/kernel/randomize_va_space
     0 = off
     1 = stack, mmap, vdso randomised; heap NOT randomised
     2 = all of the above + brk-managed heap (default)
```

For ASLR to apply to your executable's base, it must be a **PIE** (compiled
with `-fPIE -pie`). Modern distros build PIE by default.

See [`02_aslr_test.c`](02_aslr_test.c).

## 4. Stack canaries — `-fstack-protector-{strong,all}`

Already covered in §03. To turn ON (default on Debian/Ubuntu/RHEL):

```
   -fstack-protector-strong      -- protects functions with arrays / pointers
   -fstack-protector-all         -- protects ALL functions
   -fstack-protector-explicit    -- only functions tagged
                                    __attribute__((stack_protect))
   -fno-stack-protector          -- turn OFF (for benchmarking; not for prod)
```

## 5. RELRO

The linker initially makes the GOT writable so the runtime can resolve
symbols lazily. Once resolution is done, `mprotect` flips it to RO. With
**Full RELRO** all resolution is forced at load time and the GOT is RO
ever after.

Build:

```
   -Wl,-z,relro          partial RELRO
   -Wl,-z,relro,-z,now   full RELRO (no lazy binding)
```

Check: `checksec --file=./a.out`.

## 6. Fortify Source (`_FORTIFY_SOURCE`)

A glibc + compiler feature that replaces variants of `memcpy`, `strcpy`,
`sprintf`, etc. with versions that take a "destination size" argument.

```c
char buf[16];
strcpy(buf, "this string is much too long"); // _FORTIFY_SOURCE catches it
*** buffer overflow detected ***: terminated
Aborted
```

Compile with `-D_FORTIFY_SOURCE=2 -O2` (most distros enable this by default).

## 7. Sigaltstack — handle signals when the stack is sick

If you `SIGSEGV` because your stack is broken, the default signal handler
runs on the *same* broken stack — and dies. Solution:

```c
static char altstk[SIGSTKSZ];
stack_t ss = { .ss_sp = altstk, .ss_size = sizeof altstk };
sigaltstack(&ss, NULL);

struct sigaction sa = {0};
sa.sa_sigaction = h;
sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
sigaction(SIGSEGV, &sa, NULL);
```

Now `h` runs on `altstk`, far from the corrupted stack.

## 8. Building hard, recommended flags

```bash
gcc -O2 -g -D_FORTIFY_SOURCE=2 \
    -fstack-protector-strong \
    -fstack-clash-protection \
    -fcf-protection=full \
    -fPIE -pie \
    -Wl,-z,relro,-z,now,-z,noexecstack,-z,separate-code \
    prog.c -o prog
```

Compare with `checksec`:

```
   Stack:    Canary found
   NX:       NX enabled
   PIE:      PIE enabled
   RELRO:    Full RELRO
   Fortify:  Yes
```

## 9. Files

| File | Demonstrates |
|------|--------------|
| [`01_mprotect_jit.c`](01_mprotect_jit.c)   | Write code into anon mmap, flip to PROT_READ\|PROT_EXEC, jump in |
| [`02_aslr_test.c`](02_aslr_test.c)         | Print addresses; run several times to see randomness |
| [`03_segfault_handler.c`](03_segfault_handler.c) | `SIGSEGV` handler on alternate stack |
| [`04_canary_demo.c`](04_canary_demo.c)     | Overflow triggers `__stack_chk_fail` |
| [`05_fortify_demo.c`](05_fortify_demo.c)   | Fortify catches a too-long `strcpy` |
| [`06_w_xor_x.c`](06_w_xor_x.c)             | Try to execute a writable page (fails) |
