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

/*
 */

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 02_process_address_space/02_environ.c
 * Command: make -C 02_process_address_space 02_environ
 * Exit status: 0
 * Output:
 * === argc / argv ===
 * argc = 1
 *   argv[0] = 0x7ffcabf20c09 -> "./02_environ"
 *   argv[1] = NULL
 * 
 * === environ ===
 *   environ[0] = 0x7ffcabf20c16 -> "SHELL=/bin/bash"
 *   environ[1] = 0x7ffcabf20c26 -> "SELENIUM_JAR_PATH=/usr/share/java/selenium-server.jar"
 *   environ[2] = 0x7ffcabf20c5c -> "CONDA=/usr/share/miniconda"
 *   environ[3] = 0x7ffcabf20c77 -> "GITHUB_WORKSPACE=/home/runner/work/linux-memory-know-hows/linux-memory-know-hows"
 *   environ[4] = 0x7ffcabf20cc8 -> "JAVA_HOME_11_X64=/usr/lib/jvm/temurin-11-jdk-amd64"
 *   environ[5] = 0x7ffcabf20cfb -> "JAVA_HOME_25_X64=/usr/lib/jvm/temurin-25-jdk-amd64"
 *   environ[6] = 0x7ffcabf20d2e -> "GITHUB_PATH=/home/runner/work/_temp/_runner_file_commands/add_path_c2d5118c-040a..."
 *   environ[7] = 0x7ffcabf20d96 -> "GITHUB_ACTION=__run_2"
 *   environ[8] = 0x7ffcabf20dac -> "JAVA_HOME=/usr/lib/jvm/temurin-17-jdk-amd64"
 *   environ[9] = 0x7ffcabf20dd8 -> "GITHUB_RUN_NUMBER=4"
 *   ... 115 total entries, environ[115] = NULL
 * 
 * === auxv ===
 *   AT_SYSINFO_EHDR (vDSO) (33) = 0x7f2c47953000
 *   AT_?                 (51) = 0x6f0
 *   AT_HWCAP             (16) = 0x178bfbff
 *   AT_PAGESZ            (6) = 0x1000
 *   AT_CLKTCK            (17) = 0x64
 *   AT_PHDR              (3) = 0x55ea9a0cc040
 *   AT_PHENT             (4) = 0x38
 *   AT_PHNUM             (5) = 0xd
 *   AT_BASE              (7) = 0x7f2c47955000
 *   AT_FLAGS             (8) = 0x0
 *   AT_ENTRY             (9) = 0x55ea9a0cd0c0
 *   AT_UID               (11) = 0x3e9
 *   AT_EUID              (12) = 0x3e9
 *   AT_GID               (13) = 0x3e9
 *   AT_EGID              (14) = 0x3e9
 *   AT_SECURE            (23) = 0x0
 *   AT_RANDOM            (25) = 0x7ffcabf20aa9
 *   AT_?                 (26) = 0x2
 *   AT_EXECFN            (31) = 0x7ffcabf21feb
 *   AT_PLATFORM          (15) = 0x7ffcabf20ab9
 *   AT_?                 (27) = 0x1c
 *   AT_?                 (28) = 0x20
 * 
 * === via getauxval(3) (libc helper) ===
 *   AT_PAGESZ        = 4096
 *   AT_BASE   (ld)   = 7f2c47955000
 *   AT_SYSINFO_EHDR  = 7f2c47953000
 *   AT_EXECFN        = "./02_environ"
 * AUTO-GENERATED RUN OUTPUT END
 */
