TOOLPATH = ./tolset/z_tools/
INCPATH  = ./tolset/z_tools/haribote/
OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj int.obj fifo.obj
BINPATH = ./bin/


# MAKE     = $(TOOLPATH)make.exe -r
MAKE       = make -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
MAKEFONT = $(TOOLPATH)makefont.exe
COPY     = cmd.exe /C copy
DEL      = rm





# 默认动作

default :
	# $(MAKE) clean
	$(MAKE) run
	# $(MAKE) clean

# 镜像文件生成

ipl10.bin : ipl10.asm Makefile
	$(NASK) ipl10.asm ipl10.bin ipl10.lst

asmhead.bin : asmhead.asm Makefile
	$(NASK) asmhead.asm asmhead.bin asmhead.lst




bootpack.obj : bootpack.asm Makefile
	$(NASK) bootpack.asm bootpack.obj bootpack.lst

naskfunc.obj : naskfunc.asm Makefile
	$(NASK) naskfunc.asm naskfunc.obj naskfunc.lst

hankaku.bin : ./font/hankaku.txt Makefile
	$(MAKEFONT) ./font/hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)
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
%.gas: %.c Makefile
	$(CC1) -o $*.gas $*.c

%.asm: %.gas Makefile
	$(GAS2NASK) $*.gas $*.asm

%.obj: %.asm Makefile
	$(NASK) $*.asm $*.obj $*.lst

img :
	$(MAKE) haribote.img

mv_all:
	-mv *.bin $(BINPATH)
	-mv *.lst $(BINPATH)
	-mv *.gas $(BINPATH)
	-mv *.obj $(BINPATH)
	-mv *.img $(BINPATH)
	-mv bootpack.asm $(BINPATH) 
	-mv bootpack.map $(BINPATH)
	-mv bootpack.bim $(BINPATH)
	-mv bootpack.hrb $(BINPATH)
	-mv haribote.sys $(BINPATH)


run :
	$(MAKE) img
	$(COPY) haribote.img .\tolset\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ./tolset/z_tools/qemu
	# $(MAKE) mv_all

install :
	$(MAKE) img
	$(IMGTOL) w a: haribote.img

clean :
	-$(DEL) *.bin
	-$(DEL) *.lst
	-$(DEL) *.gas
	-$(DEL) *.obj
	-$(DEL) *.img
	-$(DEL) bootpack.asm
	-$(DEL) bootpack.map
	-$(DEL) bootpack.bim
	-$(DEL) bootpack.hrb
	-$(DEL) haribote.sys
	

src_only :
	$(MAKE) clean
	-$(DEL) haribote.img