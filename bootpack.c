#include <stdio.h>
#include <string.h>
#include "bootpack.h"


void make_window8(unsigned char *buf, int xsize, int ysize, char *title) {
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


void make_textbox8(struct Sheet* sht, int x0, int y0, int sx, int sy, int c, char* title) {
    int x1 = x0 + sx, y1 = y0 + sy;
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c, x0 - 1, y0 - 1, x1 + 0, y1 + 0);
    putfonts8_asc_sht(sht, sx / 2 - strlen(title) / 2, 0, COL8_000000, COL8_FFFFFF, title);
    return;
}




extern struct TimerControl timerctl;


void HariMain (void) {
    unsigned char s[40];
    struct BootInfo* binfo = (struct BootInfo*) 0x0ff0;
    struct MOUSE_DEC mdec;
    struct MemMan* man = (struct MemMan*) MEMMAN_ADDR;
    struct SheetControl* shtctl;
    struct Sheet *sht_back, *sht_mouse, *sht_win, *sht_text;
    unsigned char *buf_back, *buf_win, *buf_mouse;
    struct FIFO32 fifo;
    int fifo_buf[256];
    struct Timer *timer, *timer2, *timer3;
    
    int XSIZE = binfo->scrn_x;
    int YSIZE = binfo->scrn_y;
    
    init_gdt_idt();
    init_pic();
    io_sti();
    fifo32_init(&fifo, 256, fifo_buf);
    init_pit();
    io_out8(PIC0_IMR, 0xf8); /* 开放PIC1和键盘中断和PIT，timer中断(11111000) */
    io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
    memman_init(man);
    unsigned int total_mem = memtest(0x00400000, 0xbfffffff);
    memman_free(man, 0x00001000, 0x0009e000);
    memman_free(man, 0x00400000, total_mem - 0x00400000);
    
    init_palette();/* 设定调色板 */
    shtctl = sheetcontroll_init(man, binfo->vram, XSIZE, YSIZE);
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    sht_win = sheet_alloc(shtctl);
    sht_text = sheet_alloc(shtctl);
    
    buf_back = (unsigned char*) memman_alloc_4kB(man, XSIZE * YSIZE);
    buf_mouse = (unsigned char*) memman_alloc_4kB(man, 16 * 16);
    buf_win = (unsigned char*) memman_alloc_4kB(man, 160 * 68);
    
    sheet_setbuf(sht_back, buf_back, XSIZE, YSIZE, -1); //背景background没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); //透明色号99
    sheet_setbuf(sht_win, buf_win, 160, 68, -1);
    
    init_screen(buf_back, XSIZE, YSIZE);
    init_mouse_cursor8(buf_mouse, 99);
    make_window8(buf_win, 160, 68, "Counter");
//    make_textbox8(sht_text, 8, 28, 144, 16, COL8_FFFFFF, "window");
    make_window(man, sht_text, 160, -1, COL8_C6C6C6, -1, "test");
    
    int mx = 99, my = 99;
    
    sheet_slide(sht_back, 0, 0);
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 99, 99);
    sheet_slide(sht_text, mx, my);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_mouse, 2);
    sheet_updown(sht_win, -1);
    sheet_updown(sht_text, 1);
    
    sprintf(s, "total: %u MB  free: %u KB block: %d",
            total_mem / 1024 / 1024, memman_total(man) / 1024, man->frees);
    putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_000000, s);

    
    timer = timer_alloc();
    timer2 = timer_alloc();
    timer3 = timer_alloc();
    timer_init(timer, &fifo, 10);
    timer_init(timer2, &fifo, 3);
    timer_init(timer3, &fifo, 1);
    timer_settime(timer, 10 * TIMER_COUNT_PER_SECOND);
    timer_settime(timer2, 3 * TIMER_COUNT_PER_SECOND);
    timer_settime(timer3, 0.35 * TIMER_COUNT_PER_SECOND);
    
    
    int data;
    int cursor_x, cursor_c;
    cursor_x = 0;
    cursor_c = COL8_FFFFFF;
    static char keytable[0x54] = {
            0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', '\b', 0, //15
            'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0, 0, 'A', 'S', //31
            'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0, 0, ']', 'Z', 'X', 'C', 'V', //47
            'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, //63
            0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', 75, '5', 77, '+', '1', //79
            '2', '3', '0', '.' //83
    };
    
    
    while (1)
    {
        io_cli(); //clear interrupt flags
        if (fifo32_status(&fifo) == 0)
        {
            io_stihlt();
        }
        else
        {
            data = fifo32_get(&fifo);
            io_sti(); // reset interrupt flags
            if (data >= 256 && data < 512)
            {
                //keyboard
                data -= 256;
                sprintf(s, "%03d", data);
                putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_000000, s);
                if (data > 0x53 || keytable[data] == 0) continue;
                s[0] = keytable[data];
                s[1] = '\0';
                boxfill8(sht_text->buf, sht_text->bxsize, COL8_C6C6C6, cursor_x, 16, cursor_x + 7, 31); //将当前光标隐藏
                sheet_refresh(sht_text, cursor_x, 16, cursor_x + 8, 32);
                if (data == 77)
                {
                    cursor_x += 8;
                }
                else if (data == 75)
                {
                    cursor_x -= 8;
                }
                else if (data == 0x0e) //backspace
                {
                    cursor_x -= 8;
                }
                else
                {
                    putfonts8_asc_sht(sht_text, cursor_x, 16, COL8_000000, COL8_C6C6C6, s);
                    cursor_x += 8;
                }
                cursor_x = check_pos(cursor_x, 0, 152);
            }
            else if (data >= 512 && data < 768)
            {
                //mouse
                if (mouse_decode(&mdec, data - 512) != 1) continue;
                mx += mdec.x;
                my += mdec.y;
                mx = check_pos(mx, -15, binfo->scrn_x - 1);
                my = check_pos(my, -15, binfo->scrn_y - 1);
                sprintf(s, "[lcr %4d %4d]", mx, my);
                if ((mdec.btn & 0x01) != 0) s[1] = 'L';
                else if ((mdec.btn & 0x02) != 0) s[3] = 'R';
                else if ((mdec.btn & 0x04) != 0) s[2] = 'C';
                putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_000000, s);
                sheet_slide(sht_mouse, mx, my);
                if ((mdec.btn & 0x01) != 0)
                {
                    sheet_slide(sht_text, mx, my);
                }
            }
            else
            {
                switch (data)
                {
                    case 10:
                        putfonts8_asc_sht(sht_back, 0, 48, COL8_FFFFFF, COL8_000000, "ten seconds!");
                        break;
                    case 3:
                        putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_000000, "3 seconds!");
                        break;
                    case 1:
                        timer_init(timer3, &fifo, 0);
                        boxfill8(sht_text->buf, sht_text->bxsize, cursor_c, cursor_x, 16, cursor_x + 7, 31);
                        sheet_refresh(sht_text, cursor_x, 16, cursor_x + 8, 32);
                        cursor_c = COL8_C6C6C6;
                        timer_settime(timer3, 0.35 * TIMER_COUNT_PER_SECOND);
                        break;
                    case 0:
                        timer_init(timer3, &fifo, 1);
                        boxfill8(sht_text->buf, sht_text->bxsize, cursor_c, cursor_x, 16, cursor_x + 7, 31);
                        sheet_refresh(sht_text, cursor_x, 16, cursor_x + 8, 32);
                        cursor_c = COL8_FFFFFF;
                        timer_settime(timer3, 0.35 * TIMER_COUNT_PER_SECOND);
                        break;
                }
                
            }
        }
    }




    while (1)
    {
        io_hlt();
    }
}












































