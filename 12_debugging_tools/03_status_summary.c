/*
 * 03_status_summary.c
 * --------------------
 * Print the Vm* fields from /proc/self/status with explanations.
 */
#include <stdio.h>
#include <string.h>

struct entry { const char *key; const char *desc; };
static struct entry e[] = {
    { "VmPeak:",  "peak VSZ since exec"          },
    { "VmSize:",  "current VSZ (sum of VMAs)"    },
    { "VmLck:",   "mlocked"                      },
    { "VmHWM:",   "peak RSS since exec"          },
    { "VmRSS:",   "current resident set"         },
    { "RssAnon:", "anon resident"                },
    { "RssFile:", "file-backed resident"         },
    { "RssShmem:","shmem resident"               },
    { "VmData:",  "data segments (heap + bss)"   },
    { "VmStk:",   "main thread stack"            },
    { "VmExe:",   "text"                         },
    { "VmLib:",   "shared libraries (text only)" },
    { "VmPTE:",   "page tables themselves"       },
    { "VmSwap:",  "in swap"                      },
    { "Threads:", "thread count"                 },
};

int main(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    char line[256];
    while (fgets(line, sizeof line, f)) {
        for (size_t i = 0; i < sizeof e / sizeof *e; ++i) {
            if (!strncmp(line, e[i].key, strlen(e[i].key))) {
                line[strcspn(line, "\n")] = 0;
                printf("  %-40s  -- %s\n", line, e[i].desc);
            }
        }
    }
    fclose(f);
    return 0;
}
