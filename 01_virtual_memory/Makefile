CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_pagesize 02_lazy_allocation 03_count_page_faults 04_pagemap

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

run: all
	@for p in $(PROGS); do echo "=== $$p ==="; ./$$p; done

clean:
	rm -f $(PROGS)

.PHONY: all run clean
