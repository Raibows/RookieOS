TOOLPATH = ./tolset/z_tools/
INCPATH  = ./tolset/z_tools/haribote/

# MAKE     = $(TOOLPATH)make.exe -r
MAKE       = make -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = cmd.exe /C copy
DEL      = rm





# 默认动作

default :
	$(MAKE) img

# 镜像文件生成

ipl10.bin : ipl10.asm Makefile
	$(NASK) ipl10.asm ipl10.bin ipl10.lst

asmhead.bin : asmhead.asm Makefile
	$(NASK) asmhead.asm asmhead.bin asmhead.lst

bootpack.gas : bootpack.c Makefile
	$(CC1) -o bootpack.gas bootpack.c

bootpack.asm : bootpack.gas Makefile
	$(GAS2NASK) bootpack.gas bootpack.asm

bootpack.obj : bootpack.asm Makefile
	$(NASK) bootpack.asm bootpack.obj bootpack.lst

naskfunc.obj : naskfunc.asm Makefile
	$(NASK) naskfunc.asm naskfunc.obj naskfunc.lst

bootpack.bim : bootpack.obj naskfunc.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		bootpack.obj naskfunc.obj
# 3MB+64KB=3136KB

bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

haribote.sys : asmhead.bin bootpack.hrb Makefile
	$(COPY) /B asmhead.bin+bootpack.hrb haribote.sys

haribote.img : ipl10.bin haribote.sys Makefile
	$(EDIMG)   imgin:./tolset/z_tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
		copy from:haribote.sys to:@: \
		imgout:haribote.img

# 其他指令

img :
	$(MAKE) haribote.img

run :
	$(MAKE) img
	$(COPY) haribote.img .\tolset\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ./tolset/z_tools/qemu

install :
	$(MAKE) img
	$(IMGTOL) w a: haribote.img

clean :
	-$(DEL) *.bin
	-$(DEL) *.lst
	-$(DEL) *.gas
	-$(DEL) *.obj
	-$(DEL) bootpack.asm
	-$(DEL) bootpack.map
	-$(DEL) bootpack.bim
	-$(DEL) bootpack.hrb
	-$(DEL) haribote.sys

src_only :
	$(MAKE) clean
	-$(DEL) haribote.img






# ipl.bin: ipl.asm Makefile
# 	nasm -f bin ipl.asm -o ipl.bin -l ipl.lst

# os.bin: os.asm Makefile
# 	nasm -f bin os.asm -o os.bin

# run: os.img Makefile
# 	# qemu-system-x86_64 -fda format=raw,file=./os.img
# 	qemu-system-x86_64 -fda os.img -boot a

# clean:
# 	rm *.bin
# 	rm *.img
# 	rm *.lst

# os.img: os.bin ipl.bin Makefile
# 	mv ipl.bin os.img
# 	dd if=os.bin of=os.img bs=512 seek=33 count=1 conv=notrunc

# qemu: $(file)
# 	nasm $(file) -o $(file).bin
# 	qemu-system-x86_64 -drive format=raw,file=$(file).bin
