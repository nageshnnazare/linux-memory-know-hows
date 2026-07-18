/*
 * 10_scm_rights.c
 * ----------------
 * Pass an open file descriptor across a UNIX socket using SCM_RIGHTS.
 *
 *   sender:                                    receiver:
 *     - open a file -> got fd X                  - has its own socketpair end
 *     - sendmsg(sock, msg with cmsg SCM_RIGHTS{X})
 *                                                - recvmsg returns: msg + cmsg
 *                                                - the kernel installed a NEW fd Y in the receiver
 *                                                - read/write Y == read/write X
 *
 * This is how memfd_create is shared between sandboxed processes, how
 * fork-server architectures hand out pty/tty fds, how snapd installs perms.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>

static int send_fd(int sock, int fd_to_send)
{
    char dummy = 'F';
    char cbuf[CMSG_SPACE(sizeof(int))];
    struct iovec iov = { .iov_base = &dummy, .iov_len = 1 };
    struct msghdr msg = {0};
    msg.msg_iov     = &iov;
    msg.msg_iovlen  = 1;
    msg.msg_control = cbuf;
    msg.msg_controllen = sizeof cbuf;
    struct cmsghdr *cm = CMSG_FIRSTHDR(&msg);
    cm->cmsg_level = SOL_SOCKET;
    cm->cmsg_type  = SCM_RIGHTS;
    cm->cmsg_len   = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cm), &fd_to_send, sizeof(int));
    return sendmsg(sock, &msg, 0);
}

static int recv_fd(int sock)
{
    char dummy;
    char cbuf[CMSG_SPACE(sizeof(int))];
    struct iovec iov = { .iov_base = &dummy, .iov_len = 1 };
    struct msghdr msg = {0};
    msg.msg_iov     = &iov;
    msg.msg_iovlen  = 1;
    msg.msg_control = cbuf;
    msg.msg_controllen = sizeof cbuf;
    if (recvmsg(sock, &msg, 0) <= 0) return -1;
    struct cmsghdr *cm = CMSG_FIRSTHDR(&msg);
    if (!cm || cm->cmsg_type != SCM_RIGHTS) return -1;
    int fd;
    memcpy(&fd, CMSG_DATA(cm), sizeof fd);
    return fd;
}

int main(void)
{
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);

    pid_t pid = fork();
    if (pid == 0) {
        close(sv[0]);
        int fd = recv_fd(sv[1]);
        printf("[child] received fd=%d\n", fd);
        char buf[64];
        lseek(fd, 0, SEEK_SET);
        int n = read(fd, buf, sizeof buf);
        write(1, buf, n);
        close(fd); close(sv[1]);
        _exit(0);
    }
    close(sv[1]);
    int fd = open("/etc/hostname", O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    printf("[parent] passing fd=%d (/etc/hostname) to child\n", fd);
    send_fd(sv[0], fd);
    close(fd); close(sv[0]);
    waitpid(pid, NULL, 0);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 10_ipc_memory/10_scm_rights.c
 * Command: make -C 10_ipc_memory 10_scm_rights
 * Exit status: 0
 * Output:
 * runnervm3jd5f
 * [parent] passing fd=4 (/etc/hostname) to child
 * AUTO-GENERATED RUN OUTPUT END
 */
