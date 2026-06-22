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
