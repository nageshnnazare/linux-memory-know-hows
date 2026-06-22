# 08 — Process Syscalls (fork / exec / wait / clone / signals)

A *process* is the kernel-side data structure (`struct task_struct`) wrapping
"one address space + one or more threads + one set of fds + ...". This
section is how you create, control, and tear down processes from C.

## 1. The life of a process

```
   fork() / clone()        execve()                 _exit()
       |                       |                      |
       v                       v                      v
   +------------+    +-------------------+   +-----------------+
   |  newborn   |--->|  running          |-->|  zombie (Z)     |--> reaped by
   |  child     |    |  R / S / D / T    |   |  exit_code held |    parent's
   +------------+    +-------------------+   +-----------------+    wait()
                          ^                       ^
                          |                       |
                       signals,                  parent must call
                       scheduling                wait/waitpid/waitid
                                                 (or pre-set SA_NOCLDWAIT)
```

Process states (visible in `ps -o pid,stat`):

```
   R   running / runnable
   S   interruptible sleep    (most blocking I/O)
   D   uninterruptible sleep  (I/O that ignores signals; can't kill)
   T   stopped (Ctrl-Z)
   t   traced (gdb attached)
   Z   zombie (exited, parent hasn't waited)
   X   dead (just removed)
```

## 2. `fork()`

```c
pid_t pid = fork();
if (pid < 0)        perror("fork"), exit(1);
else if (pid == 0)  /* child  -- runs in a copy of the parent */
else                /* parent -- pid is the child's PID       */
```

What gets duplicated:

```
   shared (read by both):                copied (CoW until written):
     - file descriptors (refcount++)      - the entire address space
     - controlling tty                      (anon pages, heap, stack
     - signal dispositions                  shared libs)
     - umask, cwd, ...                    - registers
                                            - thread-local storage
   reset in child:
     - pid, ppid
     - file lock state
     - timers, semadj, ...
```

Pictorially:

```
   before fork():                                after fork():
   parent
     +----------------+                          parent             child
     | data  [F1][F2] |                            |                  |
     | heap  [F3][F4] |                            v                  v
     | stack ...      |                          [F1*][F2*]         [F1*][F2*]    <- frames marked RO
     +----------------+                          [F3*][F4*]         [F3*][F4*]       and shared
                                                 stack ...          stack ...
                                                  ^ on first write, frame is copied
```

## 3. `execve()`

Replaces the entire address space with a new program.

```c
char *argv[] = { "ls", "-la", NULL };
char *envp[] = { "PATH=/usr/bin", NULL };
execve("/bin/ls", argv, envp);
perror("execve");   /* only reached on failure */
```

What survives the call:

```
   survives execve:                           gone after execve:
     - pid                                      - text, data, bss, heap, stack
     - ppid                                     - mappings (mmap, mlock)
     - open fds (unless O_CLOEXEC)              - signal handlers (reset to default)
     - cwd, umask, controlling tty              - threads (only main remains)
     - resource limits                          - the whole image
     - signal mask
     - environment (if you pass envp through)
```

Variants:

```
   execve(path, argv, envp)        the syscall
   execv (path, argv)              uses current environ
   execvp(file, argv)              search PATH
   execve(...) with envp = NULL    not portable; pass an empty array
   execvpe(file, argv, envp)       PATH + env
   execl/execle/execlp/execlpe     variadic flavours (NUL-terminated list)
```

## 4. `wait` family — reaping children

```c
int status;
pid_t cpid = waitpid(-1, &status, 0);    // -1 = any child, 0 = block

if (WIFEXITED(status))   printf("exited with %d\n", WEXITSTATUS(status));
if (WIFSIGNALED(status)) printf("killed by signal %d\n", WTERMSIG(status));
if (WIFSTOPPED(status))  printf("stopped by %d\n", WSTOPSIG(status));
if (WIFCONTINUED(status))printf("continued\n");
```

Variants:

```
   wait(&status)                       reap any one child
   waitpid(pid, &s, opt)               specific child or -1 (any)
   waitid(idtype, id, &siginfo, opt)   more info via siginfo_t
   wait3, wait4                        legacy + rusage
```

Options:

```
   WNOHANG     don't block, return 0 if no child to reap
   WUNTRACED   also report stopped children
   WCONTINUED  also report resumed children
```

### Zombies and orphans

- **Zombie**: child exited, parent hasn't waited. Holds onto a tiny kernel
  struct + exit code. State `Z`.
- **Orphan**: parent died first. The orphan is re-parented to `init` (PID 1)
  which always `wait`s.

To prevent zombies in a server you don't care about reaping:

```c
struct sigaction sa = { .sa_handler = SIG_IGN, .sa_flags = SA_NOCLDWAIT };
sigaction(SIGCHLD, &sa, NULL);
```

Or `fork`-twice (the classic daemonisation trick).

## 5. `clone()` — the real syscall

`fork()`, `vfork()`, and `pthread_create()` are all implemented via
`clone(2)`. Its flags pick what is **shared** vs **copied**:

```
   CLONE_VM       address space is shared    (threads, vfork)
   CLONE_FS       cwd, umask, root           (threads)
   CLONE_FILES    fd table                   (threads)
   CLONE_SIGHAND  signal handlers            (threads)
   CLONE_THREAD   same thread group (TGID)   (threads)
   CLONE_PARENT   same ppid as caller
   CLONE_NEWPID   new PID namespace          (containers)
   CLONE_NEWNS    new mount namespace        (containers)
   CLONE_NEWNET   new network namespace
   CLONE_NEWUTS   new hostname namespace
   CLONE_NEWIPC, CLONE_NEWUSER, CLONE_NEWCGROUP
   CLONE_CHILD_CLEARTID  for futex (used by glibc to do pthread_join)
   CLONE_SETTLS         set new TLS (FS base on x86_64)
```

`fork()` = `clone(SIGCHLD)` — nothing shared.
`pthread_create()` ≈ `clone(CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID, stack, ...)`

We have a raw-`clone` example in
[`05_clone_thread.c`](05_clone_thread.c).

## 6. `vfork()`

```c
pid_t pid = vfork();   // child shares VM with parent until execve or _exit
```

- Child runs in the **same address space** as the parent.
- Parent is **suspended** until the child does `execve` or `_exit`.
- Used for the tiny shim "fork-then-exec" without paying the CoW page-table
  copy cost. Modern Linux makes CoW cheap so the benefit is minor.
- Dangerous: in the child you can write to the parent's stack — only call
  exec/_exit, nothing else.

## 7. Signals — interrupting your process

A **signal** is a notification delivered to a process or thread.

```
   when:                       kernel queues a signal
                               +--------+
                               | signal |
                               +--------+
                                    |
                                    v
   ----------------------- process is scheduled -----------------------
                                    |
                                    v
                       is signal in this thread's mask?
                          yes -> stays pending
                          no  -> deliver:
                                   |
                                   +-> default action (SIG_DFL):
                                   |     SIGTERM, SIGINT: term
                                   |     SIGKILL, SIGSTOP: term/stop, NOT catchable
                                   |     SIGCHLD: ignored
                                   |     SIGSEGV: term + coredump
                                   +-> SIG_IGN: ignored
                                   +-> user handler:
                                         saved context, call handler,
                                         then sigreturn back
```

The most common signals:

| Sig         | Default | Cause |
|-------------|---------|-------|
| `SIGHUP`    | term    | terminal closed |
| `SIGINT`    | term    | Ctrl-C |
| `SIGQUIT`   | core    | Ctrl-\ |
| `SIGILL`    | core    | illegal instruction |
| `SIGABRT`   | core    | `abort()`, also from heap corruption |
| `SIGFPE`    | core    | div-by-zero |
| `SIGKILL`   | term    | unblockable kill |
| `SIGSEGV`   | core    | bad memory access |
| `SIGPIPE`   | term    | write to closed pipe |
| `SIGALRM`   | term    | `alarm`/`setitimer` |
| `SIGTERM`   | term    | polite kill |
| `SIGCHLD`   | ign     | child stopped/terminated |
| `SIGSTOP`   | stop    | unblockable stop |
| `SIGCONT`   | cont    | continue |
| `SIGUSR1/2` | term    | application-defined |

### Install a handler — always use `sigaction`, never `signal`

```c
void handler(int s, siginfo_t *si, void *uc) { /* ... */ }
struct sigaction sa = {0};
sa.sa_sigaction = handler;
sa.sa_flags = SA_SIGINFO | SA_RESTART;   // SA_RESTART: auto-restart syscalls
sigemptyset(&sa.sa_mask);
sigaction(SIGTERM, &sa, NULL);
```

### Async-signal-safe functions

The list of functions you can call inside a handler is **tiny**: see
`signal-safety(7)`. `printf`, `malloc`, `free`, `pthread_mutex_lock` are
*not* on it. Stick to `write`, `_exit`, `signalfd`-style designs, or set a
flag (`volatile sig_atomic_t flag = 1`) and handle in the main loop.

## 8. Pipes and `fork` — classic IPC

```c
int fd[2];
pipe(fd);                    // [0] = read, [1] = write
pid_t pid = fork();
if (pid == 0) {
    close(fd[1]);            // child reads
    char buf[64];
    int n = read(fd[0], buf, sizeof buf);
    write(1, buf, n);
    _exit(0);
}
close(fd[0]);                // parent writes
write(fd[1], "hi child\n", 9);
close(fd[1]);                // child reads EOF
waitpid(pid, NULL, 0);
```

Pipe data lives in a small in-kernel circular buffer (64 KiB by default).

```
   parent          kernel              child
   write(fd[1]) -> [ buffer .... ] -> read(fd[0])
                    ^                  ^
                    one fd per end; close both refs to drop
```

## 9. Daemonisation

```
   1. fork ; parent exits      -> child has new ppid (init eventually)
   2. setsid()                 -> become session leader, no controlling tty
   3. fork again ; parent exits-> child no longer session leader,
                                  can't acquire tty by accident
   4. chdir("/")               -> don't pin any filesystem
   5. umask(0)                 -> predictable file perms
   6. close stdin/out/err or redirect to /dev/null
   7. open syslog / log file
```

See [`07_daemonise.c`](07_daemonise.c).

## 10. Files in this folder

| File | Demonstrates |
|------|--------------|
| [`01_fork.c`](01_fork.c)                 | Hello world from parent + child |
| [`02_fork_exec_wait.c`](02_fork_exec_wait.c) | The canonical fork-exec-wait pattern |
| [`03_vfork.c`](03_vfork.c)               | `vfork()` followed by `execve` |
| [`04_clone_raw.c`](04_clone_raw.c)       | Call `clone(2)` directly to make a process |
| [`05_clone_thread.c`](05_clone_thread.c) | Use `clone` to make a "thread" (shared VM) |
| [`06_signal_handler.c`](06_signal_handler.c) | Catch SIGINT, SIGTERM with `sigaction` |
| [`07_daemonise.c`](07_daemonise.c)       | Double-fork daemon recipe |
| [`08_pipe_fork.c`](08_pipe_fork.c)       | Pipe between parent and child |
| [`09_zombie_orphan.c`](09_zombie_orphan.c)| Demonstrate zombies and re-parenting |
| [`10_sigchld.c`](10_sigchld.c)           | Reap children asynchronously via SIGCHLD |
