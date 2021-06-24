.PHONY: clean all

floppy.img:
	mkfs.msdos -n "FAT-FLOP" -F 16 -C floppy.img 1440
	# mount without root
	udisksctl loop-setup -f floppy.img
	cp imperial.mbp /media/$(USER)/FAT-FLOP
	echo "Hello world" > /media/$(USER)/FAT-FLOP/hello.txt
	echo "Good bye" > /media/$(USER)/FAT-FLOP/bye.txt
	mkdir /media/$(USER)/FAT-FLOP/subdir
	echo "Buenos dias" > /media/$(USER)/FAT-FLOP/subdir/hola.txt
	ls -lR /media/$(USER)/FAT-FLOP/
	udisksctl loop-delete -b /dev/loop2

clean:
	rm -rf floppy.img

