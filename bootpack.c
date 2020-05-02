#include <stdio.h>


struct BootInfo {
	char cyls, leds, vmode, reserve;
	short scrn_x, scrn_y;
	char* vram;
};

struct SEGMENT_DESCRIPTOR {
	// 8 bytes
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
	// 8 bytes
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};









void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void write_mem8(int addr, int data);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void init_palette(void);
int check_pos(int x, int limit);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(char* vram, unsigned char color, int x0, int y0, int x1, int y1);
void init_screen(char* vram);
void putfont8(char* vram, int x, int y, char color, char* font);
void putfonts8_asc(char* vram, int x, int y, char color, unsigned char* fonts);
void init_mouse_cursor8(char* mouse, char bc);
void putblock8_8(char* vram, int pxsize, int pysize, int px0, int py0, char* buf, int bxsize);
void set_segmdesc(struct SEGMENT_DESCRIPTOR* sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR* gd, int offset, int selector, int ar);
void init_gdt_idt(void);





int XSIZE = -1;
int YSIZE = -1;
#define COL8_000000 0   //黑 
#define COL8_FF0000 1   //梁红 
#define COL8_00FF00 2   //亮绿 
#define COL8_FFFF00 3   //亮黄 
#define COL8_0000FF 4   //亮蓝 
#define COL8_FF00FF 5   //亮紫 
#define COL8_00FFFF 6   //浅亮蓝
#define COL8_FFFFFF 7   //白 
#define COL8_C6C6C6 8   //亮灰 
#define COL8_840000 9   //暗红 
#define COL8_008400 10  //暗绿 
#define COL8_848400 11  //暗黄 
#define COL8_000084 12  //暗青 
#define COL8_840084 13  //暗紫 
#define COL8_008484 14  //浅暗蓝
#define COL8_848484 15  //暗灰   





void HariMain (void) {
    int i;
    char* vram;
	char mycursor[16*16];
	struct BootInfo* binfo;
	binfo = (struct BootInfo*) 0x0ff0;
	vram = binfo->vram;
	XSIZE = binfo->scrn_x;
	YSIZE = binfo->scrn_y;
	init_palette();/* 设定调色板 */
	init_screen(vram);
	init_mouse_cursor8(mycursor, COL8_008484);
	init_gdt_idt();

	putblock8_8(vram, 16, 16, 99, 99, mycursor, 16);
	unsigned char str[] = "Hello, Futeng Feng";
	putfonts8_asc(vram, 150, 100, COL8_FFFFFF, str);
	unsigned char s[50];
	sprintf(s, "screenX = %d", binfo->scrn_x);  
	putfonts8_asc(vram, 0, 10, COL8_FF0000, s);





    while (1)
    {
        io_hlt();
    }
}









void init_palette(void) {
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,    /*  0:黑 */
        0xff, 0x00, 0x00,    /*  1:梁红 */
        0x00, 0xff, 0x00,    /*  2:亮绿 */
        0xff, 0xff, 0x00,    /*  3:亮黄 */
        0x00, 0x00, 0xff,    /*  4:亮蓝 */
        0xff, 0x00, 0xff,    /*  5:亮紫 */
        0x00, 0xff, 0xff,    /*  6:浅亮蓝 */
        0xff, 0xff, 0xff,    /*  7:白 */
        0xc6, 0xc6, 0xc6,    /*  8:亮灰 */
        0x84, 0x00, 0x00,    /*  9:暗红 */
        0x00, 0x84, 0x00,    /* 10:暗绿 */
        0x84, 0x84, 0x00,    /* 11:暗黄 */
        0x00, 0x00, 0x84,    /* 12:暗青 */
        0x84, 0x00, 0x84,    /* 13:暗紫 */
        0x00, 0x84, 0x84,    /* 14:浅暗蓝 */
        0x84, 0x84, 0x84    /* 15:暗灰 */
    };
    set_palette(0, 15, table_rgb);
    return;

    /* C语言中的static char语句只能用于数据，相当于汇编中的DB指令 */
}

void init_screen(char* vram) {
	boxfill8(vram, COL8_008484,  0, 0, XSIZE -  1, YSIZE - 29);
	boxfill8(vram, COL8_C6C6C6,  0, YSIZE - 28, XSIZE -  1, YSIZE - 28);
	boxfill8(vram, COL8_FFFFFF,  0, YSIZE - 27, XSIZE -  1, YSIZE - 27);
	boxfill8(vram, COL8_C6C6C6,  0, YSIZE - 26, XSIZE -  1, YSIZE -  1);

	boxfill8(vram, COL8_FFFFFF,  3, YSIZE - 24, 59, YSIZE - 24);
	boxfill8(vram, COL8_FFFFFF,  2, YSIZE - 24,  2, YSIZE -  4);
	boxfill8(vram, COL8_848484,  3, YSIZE -  4, 59, YSIZE -  4);
	boxfill8(vram, COL8_848484, 59, YSIZE - 23, 59, YSIZE -  5);
	boxfill8(vram, COL8_000000,  2, YSIZE -  3, 59, YSIZE -  3);
	boxfill8(vram, COL8_000000, 60, YSIZE - 24, 60, YSIZE -  3);

	boxfill8(vram, COL8_848484, XSIZE - 47, YSIZE - 24, XSIZE -  4, YSIZE - 24);
	boxfill8(vram, COL8_848484, XSIZE - 47, YSIZE - 23, XSIZE - 47, YSIZE -  4);
	boxfill8(vram, COL8_FFFFFF, XSIZE - 47, YSIZE -  3, XSIZE -  4, YSIZE -  3);
	boxfill8(vram, COL8_FFFFFF, XSIZE -  3, YSIZE - 24, XSIZE -  3, YSIZE -  3);
	return;
}

void init_mouse_cursor8(char* mouse, char bc) {
	//bc is back color
	static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};
	int x, y;
	for (y=0; y<16; ++y)
	{
		for (x=0; x<16; ++x)
		{
			switch (cursor[y][x])
			{
			case '*':
				mouse[y * 16 + x] = COL8_000000;
				break;
			case 'O':
				mouse[y * 16 + x] = COL8_FFFFFF;
				break;
			case '.':
				mouse[y * 16 + x] = bc;
				break;
			default:
				break;
			}
		}
	}
	return;
}

void init_gdt_idt(void) {
	// init global (segment) description table 8bytes
	// init interupt description table

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
	
	return;
}


void set_palette(int start, int end, unsigned char *rgb) {
    int i, eflags;
    eflags = io_load_eflags();    /* 记录中断许可标志的值 */
    io_cli();                     /* 将中断许可标志置为0,禁止中断 */
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) 
    {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags);    /* 复原中断许可标志 */
    return;
}

int check_pos(int x, int limit) {
	if (x >= limit) return limit-1;
	if (x < 0) return 0;
	return x;
}

void boxfill8(char* vram, unsigned char color, int x0, int y0, int x1, int y1) {
	/*
	vram坐标左上角为(0, 0), 右下角为(ysize-1, xsize-1)
	也就是说横着的是x轴，竖着的是y轴
	相应的对应到内存就是 head + y * xsize + x
	*/
	int i, j;
	x1 = check_pos(x1, XSIZE);
	y1 = check_pos(y1, YSIZE);
	for (i=y0; i<=y1; ++i)
	{
		for (j=x0; j<=x1; ++j) vram[i*XSIZE+j] = color;
	}
	return;
}

void putfont8(char* vram, int x, int y, char color, char* font) {
	//font size is 16 rows * 8 columns
	int i;
	char* p, d;
	for (i=0; i<16; ++i)
	{
		p = vram + (y + i) * XSIZE + x;
		d = font[i];
		if ((d & 0x80) != 0) p[0] = color;
		if ((d & 0x40) != 0) p[1] = color;
		if ((d & 0x20) != 0) p[2] = color;
		if ((d & 0x10) != 0) p[3] = color;
		if ((d & 0x08) != 0) p[4] = color;
		if ((d & 0x04) != 0) p[5] = color;
		if ((d & 0x02) != 0) p[6] = color;
		if ((d & 0x01) != 0) p[7] = color;
	}
	return;
}

void putfonts8_asc(char* vram, int x, int y, char color, unsigned char* fonts) {
	//ASCII Encoding
	extern char hankaku[4096];
	int i = 0;
	for (; fonts[i] != '\0'; ++i)
	{
		putfont8(vram, x+i*8, y, color, hankaku + fonts[i] * 16);
	}
	return;
}

void putblock8_8(char* vram, int pxsize, int pysize, int px0, int py0, char* buf, int bxsize) {
	int x, y;
	for (y=0; y<pysize; ++y)
	{
		for (x=0; x<pxsize; ++x)
		{
			vram[(py0+y) * XSIZE + (px0+x)] = buf[y * bxsize + x];
		}
	}
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