/*
 * 01_stack_growth.c
 * ------------------
 * Print &local at increasing recursion depths.  On x86_64 the value
 * decreases with depth (stack grows DOWN).
 */
#include <stdio.h>
#include <stdint.h>

static void recurse(int depth, uintptr_t prev_addr)
{
    int local;
    long delta = prev_addr ? (long)prev_addr - (long)&local : 0;
    printf("depth=%2d  &local=%p  (delta=%ld bytes from caller)\n",
           depth, (void *)&local, delta);
    if (depth < 8)
        recurse(depth + 1, (uintptr_t)&local);
}

int main(void)
{
    int top;
    printf("main &top = %p (top of our journey)\n", (void *)&top);
    recurse(0, (uintptr_t)&top);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 03_stack/01_stack_growth.c
 * Command: make -C 03_stack 01_stack_growth
 * Exit status: 0
 * Output:
 * main &top = 0x7ffd16a5b924 (top of our journey)
 * depth= 0  &local=0x7ffd16a5b8fc  (delta=40 bytes from caller)
 * depth= 1  &local=0x7ffd16a5b8bc  (delta=64 bytes from caller)
 * depth= 2  &local=0x7ffd16a5b87c  (delta=64 bytes from caller)
 * depth= 3  &local=0x7ffd16a5b83c  (delta=64 bytes from caller)
 * depth= 4  &local=0x7ffd16a5b7fc  (delta=64 bytes from caller)
 * depth= 5  &local=0x7ffd16a5b7bc  (delta=64 bytes from caller)
 * depth= 6  &local=0x7ffd16a5b77c  (delta=64 bytes from caller)
 * depth= 7  &local=0x7ffd16a5b73c  (delta=64 bytes from caller)
 * depth= 8  &local=0x7ffd16a5b6fc  (delta=64 bytes from caller)
 * AUTO-GENERATED RUN OUTPUT END
 */
