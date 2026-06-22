/*
 * 02_environ.c
 * -------------
 * Show how argv, envp, and the auxiliary vector are laid out at the top
 * of the stack at program start.
 *
 *   higher addr (top of stack)
 *   +--------------------------+
 *   | env strings (NUL-sep)    |
 *   | argv strings (NUL-sep)   |
 *   | random data (AT_RANDOM)  |
 *   | (alignment)              |
 *   +--------------------------+
 *   | NULL                     |
 *   | auxv entries (8B key + 8B val), terminated by AT_NULL
 *   +--------------------------+
 *   | NULL                     |
 *   | envp[N-1] -> "PATH=..."  |
 *   | ...                      |
 *   | envp[0]   -> "HOME=..."  |
 *   +--------------------------+
 *   | NULL                     |
 *   | argv[argc-1] -> ...      |
 *   | ...                      |
 *   | argv[0]    -> "./prog"   |
 *   +--------------------------+
 *   | argc (a long)            |
 *   +--------------------------+   <- %rsp at _start
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/auxv.h>
#include <elf.h>
#include <string.h>

extern char **environ;

static const char *auxv_name(unsigned long type)
{
    switch (type) {
    case AT_NULL:      return "AT_NULL";
    case AT_PHDR:      return "AT_PHDR";
    case AT_PHENT:     return "AT_PHENT";
    case AT_PHNUM:     return "AT_PHNUM";
    case AT_PAGESZ:    return "AT_PAGESZ";
    case AT_BASE:      return "AT_BASE";       /* ld.so load address */
    case AT_FLAGS:     return "AT_FLAGS";
    case AT_ENTRY:     return "AT_ENTRY";      /* program entry point */
    case AT_UID:       return "AT_UID";
    case AT_EUID:      return "AT_EUID";
    case AT_GID:       return "AT_GID";
    case AT_EGID:      return "AT_EGID";
    case AT_PLATFORM:  return "AT_PLATFORM";
    case AT_HWCAP:     return "AT_HWCAP";
    case AT_CLKTCK:    return "AT_CLKTCK";
    case AT_SECURE:    return "AT_SECURE";
    case AT_RANDOM:    return "AT_RANDOM";
    case AT_EXECFN:    return "AT_EXECFN";
    case AT_SYSINFO_EHDR: return "AT_SYSINFO_EHDR (vDSO)";
    default:           return "AT_?";
    }
}

int main(int argc, char **argv)
{
    printf("=== argc / argv ===\n");
    printf("argc = %d\n", argc);
    for (int i = 0; i < argc; ++i)
        printf("  argv[%d] = %p -> \"%s\"\n", i, (void *)argv[i], argv[i]);
    printf("  argv[%d] = NULL\n", argc);

    printf("\n=== environ ===\n");
    int n = 0;
    for (char **e = environ; *e; ++e, ++n) {
        if (n < 10) /* don't drown user */
            printf("  environ[%d] = %p -> \"%.80s%s\"\n",
                   n, (void *)*e, *e, strlen(*e) > 80 ? "..." : "");
    }
    printf("  ... %d total entries, environ[%d] = NULL\n", n, n);

    /* Walk auxv by finding the NULL terminator after environ */
    char **after_env = environ;
    while (*after_env) ++after_env;
    ++after_env; /* skip the NULL */
    Elf64_auxv_t *auxv = (Elf64_auxv_t *)after_env;

    printf("\n=== auxv ===\n");
    for (; auxv->a_type != AT_NULL; ++auxv) {
        printf("  %-20s (%lu) = 0x%lx\n",
               auxv_name(auxv->a_type), auxv->a_type,
               auxv->a_un.a_val);
    }

    printf("\n=== via getauxval(3) (libc helper) ===\n");
    printf("  AT_PAGESZ        = %lu\n",  getauxval(AT_PAGESZ));
    printf("  AT_BASE   (ld)   = %lx\n",  getauxval(AT_BASE));
    printf("  AT_SYSINFO_EHDR  = %lx\n",  getauxval(AT_SYSINFO_EHDR));
    printf("  AT_EXECFN        = \"%s\"\n", (const char *)getauxval(AT_EXECFN));
    return 0;
}
