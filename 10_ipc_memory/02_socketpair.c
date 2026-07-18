/*
 * 02_socketpair.c
 * ----------------
 * Anonymous, bidirectional, in-kernel socket pair.
 *
 *   socketpair(AF_UNIX, SOCK_STREAM, 0, sv)
 *     sv[0] <-> sv[1], full duplex
 *
 *   No filesystem name; pass to child via fork() (inherit) or via
 *   SCM_RIGHTS on another already-shared socket.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>

int main(void)
{
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) { perror("socketpair"); return 1; }

    pid_t pid = fork();
    if (pid == 0) {
        close(sv[0]);
        char buf[64];
        ssize_t n = read(sv[1], buf, sizeof buf);
        printf("[child] got %.*s", (int)n, buf);
        write(sv[1], "pong\n", 5);
        close(sv[1]);
        _exit(0);
    }
    close(sv[1]);
    write(sv[0], "ping\n", 5);
    char buf[64]; ssize_t n = read(sv[0], buf, sizeof buf);
    printf("[parent] got %.*s", (int)n, buf);
    close(sv[0]);
    waitpid(pid, NULL, 0);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 10_ipc_memory/02_socketpair.c
 * Command: make -C 10_ipc_memory 02_socketpair
 * Exit status: 0
 * Output:
 * [parent] got pong
 * AUTO-GENERATED RUN OUTPUT END
 */
