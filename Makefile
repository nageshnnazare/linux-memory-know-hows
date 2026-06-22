# Top-level Makefile - builds every section.
# Each subdirectory has its own self-contained Makefile.

SUBDIRS := 00_fundamentals \
           01_virtual_memory \
           02_process_address_space \
           03_stack \
           04_heap_malloc \
           05_mmap \
           06_brk_sbrk \
           07_paging_swapping \
           08_process_syscalls \
           09_threads_memory \
           10_ipc_memory \
           11_memory_protection \
           12_debugging_tools \
           13_advanced \
           14_putting_it_together

.PHONY: all clean run $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	@echo "=== $@ ==="
	@$(MAKE) -C $@ --no-print-directory

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean --no-print-directory; done

run:
	@for d in $(SUBDIRS); do echo "=== run $$d ==="; $(MAKE) -C $$d run --no-print-directory; done
