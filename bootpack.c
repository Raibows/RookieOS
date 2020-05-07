#include <stdio.h>
#include <string.h>
#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;
void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC* mdec);
int mouse_decode(struct MOUSE_DEC* mdec, unsigned char data);

void HariMain (void) {
    char* vram;
	char mycursor[16*16];
	unsigned char s[40], keybuf[32], mousebuf[128];
	struct BootInfo* binfo;
    struct MOUSE_DEC mdec;
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
    int mx = 99, my = 99;
    putblock8_8(vram, XSIZE, 16, 16, mx, my, mycursor, 16);
    
    enable_mouse(&mdec);
	
	
	
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
                if (mouse_decode(&mdec, data) != 1) continue;
                sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                if ((mdec.btn & 0x01) != 0) s[1] = 'L';
                if ((mdec.btn & 0x02) != 0) s[3] = 'R';
                if ((mdec.btn & 0x04) != 0) s[2] = 'C';
                boxfill8(binfo->vram, binfo->scrn_x, binfo->scrn_y, COL8_000000, 0, 16, strlen(s)*8-1, 31);
                putfonts8_asc(binfo->vram, binfo->scrn_x, 0, 16, COL8_FFFFFF, s);
                boxfill8(binfo->vram, binfo->scrn_x, binfo->scrn_y, COL8_008484, mx, my, mx+15, my+15); //重新上色，把上一个鼠标隐藏掉
                mx += mdec.x;
                my += mdec.y;
                mx = check_pos(mx, 0, binfo->scrn_x - 16);
                my = check_pos(my, 0, binfo->scrn_y - 16);
                putblock8_8(vram, XSIZE, 16, 16, mx, my, mycursor, 16); //绘制当前鼠标
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

void enable_mouse(struct MOUSE_DEC* mdec) {
    /*
     * 激活鼠标
     * */
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0; // 等待0xfa
    return; // 顺利的话，键盘控制器会返回ACK(0xfa)
}

int mouse_decode(struct MOUSE_DEC* mdec, unsigned char data) {
    /*
     * decode mouse
     */
    if (mdec->phase == 0)
    {
        if (data == 0xfa) mdec->phase = 1;
        return 0;
    }
    if (mdec->phase == 1 && (data & 0xc8) != 0x08) return -1; //第1字节不正确
    mdec->buf[mdec->phase-1] = data;
    ++mdec->phase;
    if (mdec->phase == 4)
    {
        mdec->phase = 1;
        mdec->btn = mdec->buf[0] & 0x07;
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if ((mdec->buf[0] & 0x10) != 0) {
            mdec->x |= 0xffffff00;
        }
        if ((mdec->buf[0] & 0x20) != 0) {
            mdec->y |= 0xffffff00;
        }
        /* 鼠标的y方向与画面符号相反 */
        mdec->y = -mdec->y;
        return 1;
    }
    return -1;
}









































