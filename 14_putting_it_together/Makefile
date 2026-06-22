CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O2 -g -Wall -Wextra

PROGS = 01_bump 02_pool 03_slab 04_freelist 05_buddy

all: $(PROGS)
%: %.c ; $(CC) $(CFLAGS) $< -o $@

run: all
	@for p in $(PROGS); do echo "=== $$p ==="; ./$$p; done

clean: ; rm -f $(PROGS)
.PHONY: all run clean
