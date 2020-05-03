#include <stdio.h>
#include "bootpack.h"



void HariMain (void) {
    char* vram;
	char mycursor[16*16];
	struct BootInfo* binfo;
	binfo = (struct BootInfo*) 0x0ff0;
	vram = binfo->vram;
	int XSIZE = binfo->scrn_x;
	int YSIZE = binfo->scrn_y;
	init_palette();/* 设定调色板 */
	init_screen(vram, XSIZE, YSIZE);
	init_mouse_cursor8(mycursor, COL8_008484);
	init_gdt_idt();
	init_pic();
	io_sti();
	io_out8(PIC0_IMR, 0xf9); /* 开放PIC1和键盘中断(11111001) */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */

	putblock8_8(vram, XSIZE, 16, 16, 99, 99, mycursor, 16);
	/*unsigned char str[] = "Hello, Futeng Feng";
	putfonts8_asc(vram, XSIZE, 150, 100, COL8_FFFFFF, str);
	unsigned char s[50];
	sprintf(s, "screenX = %d", binfo->scrn_x);  
	putfonts8_asc(vram, XSIZE, 0, 10, COL8_FF0000, s);*/





    while (1)
    {
        io_hlt();
    }
}


