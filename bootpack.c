#include <stdio.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;
void init_keyboard(void);
void enable_mouse(void);

void HariMain (void) {
    char* vram;
	char mycursor[16*16];
	unsigned char s[40], keybuf[32], mousebuf[128];
	struct BootInfo* binfo;
	binfo = (struct BootInfo*) 0x0ff0;
	vram = binfo->vram;
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
	
	
    init_palette();/* 设定调色板 */
    init_screen(vram, XSIZE, YSIZE);
    init_mouse_cursor8(mycursor, COL8_008484);
    putblock8_8(vram, XSIZE, 16, 16, 99, 99, mycursor, 16);
    
    enable_mouse();
	
	
	
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
                boxfill8(binfo->vram, binfo->scrn_x, binfo->scrn_y, COL8_008484, 0, 0, 32 * 8 - 1, 15);
                putfonts8_asc(binfo->vram, binfo->scrn_x, 0, 0, COL8_FFFFFF, s);
            }
            else if (fifo8_status(&mousefifo) != 0)
            {
                data = fifo8_get(&mousefifo);
                io_sti();
                sprintf(s, "%02x", data);
                boxfill8(binfo->vram, binfo->scrn_x, binfo->scrn_y, COL8_000000, 0, 16, 31, 31);
                putfonts8_asc(binfo->vram, binfo->scrn_x, 0, 16, COL8_FFFFFF, s);
            }
            
        }
	}




    while (1)
    {
        io_hlt();
    }
}

#define PORT_KEYDAT	0x0060
#define PORT_KEYSTA	0x0064
#define PORT_KEYCMD	0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47
#define KEYCMD_SENDTO_MOUSE	0xd4
#define MOUSECMD_ENABLE	0xf4

void wait_KBC_sendready(void) {
    /*
     * 等待键盘控制电路准备完毕
     * 使用KEYSTA_SEND_NOTREADY，来判断低二位应该是0就对了
     * */
    while (1)
    {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) return;
    }
}

void init_keyboard(void) {
    /*
     * 初始化键盘控制电路
     */
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

void enable_mouse(void) {
    /*
     * 激活鼠标
     * */
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    return; // 顺利的话，键盘控制器会返回ACK(0xfa)
}