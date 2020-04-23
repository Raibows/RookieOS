ipl.bin: ipl.asm Makefile
	nasm -f bin ipl.asm -o ipl.bin -l ipl.lst

os.bin: os.asm Makefile
	nasm -f bin os.asm -o os.bin

run: os.img Makefile
	# qemu-system-x86_64 -fda format=raw,file=./os.img
	qemu-system-x86_64 -fda os.img -boot a

clean:
	rm *.bin
	rm *.img
	rm *.lst

os.img: os.bin ipl.bin Makefile
	mv ipl.bin os.img
	dd if=os.bin of=os.img bs=512 seek=33 count=1 conv=notrunc

qemu: $(file)
	nasm $(file) -o $(file).bin
	qemu-system-x86_64 -drive format=raw,file=$(file).bin
