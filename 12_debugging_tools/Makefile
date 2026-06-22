CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_proc_maps_dump 02_proc_smaps 03_status_summary 04_getrusage_demo 05_mallinfo_dump

all: $(PROGS)
%: %.c ; $(CC) $(CFLAGS) $< -o $@

run: all
	@for p in $(PROGS); do echo "=== $$p ==="; ./$$p; done

clean: ; rm -f $(PROGS)
.PHONY: all run clean
