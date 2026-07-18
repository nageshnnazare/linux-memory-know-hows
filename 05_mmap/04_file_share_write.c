/*
 * 04_file_share_write.c
 * ----------------------
 * Open a file, map it MAP_SHARED, write through the pointer, msync().
 * Then verify that another reader sees the change on disk.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

int main(void)
{
    const char *path = "/tmp/mmap_shared.txt";
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    write(fd, "AAAAAAAAAAAAAAAA", 16);
    struct stat st; fstat(fd, &st);

    char *p = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) { perror("mmap"); close(fd); return 1; }
    close(fd);

    printf("before: ");
    for (int i = 0; i < st.st_size; ++i) putchar(p[i]); putchar('\n');

    /* Write through the mapping; dirties the page */
    memcpy(p, "BBBBBBBB", 8);

    /* Flush to disk immediately (otherwise the writeback daemon will) */
    msync(p, st.st_size, MS_SYNC);

    /* Reopen and read back */
    FILE *f = fopen(path, "r");
    char buf[64];
    size_t n = fread(buf, 1, sizeof buf, f); fclose(f);
    printf("on disk : %.*s\n", (int)n, buf);

    munmap(p, st.st_size);
    unlink(path);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 05_mmap/04_file_share_write.c
 * Command: make -C 05_mmap 04_file_share_write
 * Exit status: 0
 * Output:
 * before: AAAAAAAAAAAAAAAA
 * on disk : BBBBBBBBAAAAAAAA
 * AUTO-GENERATED RUN OUTPUT END
 */
