#include <stdio.h>
#include <string.h>
#include "bootpack.h"


void parallel_move(struct Sheet* sht, int m, int c, int x0, int y0, int x1, int y1) {
    /*
     * 注意范围是【x0, x1】，双闭区间
     * 将矩形范围内的东西平行复制过去
     * m为正时，向上移动
     * m为负时，向下移动
     * 注意并不会影响到矩阵范围外的内容，是在矩阵范围内的移动
     * 移动后空余的部分将被填充颜色 c
     */
    int i, j;
    if (m > 0)
    {
        for (i = y0; i <= y1 - m; ++i)
        {
            for (j = x0; j <= x1; ++j)
            {
                sht->buf[j + i * sht->bxsize] = sht->buf[j + (i + m) * sht->bxsize];
            }
        }
        for (; i <= y1; ++i)
        {
            for (j = x0; j <= x1; ++j) sht->buf[j + i * sht->bxsize] = c;
        }
    }
    else if (m < 0)
    {
        for (i = y1; i >= y0 + m; --i)
        {
            for (j = x0; j <= x1; ++j)
            {
                sht->buf[j + i * sht->bxsize] = sht->buf[j + (i + m) * sht->bxsize];
            }
        }
        for (; i >= y0; --i)
        {
            for (j = x0; j <= x1; ++j) sht->buf[j + i * sht->bxsize] = c;
        }
    }
    sheet_refresh(sht, x0, y0, x1 + 1, y1 + 1);
    return;
}

void console_newline(struct Sheet* sht, int* cursor_x, int* cursor_y) {
    if (*cursor_y + LINE_GAP <= sht->cursor_y_high)
    {
        *cursor_y += LINE_GAP;
    }
    else
    {
        parallel_move(sht, LINE_GAP, COL8_C6C6C6, 0, sht->cursor_y_low,
                sht->cursor_x_high+7, sht->cursor_y_high+LINE_GAP-1);
    }
    *cursor_x = sht->cursor_x_low;
    return;
}

void task_b_main(struct Sheet* sht_win_b) {
    char s[40];
    unsigned int cnt = 0;
    while (1)
    {
        ++cnt;
//        io_cli();
        sprintf(s, "%010d", cnt);
        putfonts8_asc_sht(sht_win_b, 0, 16, COL8_FFFFFF, COL8_000000, s);
        //        io_sti();
    }
    
}

int judge_command(unsigned char* s) {
    const static int cmd_num = 3;
    static unsigned char cmds[3][10] = {
            {"free"}, {"clear"}, {"ls"},
    };
    int i;
    for (i = 0; i < cmd_num; ++i)
    {
        if (strcmp(s, cmds[i]) == 0) return i;
    }
    return -1;
}

void console_task(struct Sheet* sht, unsigned int total_mem) {
    struct Timer* timer;
    struct Task* task = task_now();
    struct MemMan* man = (struct MemMan*) ADR_MEMMAN;
    struct FileInfo* fileinfo = (struct FileInfo*) (ADR_DISKIMG + 0x002600);
    int fifobuf[128], data, cursor_x = sht->cursor_x_low, cursor_c = -1, cursor_y = sht->cursor_y_low;
    unsigned char s[40], cmdline[40];
    int cmd_judge_flag = -1;
    fifo32_init(&task->fifo, 128, fifobuf, task);
    
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 0.35 * TIMER_COUNT_PER_SECOND);
    putfonts8_asc_sht(sht, cursor_x, cursor_y, COL8_000000, COL8_C6C6C6, "$ ");
    cursor_x += 16;
    sht->cursor_x_low = 16;
    
    total_mem /= (1024 * 1024);
    
    while (1)
    {
        io_cli();
        if (fifo32_status(&task->fifo) == 0)
        {
            task_sleep(task);
            io_sti();
        }
        else
        {
            data = fifo32_get(&task->fifo);
            io_sti();
            if (data >= 256 && data < 512) // keyboard
            {
                data -= 256;
                if (data == 8) // backspace
                {
                    if (cursor_x - 8 >= sht->cursor_x_low)
                    {
                        boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15); //将当前光标隐藏
                        sheet_refresh(sht, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
                        cursor_x -= 8;
                    }
                }
                else if (data == 10) // enter
                {
                    if (cursor_x == sht->cursor_x_low) continue;
                    cmdline[(cursor_x - sht->cursor_x_low) / 8] = '\0';
                    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15); //将当前光标隐藏
                    sheet_refresh(sht, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
                    console_newline(sht, &cursor_x, &cursor_y);
                    cmd_judge_flag = judge_command(cmdline);
                    if (cmd_judge_flag == 0)
                    {
                        // free command
                        sprintf(s, "Total: %dMB, Free: %dKB", total_mem, memman_total(man) / 1024);
                        putfonts8_asc_sht(sht, cursor_x, cursor_y, COL8_000000, COL8_C6C6C6, s);
                        console_newline(sht, &cursor_x, &cursor_y);
                    }
                    else if (cmd_judge_flag == 1)
                    {
                        boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, 0, sht->cursor_y_low,
                                sht->cursor_x_high+7, sht->cursor_y_high+LINE_GAP-1);
                        sheet_refresh(sht, 0, sht->cursor_y_low,
                                      sht->cursor_x_high+8, sht->cursor_y_high+LINE_GAP);
                        cursor_x = sht->cursor_x_low;
                        cursor_y = sht->cursor_y_low;
                    }
                    else if (cmd_judge_flag == 2)
                    {
                        int t;
                        for (t = 0; t < 224; ++t)
                        {
                            if (fileinfo[t].name[0] == 0x00) break; // 已到头
                            if (fileinfo[t].name[0] != 0xe5) // 0xe5代表被删除
                            {
                                if ((fileinfo[t].type & 0x18) == 0)
                                {
                                    sprintf(s, "01234567.9AB  %07d B", fileinfo[t].size);
                                    int t2;
                                    for (t2 = 0; t2 < 8; ++t2) s[t2] = fileinfo[t].name[t2];
                                    s[9] = fileinfo[t].ext[0];
                                    s[10] = fileinfo[t].ext[1];
                                    s[11] = fileinfo[t].ext[2];
                                    if (s[9] == ' ') s[8] = ' ';
                                }
                                putfonts8_asc_sht(sht, cursor_x, cursor_y, COL8_000000, COL8_C6C6C6, s);
                                console_newline(sht, &cursor_x, &cursor_y);
                            }
                        }
                    }
                    else
                    {
                        putfonts8_asc_sht(sht, cursor_x, cursor_y, COL8_000000, COL8_C6C6C6, "Unknown Command");
                        console_newline(sht, &cursor_x, &cursor_y);
                    }
                    putfonts8_asc_sht(sht, 0, cursor_y, COL8_000000, COL8_C6C6C6, "$ ");
                }
                else
                {
                    s[0] = data;
                    s[1] = '\0';
                    cmdline[(cursor_x - sht->cursor_x_low) / 8] = data;
                    putfonts8_asc_sht(sht, cursor_x, cursor_y, COL8_000000, COL8_C6C6C6, s);
                    cursor_x += 8;
                }
                cursor_x = check_pos(cursor_x, sht->cursor_x_low, sht->cursor_x_high);
                cursor_y = check_pos(cursor_y, sht->cursor_y_low, sht->cursor_y_high);
            }
            else if (data == 4)
            {
                cursor_c = -1;
                boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, cursor_x, 16, cursor_x + 7, 16 + 15);
                sheet_refresh(sht, cursor_x, 16, cursor_x + 8, 16 + 16);
            }
            else if (data == 3)
            {
                cursor_c = 0;
            }
            else // timer
            {
                if (cursor_c < 0)
                {
                    timer_settime(timer, 0.35 * TIMER_COUNT_PER_SECOND);
                    continue;
                }
                switch (data)
                {
                    case 1:timer_init(timer, &task->fifo, 0);
                        cursor_c = COL8_FFFFFF;
                        break;
                    case 0:timer_init(timer, &task->fifo, 1);
                        cursor_c = COL8_C6C6C6;
                        break;
                }
                
                boxfill8(sht->buf, sht->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
                sheet_refresh(sht, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
                timer_settime(timer, 0.35 * TIMER_COUNT_PER_SECOND);
            }
        }
    }
}

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
    make_window(man, sht_console, 260, 180, COL8_C6C6C6, -1, "console", 0);
    
    struct Task* task_console = task_alloc();
    task_console->tss.esp = memman_alloc_4kB(man, 64 * 1024) + 64 * 1024 - 12;
    task_console->tss.eip = (int) &console_task;
    task_console->tss.es = 1 * 8;
    task_console->tss.cs = 2 * 8;
    task_console->tss.ss = 1 * 8;
    task_console->tss.ds = 1 * 8;
    task_console->tss.fs = 1 * 8;
    task_console->tss.gs = 1 * 8;
    *((int*) (task_console->tss.esp + 4)) = (int) sht_console;
    *((int*) (task_console->tss.esp + 8)) = total_mem;
    
    task_run(task_console, 2, 2); // level=2, priority=2
    
    
    struct Task *task_b[4];
    
    int i;
    for (i = 0; i < 4; ++i)
    {
        sht_win_b[i] = sheet_alloc(shtctl);
        sprintf(s, "task_b %d", i);
        make_window(man, sht_win_b[i], 160, -1, COL8_C6C6C6, -1, s, 0);
        task_b[i] = task_alloc();
        task_b[i]->tss.esp = memman_alloc_4kB(man, 64 * 1024) + 64 * 1024 - 8;
        task_b[i]->tss.eip = (int) &task_b_main;
        task_b[i]->tss.es = 1 * 8;
        task_b[i]->tss.cs = 2 * 8;
        task_b[i]->tss.ss = 1 * 8;
        task_b[i]->tss.ds = 1 * 8;
        task_b[i]->tss.fs = 1 * 8;
        task_b[i]->tss.gs = 1 * 8;
        *((int *) (task_b[i]->tss.esp + 4)) = (int) sht_win_b[i];
        task_run(task_b[i], 2, i + 1);
    }
    
    
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
                    sheet_slide(sht_text, mx, my);
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










































