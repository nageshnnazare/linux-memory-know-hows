/*
 * 05_ksm_demo.c
 * --------------
 * Allocate two large identical regions and mark them MADV_MERGEABLE so the
 * KSM daemon (if enabled) deduplicates them.
 *
 *   sudo sh -c 'echo 1 > /sys/kernel/mm/ksm/run'
 *   sudo cat /sys/kernel/mm/ksm/pages_shared      # before and after
 *
 * KSM scans periodically; you may have to wait a bit.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int main(void)
{
    size_t len = 64UL << 20;
    char *a = mmap(NULL, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    char *b = mmap(NULL, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset(a, 0xAA, len);
    memset(b, 0xAA, len);

    if (madvise(a, len, MADV_MERGEABLE) < 0) perror("madvise(MERGEABLE)");
    if (madvise(b, len, MADV_MERGEABLE) < 0) perror("madvise(MERGEABLE)");

    printf("two identical 64 MiB regions allocated and marked MERGEABLE\n");
    printf("watch:  watch -n2 cat /sys/kernel/mm/ksm/pages_shared\n");
    printf("sleeping 30s so KSM has a chance to dedupe...\n");
    sleep(30);

    munmap(a, len); munmap(b, len);
    return 0;
}
