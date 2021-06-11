.PHONY: clean all

all:
	$(MAKE) -C user/ all VERBOSE=$(VERBOSE)
	$(MAKE) -C kernel/ kernel.bin VERBOSE=$(VERBOSE)

run-now:
	qemu-system-i386 -m 256M -kernel kernel/kernel.bin

run:
	qemu-system-i386 -s -S -m 256M -kernel kernel/kernel.bin

debug:
	$(MAKE) run &
	emacs --eval '(progn (gdb "gdb -i=mi kernel/kernel.bin") (insert "target remote :1234") (gdb-many-windows))'

clean:
	$(MAKE) clean -C kernel/
	$(MAKE) clean -C user/

