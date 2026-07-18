/*
 * 03_alloca_vs_vla.c
 * -------------------
 * Compare three ways to grab N bytes inside a function:
 *
 *     malloc(N)  -- heap; survives function return; you must free()
 *     alloca(N)  -- stack; freed at function return; non-standard (BSD)
 *     char[N]    -- VLA;   stack; C99+; freed at end of enclosing block
 *
 *   stack frame after a VLA / alloca:
 *
 *   +--------------------------+   <- %rbp
 *   | saved rbp / canary / ... |
 *   +--------------------------+
 *   | other locals             |
 *   +--------------------------+
 *   | dynamic block (n bytes)  |   <-- VLA / alloca lives here
 *   +--------------------------+   <- %rsp grows down as n grows
 *
 *   NB: large n => may overflow the stack.  Always validate n!
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

static void use_malloc(size_t n)
{
    char *p = malloc(n);
    memset(p, 'M', n);
    printf("  malloc  -> %p (heap)\n", (void *)p);
    free(p);
}

static void use_alloca(size_t n)
{
    char *p = alloca(n);
    memset(p, 'A', n);
    printf("  alloca  -> %p (stack)\n", (void *)p);
}

static void use_vla(size_t n)
{
    char p[n];                  /* C99 VLA */
    memset(p, 'V', n);
    printf("  VLA     -> %p (stack)\n", (void *)p);
}

int main(void)
{
    size_t n = 1024;
    int stack_anchor;
    printf("&stack_anchor = %p (reference)\n", (void *)&stack_anchor);
    use_malloc(n);
    use_alloca(n);
    use_vla(n);
    return 0;
}

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 03_stack/03_alloca_vs_vla.c
 * Command: make -C 03_stack 03_alloca_vs_vla
 * Exit status: 0
 * Output:
 * &stack_anchor = 0x7ffd7ab6601c (reference)
 *   malloc  -> 0x563d91eeb2b0 (heap)
 *   alloca  -> 0x7ffd7ab65bd0 (stack)
 *   VLA     -> 0x7ffd7ab65bc0 (stack)
 * AUTO-GENERATED RUN OUTPUT END
 */
