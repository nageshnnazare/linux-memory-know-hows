CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O2 -g -Wall -Wextra
LDLIBS  ?=

PROGS = 01_memory_hierarchy 02_word_alignment 03_endianness 04_cacheline_falsesharing

all: $(PROGS)

01_memory_hierarchy:        01_memory_hierarchy.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

02_word_alignment:          02_word_alignment.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

03_endianness:              03_endianness.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

04_cacheline_falsesharing:  04_cacheline_falsesharing.c
	$(CC) $(CFLAGS) $< -o $@ -pthread

run: all
	@for p in $(PROGS); do echo "=== $$p ==="; ./$$p; done

clean:
	rm -f $(PROGS)

.PHONY: all run clean
