#include "bootpack.h"
#include "string.h"

struct ConsoleControl* conctl;


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

void console_printfile(struct Sheet* sht, char* buf, int size, int* cursor_x, int* cursor_y) {
    /*
     * sht是console窗口
     * buf为已经从file中读取好的内容
     * 注意size大小是实际文件大小，而不是buf大小（buf可能因4kB补齐而比实际大）
     */
    unsigned int i;
    unsigned char s[2];
    s[1] = '\0';
    for (i = 0; i < size; ++i)
    {
        s[0] = buf[i];
        int t;
        switch (s[0])
        {
            case 0x09: // \t
                for (t = 0; t < 4; ++t)
                {
                    putfonts8_asc_sht(sht, *cursor_x, *cursor_y, COL8_000000, COL8_C6C6C6, " ");
                    *cursor_x += 8;
                    if (*cursor_x >= sht->cursor_x_high)
                    {
                        console_newline(sht, cursor_x, cursor_y);
                        break;
                    }
                }
                break;
            case 0x0a: // \n
                console_newline(sht, cursor_x, cursor_y);
                break;
            case 0x0d: break; // \r
            default:putfonts8_asc_sht(sht, *cursor_x, *cursor_y, COL8_000000, COL8_C6C6C6, s);
                *cursor_x += 8;
                if (*cursor_x >= sht->cursor_x_high) console_newline(sht, cursor_x, cursor_y);
        }
    }
    return;
}

void task_count_main(struct Sheet* sht_win_b) {
    char s[40];
    unsigned int cnt = 0;
    while (1)
    {
        ++cnt;
//        io_cli();
        sprintf(s, "%010u", cnt);
        putfonts8_asc_sht(sht_win_b, 0, 16, COL8_FFFFFF, COL8_000000, s);
        //        io_sti();
    }
    
}

void task_count_kill(struct MemMan* man, struct Task* task, struct Sheet* sht) {
    task_free(task);
    sheet_free(sht);
    memman_free_4kB(man, sht->buf, sht->bxsize * sht->bysize);
    task = NULL;
    sht = NULL;
    return;
}

void task_count_create() {
    return;
}

void console_init(struct SheetControl* shtctl, struct MemMan* man, struct Sheet** consht,
        struct Task** contask, struct Task** task_b, struct Sheet** sht_b, int task_b_num) {
    
    conctl = (struct ConsoleControl*) memman_alloc_4kB(man, sizeof(struct ConsoleControl));
    conctl->consht = consht;
    conctl->contask = contask;
    conctl->shtctl = shtctl;
    conctl->man = man;
    conctl->task_b_num = task_b_num;
    conctl->bshts = sht_b;
    conctl->btasks = task_b;
    conctl->fileinfo = (struct FileInfo*) (ADR_DISKIMG + 0x002600);
    conctl->gdt = (struct SEGMENT_DESCRIPTOR*) ADR_GDT;
    *consht = sheet_alloc(shtctl);
    make_window(man, *consht, 280, 220, COL8_C6C6C6, -1, "console", 1);
    *contask = task_alloc();
    (*contask)->tss.esp = memman_alloc_4kB(man, 64 * 1024) + 64 * 1024;
    (*contask)->tss.eip = (int) &console_task;
    (*contask)->tss.es = 1 * 8;
    (*contask)->tss.cs = 2 * 8;
    (*contask)->tss.ss = 1 * 8;
    (*contask)->tss.ds = 1 * 8;
    (*contask)->tss.fs = 1 * 8;
    (*contask)->tss.gs = 1 * 8;
    task_run(*contask, 2, 2);
    return;
}

int judge_command(unsigned char* s) {
    const static int cmd_num = 4;
    static unsigned char cmds[4][10] = {
            {"free"}, {"clear"}, {"ls"}, {"hlt"}
    };
    int i;
    for (i = 0; i < cmd_num; ++i)
    {
        if (strcmp(s, cmds[i]) == 0) return i;
    }
    if (strncmp(s, "cat ", 4) == 0) return 100;
    if (strncmp(s, "kill ", 5) == 0)
    {
        if (s[5] >= '0' && s[5] < '4' && s[6] == '\0') return 101;
    }
    return -1;
}

void console_task() {
    struct Sheet* sht = *(conctl->consht);
    struct Task* task = *(conctl->contask);
    struct Sheet* task_b = *(conctl->btasks);
    struct Sheet* sht_b = *(conctl->bshts);
    struct Timer* timer;
    struct MemMan* man = conctl->man;
    struct FileInfo* fileinfo = conctl->fileinfo;
    struct SEGMENT_DESCRIPTOR* gdt = conctl->gdt;
    int fifobuf[128], data, cursor_x = sht->cursor_x_low, cursor_c = -1, cursor_y = sht->cursor_y_low;
    unsigned char s[40], cmdline[40];
    int cmd_judge_flag = -1;
    int *fat = (int *) memman_alloc_4kB(man, 2 * 2880); // 2880 = 2个磁头（上下） * 18个扇区 * 80个柱面； 4 = 2个fat表 * 扇区号用2个字节存储
    file_unzip_fat(fat, (unsigned char*)(ADR_DISKIMG + 0x200)); // 第一个fat表的位置在0x200
    
    
    fifo32_init(&task->fifo, 128, fifobuf, task);
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 0.35 * TIMER_COUNT_PER_SECOND);
    putfonts8_asc_sht(sht, cursor_x, cursor_y, COL8_000000, COL8_C6C6C6, "$ ");
    cursor_x += 16;
    sht->cursor_x_low = 16;
    
    unsigned int total_mem = memtest(0x00400000, 0xbfffffff) / (1024 * 1024);
    
    while (1)
    {
        io_cli();
        if (fifo32_status(&task->fifo) == 0)
        {
            task_sleep(task);
            io_stihlt();
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
                    if (cmd_judge_flag == 0) // free
                    {
                        // free command
                        sprintf(s, "Total: %dMB, Free: %dKB", total_mem, memman_total(man) / 1024);
                        putfonts8_asc_sht(sht, cursor_x, cursor_y, COL8_000000, COL8_C6C6C6, s);
                        console_newline(sht, &cursor_x, &cursor_y);
                    }
                    else if (cmd_judge_flag == 1) // clear
                    {
                        boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, 0, sht->cursor_y_low,
                                 sht->cursor_x_high+7, sht->cursor_y_high+LINE_GAP-1);
                        sheet_refresh(sht, 0, sht->cursor_y_low,
                                      sht->cursor_x_high+8, sht->cursor_y_high+LINE_GAP);
                        cursor_x = sht->cursor_x_low;
                        cursor_y = sht->cursor_y_low;
                    }
                    else if (cmd_judge_flag == 2) // ls
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
                    else if (cmd_judge_flag == 3) // hlt
                    {
                        int f = file_locate(fileinfo, "HLT     OBJ");
                        if (f == -1) putfonts8_asc_sht(sht, cursor_x, cursor_y, COL8_000000, COL8_C6C6C6, "File Not Found");
                        else
                        {
                            unsigned char* buf = (unsigned char*) memman_alloc_4kB(man, fileinfo[f].size);
                            file_loadfile(fileinfo[f].cluster_id, fileinfo[f].size, buf, fat, (unsigned char*) (0x003e00 + ADR_DISKIMG));
                            set_segmdesc(gdt + 1003, fileinfo[f].size - 1, (int) buf, AR_CODE32_ER);
                            far_jmp(0, 1003 * 8);
                            memman_free_4kB(man, (unsigned int)buf, fileinfo[f].size);
                        }
                        console_newline(sht, &cursor_x, &cursor_y);
                    }
                    else if (cmd_judge_flag == 100) // cat
                    {
                        unsigned char filename[11];
                        int t1, t2, t3;
                        t2 = 0;
                        for (t1 = 0; t1 < 11; ++t1) filename[t1] = ' ';
                        for (t1 = 4; t1 < strlen(cmdline); ++t1)
                        {
                            if (cmdline[t1] >= 'a' && cmdline[t1] <= 'z') cmdline[t1] -= 0x20;
                            if (cmdline[t1] == '.')
                            {
                                t2 = 8;
                                continue;
                            }
                            filename[t2++] = cmdline[t1];
                        }
                        t3 = file_locate(fileinfo, filename);
                        if (t3 == -1) // not found
                        {
                            putfonts8_asc_sht(sht, cursor_x, cursor_y, COL8_000000, COL8_C6C6C6, "File Not Found");
                        }
                        else // file exists
                        {
                            unsigned char* buf = (unsigned char*) memman_alloc_4kB(man, fileinfo[t3].size);
                            file_loadfile(fileinfo[t3].cluster_id, fileinfo[t3].size, buf,
                                    fat, (unsigned char*) (0x003e00 + ADR_DISKIMG));
                            console_printfile(sht, buf, fileinfo[t3].size, &cursor_x, &cursor_y);
                            memman_free_4kB(man, (unsigned int) buf, fileinfo[t3].size);
                        }
                        console_newline(sht, &cursor_x, &cursor_y);
                    }
                    else if (cmd_judge_flag == 101) // kill
                    {
                        int b = cmdline[5] - '0';
                        task_count_kill(man, &task_b[b], &sht_b[b]);
                        sprintf(s, "task_b %d has been killed", b);
                        putfonts8_asc_sht(sht, cursor_x, cursor_y, COL8_000000, COL8_C6C6C6, s);
                        console_newline(sht, &cursor_x, &cursor_y);
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
            else if (data == 4) // tab to main
            {
                cursor_c = -1;
                boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, cursor_x, 16, cursor_x + 7, 16 + 15);
                sheet_refresh(sht, cursor_x, 16, cursor_x + 8, 16 + 16);
            }
            else if (data == 3) // tab to console
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



