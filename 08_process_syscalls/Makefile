CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Wextra

PROGS = 01_fork 02_fork_exec_wait 03_vfork 04_clone_raw 05_clone_thread \
        06_signal_handler 07_daemonise 08_pipe_fork 09_zombie_orphan 10_sigchld

all: $(PROGS)
%: %.c ; $(CC) $(CFLAGS) $< -o $@

run: all
	@for p in 01_fork 02_fork_exec_wait 03_vfork 04_clone_raw 05_clone_thread 08_pipe_fork 10_sigchld; do \
	   echo "=== $$p ==="; ./$$p; done

clean: ; rm -f $(PROGS)
.PHONY: all run clean
