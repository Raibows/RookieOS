#include <stdio.h>
#include <string.h>
#include "bootpack.h"










extern struct FIFO8 keyfifo, mousefifo;


void HariMain (void) {
	unsigned char s[40], keybuf[32], mousebuf[128];
	struct BootInfo* binfo = (struct BootInfo*) 0x0ff0;
    struct MOUSE_DEC mdec;
    struct MemMan* man = (struct MemMan*) MEMMAN_ADDR;
    struct SheetControl* shtctl;
    struct Sheet *sht_back, *sht_mouse;
    unsigned char* buf_back;
    unsigned char buf_mouse[256];
    
	int XSIZE = binfo->scrn_x;
	int YSIZE = binfo->scrn_y;
	
	init_gdt_idt();
	init_pic();
    io_sti();
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
    io_out8(PIC0_IMR, 0xf9); /* 开放PIC1和键盘中断(11111001) */
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */
	init_keyboard();
    enable_mouse(&mdec);
    memman_init(man);
    unsigned int total_mem = memtest(0x00400000, 0xbfffffff);
    memman_free(man, 0x00001000, 0x0009e000);
    memman_free(man, 0x00400000, total_mem - 0x00400000);
    
    init_palette();/* 设定调色板 */
    shtctl = sheetcontroll_init(man, binfo->vram, XSIZE, YSIZE);
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    buf_back = memman_alloc_4kB(man, XSIZE * YSIZE);
    sheet_setbuf(sht_back, buf_back, XSIZE, YSIZE, -1); //背景background没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); //透明色号99
    init_screen(buf_back, XSIZE, YSIZE);
    init_mouse_cursor8(buf_mouse, 99);
    sheet_slide(shtctl, sht_back, 0, 0);
    int mx = 99, my = 99;
    sheet_slide(shtctl, sht_mouse, mx, my);
    sheet_updown(shtctl, sht_back, 0);
    sheet_updown(shtctl, sht_mouse, 1);
    
    sprintf(s, "total: %u MB  free: %u KB block: %d",
            total_mem / 1024 / 1024, memman_total(man) / 1024, man->frees);
//    boxfill8(vram, binfo->scrn_x, binfo->scrn_y, COL8_000000, 0, 32, strlen(s)*8-1, 47);
	putfonts8_asc(buf_back, binfo->scrn_x, 0, 32, COL8_FFFFFF, s);
	sheet_refresh(shtctl, sht_back, 0, 0, XSIZE, 48);
    
    int data;

	while (1)
	{
		io_cli(); //clear interrupt flags
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) io_stihlt();
        else
        {
            if (fifo8_status(&keyfifo) != 0)
            {
                data = fifo8_get(&keyfifo);
                io_sti(); // reset interrupt flags
                sprintf(s, "%02x", data);
                boxfill8(buf_back, binfo->scrn_x, binfo->scrn_y, COL8_000000, 0, 0,  strlen(s)*8-1, 15);
                putfonts8_asc(buf_back, binfo->scrn_x, 0, 0, COL8_FFFFFF, s);
                sheet_refresh(shtctl, sht_back, 0, 0, strlen(s)*8-1, 16);
            }
            else if (fifo8_status(&mousefifo) != 0)
            {
                data = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, data) != 1) continue;
//                boxfill8(binfo->vram, binfo->scrn_x, binfo->scrn_y, COL8_008484, mx, my, mx+15, my+15); //重新上色，把上一个鼠标隐藏掉
                mx += mdec.x;
                my += mdec.y;
                mx = check_pos(mx, 0, binfo->scrn_x - 16);
                my = check_pos(my, 0, binfo->scrn_y - 16);
                sprintf(s, "[lcr %4d %4d]", mx, my);
                if ((mdec.btn & 0x01) != 0) s[1] = 'L';
                if ((mdec.btn & 0x02) != 0) s[3] = 'R';
                if ((mdec.btn & 0x04) != 0) s[2] = 'C';
                boxfill8(buf_back, binfo->scrn_x, binfo->scrn_y, COL8_000000, 0, 16, strlen(s)*8-1, 31);
                putfonts8_asc(buf_back, binfo->scrn_x, 0, 16, COL8_FFFFFF, s);
                sheet_refresh(shtctl, sht_back, 0, 16, strlen(s)*8+1, 32);
//                putblock8_8(vram, XSIZE, 16, 16, mx, my, mycursor, 16); //绘制当前鼠标
                sheet_slide(shtctl, sht_mouse, mx, my);
                
            }
            
        }
	}




    while (1)
    {
        io_hlt();
    }
}












































