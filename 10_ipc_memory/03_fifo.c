/*
 * 03_fifo.c
 * ----------
 * Named pipe (FIFO).  Works between UNRELATED processes (no fork).
 *
 *   mkfifo("/tmp/myfifo", 0600)   -> a special inode in the filesystem
 *   open(... O_RDONLY)            blocks until a writer opens the other end
 *   open(... O_WRONLY)            same, for the reader
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

int main(void)
{
    const char *path = "/tmp/mem_guide_fifo";
    unlink(path);
    if (mkfifo(path, 0600) < 0) { perror("mkfifo"); return 1; }

    pid_t pid = fork();
    if (pid == 0) {
        int fd = open(path, O_RDONLY);
        char buf[64]; int n = read(fd, buf, sizeof buf);
        write(1, buf, n);
        close(fd);
        _exit(0);
    }
    int fd = open(path, O_WRONLY);
    write(fd, "hello fifo\n", 11);
    close(fd);
    waitpid(pid, NULL, 0);
    unlink(path);
    return 0;
}
