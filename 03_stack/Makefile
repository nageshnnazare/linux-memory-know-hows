CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_stack_growth 02_recursion_depth 03_alloca_vs_vla 04_stack_overflow \
        05_dangling_local 06_stack_canary 07_pthread_stack

all: $(PROGS)

01_stack_growth:      01_stack_growth.c   ; $(CC) $(CFLAGS) $< -o $@
02_recursion_depth:   02_recursion_depth.c; $(CC) $(CFLAGS) $< -o $@
03_alloca_vs_vla:     03_alloca_vs_vla.c  ; $(CC) $(CFLAGS) $< -o $@
04_stack_overflow:    04_stack_overflow.c ; $(CC) $(CFLAGS) $< -o $@
05_dangling_local:    05_dangling_local.c ; $(CC) -O0 -g -Wno-return-local-addr $< -o $@
06_stack_canary:      06_stack_canary.c   ; $(CC) -O0 -g -fstack-protector-strong -Wno-stringop-overflow -Wno-stringop-overread $< -o $@
07_pthread_stack:     07_pthread_stack.c  ; $(CC) $(CFLAGS) $< -o $@ -pthread

run: all
	@for p in 01_stack_growth 02_recursion_depth 03_alloca_vs_vla 07_pthread_stack; do \
	   echo "=== $$p ==="; ./$$p; done

clean:
	rm -f $(PROGS)

.PHONY: all run clean
