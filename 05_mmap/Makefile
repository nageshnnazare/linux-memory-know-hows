CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_anon_mmap 02_file_mmap 03_shared_mmap 04_file_share_write \
        05_mprotect_guard 06_madvise_dontneed 07_mremap 08_map_populate \
        09_map_fixed_noreplace

all: $(PROGS)

%: %.c ; $(CC) $(CFLAGS) $< -o $@

run: all
	@for p in 01_anon_mmap 02_file_mmap 04_file_share_write 06_madvise_dontneed 07_mremap 08_map_populate 09_map_fixed_noreplace; do \
	   echo "=== $$p ==="; ./$$p; done

clean: ; rm -f $(PROGS)

.PHONY: all run clean
