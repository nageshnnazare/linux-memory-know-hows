/*
 * 02_file_mmap.c
 * ---------------
 * Read a file by pointer.  We create a small file, map it, hexdump its
 * contents, and munmap.
 *
 *   disk file
 *   +-----+-----+-----+-----+
 *   | 'h' | 'e' | 'l' | ... |
 *   +-----+-----+-----+-----+
 *   |     ^                ^
 *   |     | mmap MAP_PRIVATE
 *   |     v
 *   process VA
 *   +----------------------+
 *   |  p[0] p[1] p[2] ...  |
 *   +----------------------+
 *
 * MAP_PRIVATE -> writes to p[i] are CoW, never propagate to disk.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main(void)
{
    const char *path = "/tmp/mmapdemo.txt";
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return 1; }
    const char *msg = "hello mmap'd file world!\n";
    write(fd, msg, strlen(msg));

    struct stat st; fstat(fd, &st);
    char *p = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE, fd, 0);
    close(fd);  /* mapping holds its own reference */
    if (p == MAP_FAILED) { perror("mmap"); return 1; }

    printf("file size = %ld bytes  mapped @ %p\n", (long)st.st_size, p);
    fwrite(p, 1, st.st_size, stdout);

    /* Because MAP_PRIVATE, this write goes to a CoW page, NOT to disk. */
    p[0] = 'H';
    printf("after p[0]='H' in memory: \"%.*s\"\n", (int)st.st_size, p);

    /* Reopen the file via stdio to see it's unchanged on disk */
    FILE *f = fopen(path, "r"); char buf[64];
    size_t n = fread(buf, 1, sizeof buf, f); buf[n] = 0; fclose(f);
    printf("on disk still: \"%s\"\n", buf);

    munmap(p, st.st_size);
    unlink(path);
    return 0;
}

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 05_mmap/02_file_mmap.c
 * Command: make -C 05_mmap 02_file_mmap
 * Exit status: 0
 * Output:
 * file size = 25 bytes  mapped @ 0x7fbece2ce000
 * hello mmap'd file world!
 * after p[0]='H' in memory: "Hello mmap'd file world!
 * "
 * on disk still: "hello mmap'd file world!
 * "
 * AUTO-GENERATED RUN OUTPUT END
 */
