/*
 * 08_pipe_fork.c
 * ---------------
 * Classic pipe-then-fork: parent writes, child reads.
 *
 *   pipe(fds):
 *     fds[0] = read end
 *     fds[1] = write end
 *
 *   parent: closes [0], writes to [1], closes [1]    -> EOF in child
 *   child : closes [1], reads from [0] until EOF
 *
 *   Pipes have a small in-kernel circular buffer (default 64 KiB on Linux).
 *   write() blocks (or returns EAGAIN if O_NONBLOCK) when full.
 *   read()  returns 0 (EOF) once all write ends are closed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(void)
{
    int fd[2];
    if (pipe(fd) < 0) { perror("pipe"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        close(fd[1]);
        char buf[64];
        ssize_t n;
        while ((n = read(fd[0], buf, sizeof buf)) > 0)
            write(1, buf, n);
        close(fd[0]);
        _exit(0);
    }
    close(fd[0]);
    const char *msgs[] = { "hello\n", "from\n", "parent\n" };
    for (int i = 0; i < 3; ++i) {
        write(fd[1], msgs[i], strlen(msgs[i]));
        usleep(100000);
    }
    close(fd[1]);   /* tells child: EOF */
    waitpid(pid, NULL, 0);
    return 0;
}
