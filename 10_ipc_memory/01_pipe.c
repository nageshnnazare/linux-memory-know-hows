/*
 * 01_pipe.c
 * ----------
 * Basic anonymous pipe + fork; parent sends a few records.
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2]; if (pipe(fd) < 0) return 1;
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[1]);
        char buf[64]; ssize_t n;
        while ((n = read(fd[0], buf, sizeof buf)) > 0)
            write(1, buf, n);
        _exit(0);
    }
    close(fd[0]);
    for (int i = 0; i < 3; ++i) {
        char msg[64];
        int len = snprintf(msg, sizeof msg, "record %d\n", i);
        write(fd[1], msg, len);
        usleep(50000);
    }
    close(fd[1]);
    waitpid(pid, NULL, 0);
    return 0;
}
