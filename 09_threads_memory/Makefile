CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O2 -g -Wall -Wextra -pthread

PROGS = 01_pthread_create 02_tls_compiler 03_tls_pthread_key 04_pthread_join \
        05_futex_basic 06_mutex_vs_atomic

all: $(PROGS)
%: %.c ; $(CC) $(CFLAGS) $< -o $@

run: all
	@for p in $(PROGS); do echo "=== $$p ==="; ./$$p; done

clean: ; rm -f $(PROGS)
.PHONY: all run clean
