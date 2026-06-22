CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_pipe 02_socketpair 03_fifo 04_shm_open 05_sysv_shm 06_memfd_create \
        07_eventfd 08_signalfd 09_mq 10_scm_rights

all: $(PROGS)

04_shm_open:    04_shm_open.c   ; $(CC) $(CFLAGS) $< -o $@ -lrt
07_eventfd:     07_eventfd.c    ; $(CC) $(CFLAGS) $< -o $@ -pthread
09_mq:          09_mq.c         ; $(CC) $(CFLAGS) $< -o $@ -lrt

%: %.c ; $(CC) $(CFLAGS) $< -o $@

run: all
	@for p in 01_pipe 02_socketpair 03_fifo 04_shm_open 05_sysv_shm 06_memfd_create 07_eventfd 09_mq 10_scm_rights; do \
	   echo "=== $$p ==="; ./$$p; done

clean: ; rm -f $(PROGS)
.PHONY: all run clean
