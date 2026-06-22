/*
 * 05_sysv_shm.c
 * --------------
 * Legacy SysV shared memory.  Inspect with `ipcs -m`.
 *
 *   key  = ftok(path, id)       derive an integer key from a file
 *   id   = shmget(key, sz, IPC_CREAT|0600)
 *   p    = shmat(id, NULL, 0)
 *   ... use p ...
 *   shmdt(p)
 *   shmctl(id, IPC_RMID, NULL)  remove segment
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>
#include <stdatomic.h>

struct s { _Atomic int ready; char msg[64]; };

int main(void)
{
    key_t k = IPC_PRIVATE;     /* anonymous, inherited via fork */
    int id = shmget(k, sizeof(struct s), IPC_CREAT | 0600);
    if (id < 0) { perror("shmget"); return 1; }

    struct s *p = shmat(id, NULL, 0);
    if (p == (void *)-1) { perror("shmat"); return 1; }
    p->ready = 0;

    pid_t pid = fork();
    if (pid == 0) {
        while (atomic_load(&p->ready) == 0) usleep(1000);
        printf("[child] %s\n", p->msg);
        shmdt(p);
        _exit(0);
    }
    strncpy(p->msg, "hello SysV shm", sizeof p->msg);
    atomic_store(&p->ready, 1);
    waitpid(pid, NULL, 0);

    shmdt(p);
    shmctl(id, IPC_RMID, NULL);
    return 0;
}
