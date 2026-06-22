CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_program_break 02_brk_directly

all: $(PROGS)
%: %.c ; $(CC) $(CFLAGS) $< -o $@
run: all
	@for p in $(PROGS); do echo "=== $$p ==="; ./$$p; done
clean: ; rm -f $(PROGS)
.PHONY: all run clean
