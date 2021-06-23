.PHONY: clean all

QEMU=qemu-system-i386 -m 256M -kernel kernel/kernel.bin -soundhw pcspk
QEMU_FLOPPY=-blockdev driver=file,node-name=f0,filename=floppy/floppy.img -device floppy,drive=f0

build:
	$(MAKE) -C user/ all VERBOSE=$(VERBOSE)
	$(MAKE) -C kernel/ kernel.bin VERBOSE=$(VERBOSE)

all: build
	$(MAKE) -i -C floppy/ floppy.img VERBOSE=$(VERBOSE)

run-now:
	$(QEMU) $(QEMU_FLOPPY)

run:
	$(QEMU) -s -S $(QEMU_FLOPPY)

run-raw:
	$(QEMU)

now: all run-now
raw: all run-raw

debug:
	$(MAKE) run &
	emacs --eval '(progn (gdb "gdb -i=mi kernel/kernel.bin") (insert "target remote :1234") (gdb-many-windows))'

clean:
	$(MAKE) clean -C kernel/
	$(MAKE) clean -C floppy/
	$(MAKE) clean -C user/

