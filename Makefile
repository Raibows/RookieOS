img.bin: ipl.asm Makefile
	nasm ipl.asm -o img.bin

run: img.bin Makefile
	qemu-system-x86_64 -drive format=raw,file=./img.bin

clean:
	rm *.bin

qemu: $(file)
	nasm $(file) -o $(file).bin
	qemu-system-x86_64 -drive format=raw,file=$(file).bin
