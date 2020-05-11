#include "bootpack.h"

void init_gdt_idt(void) {
	// init global (segment) description table 8bytes
	// init interrupt description table
    // 可以用“调色板”的方式事先定义好8192个段

	// from 0x270000 to 0x27ffff = 65536bytes = 8bytes * 8192
	struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*) 0x00270000;
	// from 0x26f800 to 0x26fff = 2048bytes = 8bytes * 256
	struct GATE_DESCRIPTOR* idt = (struct GATE_DESCRIPTOR*) 0x0026f800;
	int i;
	// 段寄存器只用高13位，低3位不用，0 - 8191
	for (i=0; i < 8192; ++i)
	{
		set_segmdesc(gdt+i, 0, 0, 0);
	}
	set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092); //段号为1的段，4GB，
	set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a); //段号为2的段存储bootpack.hrb, 512KB
	load_gdtr(0xffff, 0x00270000);
	// 中断有256种
	for (i=0; i<256; ++i)
	{
		set_gatedesc(idt+i, 0, 0, 0);
	}
	load_idtr(0x7ff, 0x0026f800);
	
	/* IDT设置*/
	// 2<<3代表使用第二个段，左移3位是因为低三位必须是0
    set_gatedesc(idt + 0x20, (int)asm_int_handler20, 2<<3, AR_INTGATE32);
	set_gatedesc(idt + 0x21, (int)asm_int_handler21, 2<<3, AR_INTGATE32);
	set_gatedesc(idt + 0x27, (int)asm_int_handler27, 2<<3, AR_INTGATE32);
	set_gatedesc(idt + 0x2c, (int)asm_int_handler2c, 2<<3, AR_INTGATE32);

	return;
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR* sd, unsigned int limit, int base, int ar) {
	/*
	limit指段的大小，即段的字节数-1
	base代表基址
	ar=access right
	*/
	if (limit > 0xfffff) 
	{
		ar |= 0x8000; /* G_bit = 1 */
		limit /= 0x1000;
	}
	sd->limit_low = limit & 0xffff;
	sd->base_low = base & 0xffff;
	sd->base_mid = (base >> 16) & 0xff;
	sd->access_right = ar & 0xff;
	sd->limit_high = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
	sd->base_high = (base >> 24) & 0xff;
	return;
}

void set_gatedesc(struct GATE_DESCRIPTOR* gd, int offset, int selector, int ar) {
	gd->offset_low = offset & 0xffff;
	gd->selector = selector;
	gd->dw_count = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	return;
}