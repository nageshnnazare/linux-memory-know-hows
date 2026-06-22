CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_mprotect_jit 02_aslr_test 03_segfault_handler 04_canary_demo \
        05_fortify_demo 06_w_xor_x

all: $(PROGS)

04_canary_demo: 04_canary_demo.c ; $(CC) -O0 -g -fstack-protector-strong -Wno-stringop-overflow $< -o $@
05_fortify_demo: 05_fortify_demo.c ; $(CC) -O2 -D_FORTIFY_SOURCE=2 -Wno-stringop-overflow -Wno-stringop-overread $< -o $@
%: %.c ; $(CC) $(CFLAGS) $< -o $@

run: all
	@for p in 01_mprotect_jit 02_aslr_test; do echo "=== $$p ==="; ./$$p; done

clean: ; rm -f $(PROGS)
.PHONY: all run clean
