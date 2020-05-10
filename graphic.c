#include "bootpack.h"

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

void init_screen(char* vram, int XSIZE, int YSIZE) {
	boxfill8(vram, XSIZE, COL8_008484,  0, 0, XSIZE -  1, YSIZE - 29);
	boxfill8(vram, XSIZE, COL8_C6C6C6,  0, YSIZE - 28, XSIZE -  1, YSIZE - 28);
	boxfill8(vram, XSIZE, COL8_FFFFFF,  0, YSIZE - 27, XSIZE -  1, YSIZE - 27);
	boxfill8(vram, XSIZE, COL8_C6C6C6,  0, YSIZE - 26, XSIZE -  1, YSIZE -  1);
	boxfill8(vram, XSIZE, COL8_FFFFFF,  3, YSIZE - 24, 59, YSIZE - 24);
	boxfill8(vram, XSIZE, COL8_FFFFFF,  2, YSIZE - 24,  2, YSIZE -  4);
	boxfill8(vram, XSIZE, COL8_848484,  3, YSIZE -  4, 59, YSIZE -  4);
	boxfill8(vram, XSIZE, COL8_848484, 59, YSIZE - 23, 59, YSIZE -  5);
	boxfill8(vram, XSIZE, COL8_000000,  2, YSIZE -  3, 59, YSIZE -  3);
	boxfill8(vram, XSIZE, COL8_000000, 60, YSIZE - 24, 60, YSIZE -  3);
	boxfill8(vram, XSIZE, COL8_848484, XSIZE - 47, YSIZE - 24, XSIZE -  4, YSIZE - 24);
	boxfill8(vram, XSIZE, COL8_848484, XSIZE - 47, YSIZE - 23, XSIZE - 47, YSIZE -  4);
	boxfill8(vram, XSIZE, COL8_FFFFFF, XSIZE - 47, YSIZE -  3, XSIZE -  4, YSIZE -  3);
	boxfill8(vram, XSIZE, COL8_FFFFFF, XSIZE -  3, YSIZE - 24, XSIZE -  3, YSIZE -  3);
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

int check_pos(int x, int low, int high) {
	if (x > high) return high;
	if (x < low) return low;
	return x;
}

void boxfill8(char* vram, int XSIZE, unsigned char color, int x0, int y0, int x1, int y1) {
	/*
	vram坐标左上角为(0, 0), 右下角为(ysize-1, xsize-1)
	也就是说横着的是x轴，竖着的是y轴
	相应的对应到内存就是 head + y * xsize + x
	*/
	int i, j;
	for (i=y0; i<=y1; ++i)
	{
		for (j=x0; j<=x1; ++j) vram[i*XSIZE+j] = color;
	}
	return;
}

void putfont8(char* vram, int XSIZE, int x, int y, char color, char* font) {
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

void putfonts8_asc(char* vram, int XSIZE, int x, int y, char color, unsigned char* fonts) {
	//ASCII Encoding
	extern char hankaku[4096];
	int i = 0;
	for (; fonts[i] != '\0'; ++i)
	{
		putfont8(vram, XSIZE, x+i*8, y, color, hankaku + fonts[i] * 16);
	}
	return;
}

void putblock8_8(char* vram, int XSIZE, int pxsize, int pysize, int px0, int py0, char* buf, int bxsize) {
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






