CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_thp_madvise 02_hugetlb_mmap 03_userfaultfd 04_numa_inspect 05_ksm_demo

all: $(PROGS)

03_userfaultfd: 03_userfaultfd.c ; $(CC) $(CFLAGS) $< -o $@ -pthread
%: %.c ; $(CC) $(CFLAGS) $< -o $@

run: all
	@for p in 04_numa_inspect; do echo "=== $$p ==="; ./$$p; done

clean: ; rm -f $(PROGS)
.PHONY: all run clean
