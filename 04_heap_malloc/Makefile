CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_malloc_basics 02_chunk_layout 03_brk_vs_mmap 04_mallopt_tuning \
        05_mallinfo 06_alignment 07_calloc_realloc 08_arenas_threads \
        09_tcache_demo 10_double_free 11_use_after_free 12_heap_overflow 13_malloc_trim

all: $(PROGS)

08_arenas_threads:    08_arenas_threads.c   ; $(CC) $(CFLAGS) $< -o $@ -pthread
%: %.c
	$(CC) $(CFLAGS) $< -o $@

run: all
	@for p in 01_malloc_basics 02_chunk_layout 03_brk_vs_mmap 04_mallopt_tuning \
	          05_mallinfo 06_alignment 07_calloc_realloc 09_tcache_demo 13_malloc_trim; do \
	   echo "=== $$p ==="; ./$$p; done

clean:
	rm -f $(PROGS)

.PHONY: all run clean
