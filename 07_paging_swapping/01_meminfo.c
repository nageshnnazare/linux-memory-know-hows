/*
 * 01_meminfo.c
 * -------------
 * Read /proc/meminfo and print only the interesting lines, with brief
 * explanations.
 */
#include <stdio.h>
#include <string.h>

struct e { const char *prefix; const char *desc; };

static struct e wanted[] = {
    { "MemTotal:",      "total usable RAM"                              },
    { "MemFree:",       "untouched RAM (on buddy free lists)"           },
    { "MemAvailable:",  "what userspace could allocate without swap"    },
    { "Buffers:",       "page cache for raw block devices"              },
    { "Cached:",        "page cache for files (mmap + read)"            },
    { "SwapTotal:",     "size of swap"                                  },
    { "SwapFree:",      "unused swap"                                   },
    { "Dirty:",         "dirty pages (not yet written back)"            },
    { "Writeback:",     "pages being written back now"                  },
    { "AnonPages:",     "non-file backed (heap/stack/anon mmap)"        },
    { "Mapped:",        "mapped via mmap"                               },
    { "Shmem:",         "tmpfs + shared anon"                           },
    { "KReclaimable:",  "slab/etc that could be reclaimed"              },
    { "Slab:",          "kernel slab (kmalloc internals)"               },
    { "PageTables:",    "memory used by page tables themselves"         },
    { "CommitLimit:",   "max committable bytes (overcommit_memory=2)"   },
    { "Committed_AS:",  "current committed memory"                      },
    { "HugePages_Total:", "huge pages reserved"                         },
    { "HugePages_Free:",  "huge pages free"                             },
    { "Hugepagesize:",  "huge page size"                                },
};

int main(void)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 1;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        for (size_t i = 0; i < sizeof wanted / sizeof *wanted; ++i) {
            size_t L = strlen(wanted[i].prefix);
            if (!strncmp(line, wanted[i].prefix, L)) {
                line[strcspn(line, "\n")] = 0;
                printf("  %-50s  -- %s\n", line, wanted[i].desc);
            }
        }
    }
    fclose(f);
    return 0;
}
