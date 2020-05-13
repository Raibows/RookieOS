#include "bootpack.h"

//struct FIFO8 keyfifo;
struct FIFO32* keyfifo;
int keydata;


void int_handler21(int* esp) {
    // 用于0x21的中断, keyboard
    int data;
    io_out8(PIC0_OCW2, 0x61); //通知pic-irq01受理完毕
    data = io_in8(PORT_KEYDAT);
    fifo32_put(keyfifo, keydata + data);
    return;
}


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

void init_keyboard(struct FIFO32* fifo, int data) {
    /*
     * 初始化键盘控制电路
     */
    keyfifo = fifo;
    keydata = data;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}
















