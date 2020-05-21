#include <stdio.h>
#include <string.h>
#include "bootpack.h"




int XSIZE;
int YSIZE;
extern struct TimerControl timerctl;


void HariMain (void) {
    unsigned char s[40];
    struct BootInfo* binfo = (struct BootInfo*) 0x0ff0;
    struct MOUSE_DEC mdec;
    struct MemMan* man = (struct MemMan*) ADR_MEMMAN;
    struct SheetControl* shtctl;
    struct Sheet *sht_back, *sht_mouse, *sht_text, *sht_win_b[4], *sht_console;
    unsigned char *buf_back, *buf_mouse;
    struct FIFO32 fifo;
    int fifo_buf[256];
    struct Timer *timer;
    struct Task* task_a;
    
    XSIZE = binfo->scrn_x;
    YSIZE = binfo->scrn_y;
    
    init_gdt_idt();
    init_pic();
    io_sti();
    fifo32_init(&fifo, 256, fifo_buf, NULL);
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
    
    task_a = task_init(man);
    fifo.task = task_a;
    task_run(task_a, 1, 0);
    
    sht_back = sheet_alloc(shtctl);
    debug_init(sht_back);
    sht_mouse = sheet_alloc(shtctl);
    sht_text = sheet_alloc(shtctl);
    sht_console = sheet_alloc(shtctl);
    
    buf_back = (unsigned char*) memman_alloc_4kB(man, XSIZE * YSIZE);
    buf_mouse = (unsigned char*) memman_alloc_4kB(man, 16 * 16);
    
    sheet_setbuf(sht_back, buf_back, XSIZE, YSIZE, -1); //背景background没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); //透明色号99
    
    init_screen(buf_back, XSIZE, YSIZE);
    init_mouse_cursor8(buf_mouse, 99);
    make_window(man, sht_text, 160, -1, COL8_C6C6C6, -1, "test", 1);
    make_window(man, sht_console, 280, 220, COL8_C6C6C6, -1, "console", 1);
    
    struct Task* task_console = task_alloc();
    struct Task *task_b[4];
    
    task_console->tss.esp = memman_alloc_4kB(man, 64 * 1024) + 64 * 1024 - 20;
    task_console->tss.eip = (int) &console_task;
    task_console->tss.es = 1 * 8;
    task_console->tss.cs = 2 * 8;
    task_console->tss.ss = 1 * 8;
    task_console->tss.ds = 1 * 8;
    task_console->tss.fs = 1 * 8;
    task_console->tss.gs = 1 * 8;
    *((int*) (task_console->tss.esp + 4)) = (int) sht_console;
    *((int*) (task_console->tss.esp + 8)) = total_mem;
    
    
    
    
    
    int i;
    for (i = 0; i < 4; ++i)
    {
        sht_win_b[i] = sheet_alloc(shtctl);
        sprintf(s, "task_b %d", i);
        make_window(man, sht_win_b[i], 160, -1, COL8_C6C6C6, -1, s, 0);
        task_b[i] = task_alloc();
        task_b[i]->tss.esp = memman_alloc_4kB(man, 64 * 1024) + 64 * 1024 - 8;
        task_b[i]->tss.eip = (int) &task_count_main;
        task_b[i]->tss.es = 1 * 8;
        task_b[i]->tss.cs = 2 * 8;
        task_b[i]->tss.ss = 1 * 8;
        task_b[i]->tss.ds = 1 * 8;
        task_b[i]->tss.fs = 1 * 8;
        task_b[i]->tss.gs = 1 * 8;
        *((int *) (task_b[i]->tss.esp + 4)) = (int) sht_win_b[i];
        task_run(task_b[i], 2, i + 1);
    }
    
    *((int*) (task_console->tss.esp + 12)) = (int) task_b[0];
    *((int*) (task_console->tss.esp + 16)) = (int) sht_win_b[0];
    task_run(task_console, 2, 2); // level=2, priority=2
    
    int mx = 99, my = 99;
    
    sheet_slide(sht_back, 0, 0);
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_text, mx, my);
    sheet_slide(sht_win_b[0], 50, 100);
    sheet_slide(sht_win_b[1], 220, 100);
    sheet_slide(sht_win_b[2], 50, 150);
    sheet_slide(sht_win_b[3], 220, 150);
    sheet_slide(sht_console, 220, 200);
    sheet_updown(sht_back, 0);
    for (i = 0; i < 4; ++i) sheet_updown(sht_win_b[i], i + 1);
    sheet_updown(sht_text, 5);
    sheet_updown(sht_console, 6);
    sheet_updown(sht_mouse, 7);
    
    
    
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settime(timer, 0.35 * TIMER_COUNT_PER_SECOND);
    
    
    int data;
    int cursor_x, cursor_c;
    cursor_x = 0;
    cursor_c = COL8_FFFFFF;
    static char keytable0[0x80] = {
            0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
            'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
            'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
            'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
            '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
    }; // not pressing SHIFT
    static char keytable1[0x80] = {
            0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0,   0,
            'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
            'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
            'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
            '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
            0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
    }; // pressing SHIFT
    
    
    int tab_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7;
    
    while (1)
    {
        io_cli(); //clear interrupt flags
        if (fifo32_status(&fifo) == 0)
        {
            task_sleep(fifo.task);
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
                if (data < 0x80) // default characters
                {
                    if (key_shift == 0) s[0] = keytable0[data];
                    else s[0] = keytable1[data];
                    if (s[0] != 0)
                    {
                        if (s[0] >= 'A' && s[0] <= 'Z')
                        {
                            if (((key_leds & 4) == 0 && key_shift == 0) ||
                                ((key_leds & 4) != 0 && key_shift != 0))
                            {
                                s[0] += 0x20; // 将大写字母转换为小写字母
                            }
                        }
                        s[1] = '\0';
                        if (tab_to == 0) // task_a
                        {
                            putfonts8_asc_sht(sht_text, cursor_x, 16, COL8_000000, COL8_C6C6C6, s);
                            cursor_x += 8;
                        }
                        else fifo32_put(&task_console->fifo, s[0] + 256);
                    }
                }
                if (data == 0x0e) //backspace, 在ASCII码中为8
                {
                    if (tab_to == 0)
                    {
                        boxfill8(sht_text->buf, sht_text->bxsize, COL8_C6C6C6, cursor_x, 16, cursor_x + 7, 31); //将当前光标隐藏
                        sheet_refresh(sht_text, cursor_x, 16, cursor_x + 8, 32);
                        cursor_x -= 8;
                    }
                    else fifo32_put(&task_console->fifo, 8 + 256);
                }
                else if (data == 0x0f) // tab
                {
                    if (tab_to == 0)
                    {
                        tab_to = 1;
                        cursor_c = -1;
                        boxfill8(sht_text->buf, sht_text->bxsize, COL8_C6C6C6, cursor_x, 16, cursor_x + 7, 31);
                        sheet_refresh(sht_text, cursor_x, 16, cursor_x + 8, 32);
                        make_title(sht_console, "Console", 1, 1);
                        make_title(sht_text, "test", 0, 1);
                        fifo32_put(&task_console->fifo, 3); // on
                    }
                    else if (tab_to == 1)
                    {
                        tab_to = 0;
                        cursor_c = 0;
                        make_title(sht_console, "Console", 0, 1);
                        make_title(sht_text, "test", 1, 1);
                        fifo32_put(&task_console->fifo, 4); // off
                    }
                }
                else if (data == 0x1c) // enter
                {
                    // enter ASCII = 10
                    if (tab_to == 1) fifo32_put(&task_console->fifo, 10 + 256);
                }
                else if (data == 0x2a) // left shift on
                {
                    key_shift |= 1;
                }
                else if (data == 0x36) // right shift on
                {
                    key_shift |= 2;
                }
                else if (data == 0xaa) // left shift off
                {
                    key_shift &= ~1;
                }
                else if (data == 0xb6)
                {
                    key_shift &= ~2;
                }
                else if (data == 0x3a)
                { /* CapsLock */
                    key_leds ^= 4;
                }
                else if (data == 0x45)
                { /* NumLock */
                    key_leds ^= 2;
                }
                else if (data == 0x46)
                { /* ScrollLock */
                    key_leds ^= 1;
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
                    if (tab_to == 0) sheet_slide(sht_text, mx, my);
                    else if (tab_to == 1) sheet_slide(sht_console, mx, my);
                }
            }
            else
            {
                if (cursor_c < 0)
                {
                    timer_settime(timer, 0.35 * TIMER_COUNT_PER_SECOND);
                    continue;
                }
                switch (data)
                {
                    case 1:
                        timer_init(timer, &fifo, 0);
                        cursor_c = COL8_C6C6C6;
                        break;
                    case 0:
                        timer_init(timer, &fifo, 1);
                        cursor_c = COL8_FFFFFF;
                        break;
                }
                boxfill8(sht_text->buf, sht_text->bxsize, cursor_c, cursor_x, 16, cursor_x + 7, 31);
                sheet_refresh(sht_text, cursor_x, 16, cursor_x + 8, 32);
                timer_settime(timer, 0.35 * TIMER_COUNT_PER_SECOND);
                
            }
        }
    }
}










































