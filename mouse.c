
#include "bootpack.h"

//struct FIFO8 mousefifo;
struct FIFO32* mousefifo;
int mousedata;

void int_handler2c(int *esp) {
    /* 来自PS/2鼠标的中断 */
    int data;
    io_out8(PIC1_OCW2, 0x64);	// 通知PIC IRQ-12 已经受理完毕 先从
    io_out8(PIC0_OCW2, 0x62);	// 通知PIC IRQ-02 已经受理完毕  后主
    data = io_in8(PORT_KEYDAT);
    fifo32_put(mousefifo, data + mousedata);
    return;
}

void enable_mouse(struct FIFO32* fifo, int data, struct MOUSE_DEC* mdec) {
    /*
     * 激活鼠标
     * */
    mousefifo = fifo;
    mousedata = data;
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









