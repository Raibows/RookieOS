#include <stdio.h>
#include <string.h>
#include "bootpack.h"


void make_window8(unsigned char *buf, int xsize, int ysize, char *title)
{
    static char closebtn[14][16] = {
            "OOOOOOOOOOOOOOO@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQ@@QQQQ@@QQ$@",
            "OQQQQ@@QQ@@QQQ$@",
            "OQQQQQ@@@@QQQQ$@",
            "OQQQQQQ@@QQQQQ$@",
            "OQQQQQ@@@@QQQQ$@",
            "OQQQQ@@QQ@@QQQ$@",
            "OQQQ@@QQQQ@@QQ$@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQQQQQQQQQQQ$@",
            "O$$$$$$$$$$$$$$@",
            "@@@@@@@@@@@@@@@@"
    };
    
    int x, y;
    char c;
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0 );
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, xsize - 2, 1 );
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_000084, 3, 3, xsize - 4, 20 );
    boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0, ysize - 1, xsize - 1, ysize - 1);
    putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
    
    for (y = 0; y < 14; y++) {
        for (x = 0; x < 16; x++) {
            c = closebtn[y][x];
            if (c == '@') {
                c = COL8_000000;
            } else if (c == '$') {
                c = COL8_848484;
            } else if (c == 'Q') {
                c = COL8_C6C6C6;
            } else {
                c = COL8_FFFFFF;
            }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}







extern struct FIFO8 keyfifo, mousefifo;
extern struct TimerControl timerctl;

void HariMain (void) {
    unsigned char s[40], keybuf[32], mousebuf[256], timerbuf[8], timerbuf2[8], timerbuf3[8];
    struct BootInfo* binfo = (struct BootInfo*) 0x0ff0;
    struct MOUSE_DEC mdec;
    struct MemMan* man = (struct MemMan*) MEMMAN_ADDR;
    struct SheetControl* shtctl;
    struct Sheet *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, *buf_win, *buf_mouse;
    struct FIFO8 timer_fifo, timer_fifo2, timer_fifo3;
    struct Timer *timer, *timer2, *timer3;
//    unsigned char buf_mouse[256];
    
    int XSIZE = binfo->scrn_x;
    int YSIZE = binfo->scrn_y;
    
    init_gdt_idt();
    init_pic();
    io_sti();
    fifo8_init(&keyfifo, sizeof(keybuf), keybuf);
    fifo8_init(&mousefifo, sizeof(mousebuf), mousebuf);
    fifo8_init(&timer_fifo, sizeof(timerbuf), timerbuf);
    fifo8_init(&timer_fifo2, sizeof(timerbuf2), timerbuf2);
    fifo8_init(&timer_fifo3, sizeof(timerbuf3), timerbuf3);
    init_pit();
    io_out8(PIC0_IMR, 0xf8); /* 开放PIC1和键盘中断和PIT，timer中断(11111000) */
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
    sht_win = sheet_alloc(shtctl);
    buf_back = (unsigned char *) memman_alloc_4kB(man, XSIZE * YSIZE);
    buf_mouse = (unsigned char *) memman_alloc_4kB(man, 16 * 16);
    buf_win = (unsigned char *) memman_alloc_4kB(man, 160 * 68);
    sheet_setbuf(sht_back, buf_back, XSIZE, YSIZE, -1); //背景background没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); //透明色号99
    sheet_setbuf(sht_win, buf_win, 160, 68, -1);
    init_screen(buf_back, XSIZE, YSIZE);
    init_mouse_cursor8(buf_mouse, 99);
    make_window8(buf_win, 160, 68, "Counter");
    sheet_slide(sht_back, 0, 0);
    int mx = 99, my = 99;
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 99, 99);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_mouse, 2);
    sheet_updown(sht_win, 1);
    
    sprintf(s, "total: %u MB  free: %u KB block: %d",
            total_mem / 1024 / 1024, memman_total(man) / 1024, man->frees);
    boxfill8(buf_back, XSIZE, COL8_000000, 0, 32, strlen(s)*8-1, 32+16-1);
    putfonts8_asc(buf_back, XSIZE, 0, 32, COL8_FFFFFF, s);
    sheet_refresh(sht_back, 0, 0, XSIZE, 48);
    
    timer = timer_alloc();
    timer2 = timer_alloc();
    timer3 = timer_alloc();
    timer_init(timer, &timer_fifo, 1);
    timer_init(timer2, &timer_fifo2, 1);
    timer_init(timer3, &timer_fifo3, 1);
    timer_settime(timer, 10 * TIMER_COUNT_PER_SECOND);
    timer_settime(timer2, 3 * TIMER_COUNT_PER_SECOND);
    timer_settime(timer3, 0.5 * TIMER_COUNT_PER_SECOND);
    
    
    int data;
    
    while (1)
    {
        sprintf(s, "%010u", timerctl.count);
        boxfill8(buf_win, 160, COL8_C6C6C6, 24, 30, 24+strlen(s)*8, 45);
        putfonts8_asc(buf_win, 160, 24, 30, COL8_000000, s);
        sheet_refresh(sht_win, 24, 30, 24+strlen(s)*8-1, 45);
        io_cli(); //clear interrupt flags
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) + fifo8_status(&timer_fifo)
            + fifo8_status(&timer_fifo2) + fifo8_status(&timer_fifo3) == 0)
        {
            io_sti();
        }
        else
        {
            if (fifo8_status(&keyfifo) != 0)
            {
                data = fifo8_get(&keyfifo);
                io_sti(); // reset interrupt flags
                sprintf(s, "%02x", data);
                boxfill8(buf_back, XSIZE, COL8_000000, 0, 0,  strlen(s)*8-1, 15);
                putfonts8_asc(buf_back, XSIZE, 0, 0, COL8_FFFFFF, s);
                sheet_refresh(sht_back, 0, 0, strlen(s)*8-1, 16);
            }
            else if (fifo8_status(&mousefifo) != 0)
            {
                data = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, data) != 1) continue;
                mx += mdec.x;
                my += mdec.y;
                mx = check_pos(mx, -15, binfo->scrn_x - 1);
                my = check_pos(my, -15, binfo->scrn_y - 1);
                sprintf(s, "[lcr %4d %4d]", mx, my);
                if ((mdec.btn & 0x01) != 0) s[1] = 'L';
                if ((mdec.btn & 0x02) != 0) s[3] = 'R';
                if ((mdec.btn & 0x04) != 0) s[2] = 'C';
                boxfill8(buf_back, XSIZE, COL8_000000, 0, 16, strlen(s)*8-1, 31);
                putfonts8_asc(buf_back, XSIZE, 0, 16, COL8_FFFFFF, s);
                sheet_refresh(sht_back, 0, 16, strlen(s)*8+1, 32);
                sheet_slide(sht_mouse, mx, my);
            }
            else if (fifo8_status(&timer_fifo) != 0)
            {
                data = fifo8_get(&timer_fifo);
                io_sti();
                putfonts8_asc(buf_back, XSIZE, 0, 48, COL8_000000, "Ten seconds!");
                sheet_refresh(sht_back, 0, 48, 12*8-1, 48+15);
            }
            else if (fifo8_status(&timer_fifo2) != 0)
            {
                data = fifo8_get(&timer_fifo2);
                io_sti();
                putfonts8_asc(buf_back, XSIZE, 0, 64, COL8_000000, "3 seconds!");
                sheet_refresh(sht_back, 0, 64, 10*8-1, 64+15);
            }
            else if (fifo8_status(&timer_fifo3) != 0)
            {
                //模拟闪烁
                data = fifo8_get(&timer_fifo3);
                io_sti();
                if (data == 1)
                {
                    timer_init(timer3, &timer_fifo3, 0);
                    boxfill8(buf_back, XSIZE, COL8_FFFFFF, 50, 80, 57, 95);
                }
                else if (data == 0)
                {
                    timer_init(timer3, &timer_fifo3, 1);
                    boxfill8(buf_back, XSIZE, COL8_008484, 50, 80, 57, 95);
                }
                sheet_refresh(sht_back, 50, 80, 58, 96);
                timer_settime(timer3, 0.5 * TIMER_COUNT_PER_SECOND);
            }
            
        }
    }




    while (1)
    {
        io_hlt();
    }
}












































