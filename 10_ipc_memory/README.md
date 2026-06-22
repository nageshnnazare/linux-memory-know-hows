# 10 — IPC: Shared Memory & Other Ways Processes Talk

When two processes need to exchange data, you have an *embarrassment* of
options on Linux. Pick the one that matches your data shape and lifetime.

```
                  How big is each message?
                       |
       small (bytes)   |   large (MB+) / streaming
            v          |          v
         signals       |       mmap + shared memory
         eventfd       |       memfd_create
         signalfd      |
            |          |
            v          |          v
         pipes / FIFO  |       MAP_SHARED file
         UNIX sockets  |
         msg queues    |
         dgram sockets |
            |          |
            v          |          v
                  unidirectional / structured  -> sockets, message queues
                  full-duplex streaming         -> socketpair
                  one-shot signal               -> eventfd, signalfd
```

## 1. Pipes (`pipe2`)

```
   pipe(fd)              // [0]=read, [1]=write
   pipe2(fd, O_CLOEXEC | O_NONBLOCK)   // Linux: extra flags

   - in-kernel buffer (64 KiB default; F_SETPIPE_SZ to grow)
   - unidirectional
   - usually paired with fork(); one process closes [1], other [0]
```

A *named pipe* (FIFO) lives in the filesystem: `mkfifo /tmp/p`, then any
process can `open("/tmp/p", O_RDONLY/O_WRONLY)`.

## 2. UNIX domain sockets

Full-duplex, can be stream or datagram, can carry **fds** (the magic
`SCM_RIGHTS`!).

```
   socketpair(AF_UNIX, SOCK_STREAM, 0, fd)   // anonymous, bidirectional pair
   socket(AF_UNIX, SOCK_STREAM/DGRAM, 0)     // named, bind to /tmp/...
```

[`02_socketpair.c`](02_socketpair.c).

## 3. POSIX shared memory (`shm_open`)

This is the *modern* way to share a large memory region between unrelated
processes.

```c
fd = shm_open("/myseg", O_CREAT|O_RDWR, 0600);
ftruncate(fd, size);
void *p = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
close(fd);
...
munmap(p, size);
shm_unlink("/myseg");    // remove the name
```

Backed by `tmpfs` at `/dev/shm/`. Survives process exit until `shm_unlink`.

## 4. SysV shared memory (`shmget` / `shmat`)

Legacy. Still around for compatibility (PostgreSQL uses it, Oracle, ...).

```c
key_t k = ftok("/some/file", 1);
int id = shmget(k, size, IPC_CREAT | 0600);
void *p = shmat(id, NULL, 0);
...
shmdt(p);
shmctl(id, IPC_RMID, NULL);
```

Inspect: `ipcs -m`. Tweak limits: `/proc/sys/kernel/shmmax`, `shmall`, `shmmni`.

Prefer POSIX `shm_open` for new code.

## 5. `memfd_create` — file-less files

```c
int fd = memfd_create("scratch", MFD_CLOEXEC);
ftruncate(fd, 1 << 20);
void *p = mmap(NULL, 1 << 20, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
```

Anonymous, in-RAM, no filesystem path; pass `fd` over a UNIX socket
(`SCM_RIGHTS`) to share. Also supports **seals** (`F_ADD_SEALS`) so the
receiver can verify nobody can modify or resize it. Used by Wayland,
container runtimes, KVM/libvirt.

## 6. `eventfd` — a counter you can poll on

```c
int e = eventfd(0, EFD_CLOEXEC);
write(e, &(uint64_t){1}, 8);   // increment by 1
read (e, &(uint64_t){0}, 8);   // returns current value, sets it to 0
poll/epoll(e)                  // becomes readable when value > 0
```

Great for "thread A wants to wake up thread B" with `epoll`.

## 7. `signalfd` — signals via fd

```c
sigset_t s; sigemptyset(&s); sigaddset(&s, SIGTERM);
sigprocmask(SIG_BLOCK, &s, NULL);  // important
int sfd = signalfd(-1, &s, SFD_CLOEXEC);
struct signalfd_siginfo si; read(sfd, &si, sizeof si);
```

Now `SIGTERM` is delivered through a file descriptor — no signal handler, no
async-signal-safety constraints.

## 8. POSIX message queues (`mq_open`)

```c
mqd_t mq = mq_open("/q", O_CREAT|O_RDWR, 0600, &(struct mq_attr){.mq_maxmsg=8,.mq_msgsize=64});
mq_send(mq, msg, len, prio);
mq_receive(mq, buf, sizeof buf, &prio);
mq_unlink("/q");
```

Kernel-managed; persistent until unlink; bounded; supports priorities. Inspect:
`ls /dev/mqueue/`.

## 9. Pipes vs sockets vs shm — decision tree

```
   "I need to copy bytes between procs"
       smallish (< pagesize), one-way            -> pipe / FIFO
       smallish, bidirectional                   -> socketpair
       smallish, between unrelated procs         -> AF_UNIX socket on disk
       structured w/ priorities + persistence    -> mq_open
   "I need to share a big buffer (no copy)"
       related (fork)        -> mmap(MAP_SHARED|MAP_ANONYMOUS) before fork
       unrelated processes   -> shm_open + mmap, or memfd + SCM_RIGHTS
       persistent across boot-> mmap a real file on disk
   "I need to pass FDs"
       socketpair + SCM_RIGHTS
   "Just wake another thread / proc"
       eventfd + poll/epoll
```

## 10. Files

| File | Demonstrates |
|------|--------------|
| [`01_pipe.c`](01_pipe.c)                | Basic pipe between parent / child |
| [`02_socketpair.c`](02_socketpair.c)    | Full-duplex AF_UNIX socketpair |
| [`03_fifo.c`](03_fifo.c)                | Named pipe (`mkfifo`) |
| [`04_shm_open.c`](04_shm_open.c)        | POSIX shared memory, two-process demo |
| [`05_sysv_shm.c`](05_sysv_shm.c)        | SysV `shmget/shmat` |
| [`06_memfd_create.c`](06_memfd_create.c)| `memfd_create` + seals |
| [`07_eventfd.c`](07_eventfd.c)          | Use eventfd to wake threads |
| [`08_signalfd.c`](08_signalfd.c)        | Receive SIGTERM via signalfd |
| [`09_mq.c`](09_mq.c)                    | POSIX message queue |
| [`10_scm_rights.c`](10_scm_rights.c)    | Pass an fd over a UNIX socket |
