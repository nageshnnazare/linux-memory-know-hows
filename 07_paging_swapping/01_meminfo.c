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

/*
 * AUTO-GENERATED RUN OUTPUT START
 * Source: 07_paging_swapping/01_meminfo.c
 * Command: make -C 07_paging_swapping 01_meminfo
 * Exit status: 0
 * Output:
 *   MemTotal:       16373456 kB                         -- total usable RAM
 *   MemFree:        13990028 kB                         -- untouched RAM (on buddy free lists)
 *   MemAvailable:   15282792 kB                         -- what userspace could allocate without swap
 *   Buffers:           72208 kB                         -- page cache for raw block devices
 *   Cached:          1496904 kB                         -- page cache for files (mmap + read)
 *   SwapTotal:       3145724 kB                         -- size of swap
 *   SwapFree:        3145724 kB                         -- unused swap
 *   Dirty:             10104 kB                         -- dirty pages (not yet written back)
 *   Writeback:             0 kB                         -- pages being written back now
 *   AnonPages:        308816 kB                         -- non-file backed (heap/stack/anon mmap)
 *   Mapped:           316508 kB                         -- mapped via mmap
 *   Shmem:             43980 kB                         -- tmpfs + shared anon
 *   KReclaimable:      75440 kB                         -- slab/etc that could be reclaimed
 *   Slab:             150360 kB                         -- kernel slab (kmalloc internals)
 *   PageTables:         6192 kB                         -- memory used by page tables themselves
 *   CommitLimit:    11332452 kB                         -- max committable bytes (overcommit_memory=2)
 *   Committed_AS:    1986172 kB                         -- current committed memory
 *   HugePages_Total:       0                            -- huge pages reserved
 *   HugePages_Free:        0                            -- huge pages free
 *   Hugepagesize:       2048 kB                         -- huge page size
 * AUTO-GENERATED RUN OUTPUT END
 */
