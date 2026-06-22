CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_segments_addresses 02_environ 03_dump_maps 04_aslr 05_rlimit

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

run: all
	@for p in $(PROGS); do echo "=== $$p ==="; ./$$p; done

clean:
	rm -f $(PROGS)

.PHONY: all run clean
