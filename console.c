#include <stdio.h>
#include <string.h>
#include "bootpack.h"
#include "tools.h"


struct ConsoleControl* conctl;
unsigned char s[40];
int *fat;


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

void console_newline() {
    if (conctl->cur_y + LINE_GAP <= (*conctl->consht)->cursor_y_high)
    {
        conctl->cur_y += LINE_GAP;
    }
    else
    {
        parallel_move(*conctl->consht, LINE_GAP, COL8_C6C6C6, 0, (*conctl->consht)->cursor_y_low,
                      (*conctl->consht)->cursor_x_high+7, (*conctl->consht)->cursor_y_high+LINE_GAP-1);
    }
    conctl->cur_x = (*conctl->consht)->cursor_x_low;
    return;
}

void console_print(unsigned char* info) {
    putfonts8_asc_sht(*conctl->consht, conctl->cur_x, conctl->cur_y, COL8_000000, COL8_C6C6C6, info);
    return;
}

void console_printfile(char* buf, int size) {
    /*
     * sht是console窗口
     * buf为已经从file中读取好的内容
     * 注意size大小是实际文件大小，而不是buf大小（buf可能因4kB补齐而比实际大）
     */
    struct Sheet* sht = *conctl->consht;
    unsigned int i;
    unsigned char ss[2];
    ss[1] = '\0';
    for (i = 0; i < size; ++i)
    {
        ss[0] = buf[i];
        int t;
        switch (ss[0])
        {
            case 0x09: // \t
                for (t = 0; t < 4; ++t)
                {
                    console_print(" ");
                    conctl->cur_x += 8;
                    if (conctl->cur_x >= sht->cursor_x_high)
                    {
                        console_newline();
                        break;
                    }
                }
                break;
            case 0x0a: // \n
                console_newline();
                break;
            case 0x0d: break; // \r
            default:
                console_print(ss);
                conctl->cur_x += 8;
                if (conctl->cur_x >= sht->cursor_x_high) console_newline();
        }
    }
    return;
}

void task_count_main(struct Sheet* sht_win_b) {
    char ss[40];
    unsigned int cnt = 0;
    const int center = sht_win_b->bxsize / 2 - 40;
    while (1)
    {
        ++cnt;
        sprintf(ss, "%010u", cnt);
        putfonts8_asc_sht(sht_win_b, center, 16, COL8_FFFFFF, COL8_000000, ss);
    }
    
}

void task_count_kill(int bid) {
    task_free(conctl->btasks[bid]);
    sheet_free(conctl->bshts[bid]);
    memman_free_4kB(conctl->man, (unsigned int) conctl->bshts[bid]->buf, conctl->bshts[bid]->bxsize * conctl->bshts[bid]->bysize);
    memman_free_4kB(conctl->man, conctl->btasks[bid]->stack, 64 * 1024);
    conctl->btasks[bid] = NULL;
    conctl->bshts[bid] = NULL;
    return;
}

void task_count_create(int priority) {
    int i;
    char ss[20];
    for (i = 0; i < conctl->task_b_num; ++i)
    {
        if (conctl->btasks[i] == NULL) break;
    }
    conctl->btasks[i] = task_alloc();
    conctl->bshts[i] = sheet_alloc(conctl->shtctl);
    conctl->btasks[i]->stack = memman_alloc_4kB(conctl->man, 64 * 1024);
    conctl->btasks[i]->tss.esp = conctl->btasks[i]->stack + 64 * 1024 - 8;
    conctl->btasks[i]->tss.eip = (int) &task_count_main;
    conctl->btasks[i]->tss.es = 1 * 8;
    conctl->btasks[i]->tss.cs = 2 * 8;
    conctl->btasks[i]->tss.ss = 1 * 8;
    conctl->btasks[i]->tss.ds = 1 * 8;
    conctl->btasks[i]->tss.fs = 1 * 8;
    conctl->btasks[i]->tss.gs = 1 * 8;
    *((int *) (conctl->btasks[i]->tss.esp + 4)) = (int) (conctl->bshts[i]);
    sprintf(ss, "task_count %d", i);
    make_window(conctl->man, conctl->bshts[i], 160, -1, COL8_C6C6C6, -1, ss, 0);
    sheet_slide(conctl->bshts[i], 50, 100+50*i);
    sheet_updown(conctl->bshts[i], 1);
    task_run(conctl->btasks[i], 3, priority);
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
    (*contask)->stack = memman_alloc_4kB(man, 64 * 1024);
    (*contask)->tss.esp = (*contask)->stack + 64 * 1024;
    (*contask)->tss.eip = (int) &console_task;
    (*contask)->tss.es = 1 * 8;
    (*contask)->tss.cs = 2 * 8;
    (*contask)->tss.ss = 1 * 8;
    (*contask)->tss.ds = 1 * 8;
    (*contask)->tss.fs = 1 * 8;
    (*contask)->tss.gs = 1 * 8;
    conctl->cur_x = (*conctl->consht)->cursor_x_low;
    conctl->cur_y = (*conctl->consht)->cursor_y_low;
    conctl->cur_c = -1;
    task_run(contask[0], 2, 5);
    return;
}

int judge_command(unsigned char* ss) {
    const static int cmd_num = 4;
    static unsigned char cmds[4][10] = {
            {"free"}, {"clear"}, {"ls"}, {"hlt"}
    };
    int i;
    for (i = 0; i < cmd_num; ++i)
    {
        if (strcmp(ss, cmds[i]) == 0) return i;
    }
    if (strncmp(ss, "cat ", 4) == 0) return 100;
    if (strncmp(ss, "kill ", 5) == 0)
    {
        if (ss[5] >= '0' && ss[5] <= '9' && ss[6] == '\0') return 101;
    }
    if (strncmp(ss, "run ", 4) == 0)
    {
        // run priority
        i = tools_str2int(ss, 4, -1);
        if (i >= 1 && i <= 10) return 102;
    }
    return -1;
}

void cmd_free() {
    static unsigned int total_mem = -1;
    if (total_mem == -1) total_mem = memtest(0x00400000, 0xbfffffff) / (1024 * 1024);
    sprintf(s, "Total: %dMB, Free: %dKB", total_mem, memman_total(conctl->man) / 1024);
    console_print(s);
    console_newline();
    return;
}

void cmd_clear() {
    boxfill8((*conctl->consht)->buf, (*conctl->consht)->bxsize, COL8_C6C6C6,
            0, (*conctl->consht)->cursor_y_low,
            (*conctl->consht)->cursor_x_high+7, conctl->cur_y+LINE_GAP-1);
    
    sheet_refresh((*conctl->consht),
            0, (*conctl->consht)->cursor_y_low,
            (*conctl->consht)->cursor_x_high+8, conctl->cur_y+LINE_GAP);
    
    conctl->cur_x = (*conctl->consht)->cursor_x_low;
    conctl->cur_y = (*conctl->consht)->cursor_y_low;
    return;
}

void cmd_list() {
    static int t, t2;
    for (t = 0; t < 224; ++t)
    {
        
        if (conctl->fileinfo[t].name[0] == 0x00) break; // 已到头
        if (conctl->fileinfo[t].name[0] != 0xe5) // 0xe5代表被删除
        {
            if ((conctl->fileinfo[t].type & 0x18) == 0)
            {
                sprintf(s, "01234567.9AB  %07d B", conctl->fileinfo[t].size);
                
                for (t2 = 0; t2 < 8; ++t2) s[t2] = conctl->fileinfo[t].name[t2];
                s[9] = conctl->fileinfo[t].ext[0];
                s[10] = conctl->fileinfo[t].ext[1];
                s[11] = conctl->fileinfo[t].ext[2];
                if (s[9] == ' ') s[8] = ' ';
            }
            console_print(s);
            console_newline();
        }
    }
    return;
}

void cmd_hlt() {
    int f = file_locate(conctl->fileinfo, "HLT     OBJ");
    if (f == -1) console_print("File Not Found");
    else
    {
        unsigned char* buf = (unsigned char*) memman_alloc_4kB(conctl->man, conctl->fileinfo[f].size);
        file_loadfile(conctl->fileinfo[f].cluster_id, conctl->fileinfo[f].size, buf, fat, (unsigned char*) (0x003e00 + ADR_DISKIMG));
        set_segmdesc(conctl->gdt + 1003, conctl->fileinfo[f].size - 1, (int) buf, AR_CODE32_ER);
        far_jmp(0, 1003 * 8);
        memman_free_4kB(conctl->man, (unsigned int)buf, conctl->fileinfo[f].size);
    }
    console_newline();
    return;
}

void cmd_cat(unsigned char* cmdline) {
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
    t3 = file_locate(conctl->fileinfo, filename);
    if (t3 == -1) // not found
    {
        console_print("File Not Found");
    }
    else // file exists
    {
        unsigned char* buf = (unsigned char*) memman_alloc_4kB(conctl->man, conctl->fileinfo[t3].size);
        file_loadfile(conctl->fileinfo[t3].cluster_id, conctl->fileinfo[t3].size, buf,
                      fat, (unsigned char*) (0x003e00 + ADR_DISKIMG));
        console_printfile(buf, conctl->fileinfo[t3].size);
        memman_free_4kB(conctl->man, (unsigned int) buf, conctl->fileinfo[t3].size);
    }
    console_newline();
    return;
}

void cmd_kill(unsigned char* cmdline) {
    int b = tools_str2int(cmdline, 5, -1);
    if (b == -1)
    {
        console_print("Unknown Command");
        console_newline();
        return;
    }
    else if (conctl->btasks[b] == NULL)
    {
        sprintf(s, "task_b %d is not running", b);
        console_print(s);
        console_newline();
        return;
    }
    task_count_kill(b);
    sprintf(s, "task_b %d has been killed", b);
    console_print(s);
    console_newline();
    return;
}

void cmd_run(unsigned char* cmdline) {
    int prio = tools_str2int(cmdline, 4, -1);
    if (prio == -1)
    {
        console_print("Unknown Command");
        console_newline();
        return;
    }
    sprintf(s, "task_count priority %d is running", prio);
    console_print(s);
    task_count_create(prio);
    console_newline();
    return;
}

void cmd_runcmd(unsigned char* cmdline) {
    static int cmd_judge_flag = -1;
    cmd_judge_flag = judge_command(cmdline);
    
    if (cmd_judge_flag == 0) // free
    {
        cmd_free();
    }
    else if (cmd_judge_flag == 1) // clear
    {
        cmd_clear();
    }
    else if (cmd_judge_flag == 2) // ls
    {
        cmd_list();
    }
    else if (cmd_judge_flag == 3) // hlt
    {
        cmd_hlt();
    }
    else if (cmd_judge_flag == 100) // cat
    {
        cmd_cat(cmdline);
    }
    else if (cmd_judge_flag == 101) // kill
    {
        cmd_kill(cmdline);
    }
    else if (cmd_judge_flag == 102) // run
    {
        cmd_run(cmdline);
    }
    else
    {
        console_print("Unknown Command");
        console_newline();
    }
    return;
}

void console_task() {
    struct Sheet* sht = (*conctl->consht);
    struct Task* task = (*conctl->contask);
    struct Timer* timer;
    struct MemMan* man = conctl->man;
    struct FileInfo* fileinfo = conctl->fileinfo;
    int fifobuf[128], data;
    unsigned char cmdline[40];
    fat = (int *) memman_alloc_4kB(man, 2 * 2880); // 2880 = 2个磁头（上下） * 18个扇区 * 80个柱面； 4 = 2个fat表 * 扇区号用2个字节存储
    file_unzip_fat(fat, (unsigned char*)(ADR_DISKIMG + 0x200)); // 第一个fat表的位置在0x200
    
    
    fifo32_init(&task->fifo, 128, fifobuf, task);
    task->fifo.task = task;
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 0.35 * TIMER_COUNT_PER_SECOND);
    putfonts8_asc_sht(sht, conctl->cur_x, conctl->cur_y, COL8_000000, COL8_C6C6C6, "$ ");
    conctl->cur_x += 16;
    sht->cursor_x_low = 16;
    
    
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
                    if (conctl->cur_x - 8 >= sht->cursor_x_low)
                    {
                        boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, conctl->cur_x, conctl->cur_y, conctl->cur_x + 7, conctl->cur_y + 15); //将当前光标隐藏
                        sheet_refresh(sht, conctl->cur_x, conctl->cur_y, conctl->cur_x + 8, conctl->cur_y + 16);
                        conctl->cur_x -= 8;
                    }
                }
                else if (data == 10) // enter
                {
                    if (conctl->cur_x == sht->cursor_x_low) continue;
                    cmdline[(conctl->cur_x - sht->cursor_x_low) / 8] = '\0';
                    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, conctl->cur_x, conctl->cur_y, conctl->cur_x + 7, conctl->cur_y + 15); //将当前光标隐藏
                    sheet_refresh(sht, conctl->cur_x, conctl->cur_y, conctl->cur_x + 8, conctl->cur_y + 16);
                    console_newline();
                    cmd_runcmd(cmdline);
                    putfonts8_asc_sht(sht, 0, conctl->cur_y, COL8_000000, COL8_C6C6C6, "$ ");
                }
                else // 普通字符显示
                {
                    s[0] = data;
                    s[1] = '\0';
                    cmdline[(conctl->cur_x - sht->cursor_x_low) / 8] = data;
                    putfonts8_asc_sht(sht, conctl->cur_x, conctl->cur_y, COL8_000000, COL8_C6C6C6, s);
                    conctl->cur_x += 8;
                }
                conctl->cur_x = check_pos(conctl->cur_x, sht->cursor_x_low, sht->cursor_x_high);
                conctl->cur_y = check_pos(conctl->cur_y, sht->cursor_y_low, sht->cursor_y_high);
            }
            else if (data == 4) // tab to main
            {
                conctl->cur_c = -1;
                boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, conctl->cur_x, 16, conctl->cur_x + 7, 16 + 15);
                sheet_refresh(sht, conctl->cur_x, 16, conctl->cur_x + 8, 16 + 16);
            }
            else if (data == 3) // tab to console
            {
                conctl->cur_c = 0;
            }
            else // timer
            {
                if (conctl->cur_c < 0) // 此时不显示光标，因为焦点在别处
                {
                    timer_settime(timer, 0.35 * TIMER_COUNT_PER_SECOND);
                    continue;
                }
                switch (data)
                {
                    case 1:timer_init(timer, &task->fifo, 0);
                        conctl->cur_c = COL8_FFFFFF;
                        break;
                    case 0:timer_init(timer, &task->fifo, 1);
                        conctl->cur_c = COL8_C6C6C6;
                        break;
                }
                
                boxfill8(sht->buf, sht->bxsize, conctl->cur_c, conctl->cur_x, conctl->cur_y, conctl->cur_x + 7, conctl->cur_y + 15);
                sheet_refresh(sht, conctl->cur_x, conctl->cur_y, conctl->cur_x + 8, conctl->cur_y + 16);
                timer_settime(timer, 0.35 * TIMER_COUNT_PER_SECOND);
            }
        }
    }
}



