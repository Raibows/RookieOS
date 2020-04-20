img.bin: ipl.asm Makefile
	nasm ipl.asm -o img.bin

run: img.bin Makefile
	qemu-system-x86_64 img.bin

clean:
	rm *.bin
