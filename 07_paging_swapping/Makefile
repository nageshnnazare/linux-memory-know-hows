CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_meminfo 02_mlock 03_madvise_pageout 04_oom_score 05_overcommit

all: $(PROGS)
%: %.c ; $(CC) $(CFLAGS) $< -o $@
run: all
	@for p in 01_meminfo 02_mlock 04_oom_score 05_overcommit; do echo "=== $$p ==="; ./$$p; done
clean: ; rm -f $(PROGS)
.PHONY: all run clean
