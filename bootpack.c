void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void write_mem8(int addr, int data);
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(char* vram, unsigned char color, int x0, int y0, int x1, int y1);

#define XSIZE 320
#define YSIZE 200
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
    init_palette();/* 设定调色板 */
    vram = (char*) 0xa0000;
	// boxfill8(vram, COL8_FF0000, 0, 0, 120, 120);
	// boxfill8(vram, COL8_000084, 70, 50, 100, 100);
	// boxfill8(vram, COL8_FFFF00, 300, 180, 320, 200);
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


    // for (i=0; i<=0xffff; ++i)
    // {
    //     p[i] = i & 0x0f;
    // }
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

void boxfill8(char* vram, unsigned char color, int x0, int y0, int x1, int y1) {
	/*
	vram坐标左上角为(0, 0), 右下角为(ysize-1, xsize-1)
	也就是说横着的是x轴，竖着的是y轴
	相应的对应到内存就是 head + y * xsize + x
	*/
	int i, j;
	if (x1 >= XSIZE) x1 = XSIZE - 1;
	if (y1 >= YSIZE) y1 = YSIZE - 1;
	for (i=y0; i<=y1; ++i)
	{
		for (j=x0; j<=x1; ++j) vram[i*XSIZE+j] = color;
	}
	return;
}