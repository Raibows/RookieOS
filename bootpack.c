#include <stdio.h>
#include <string.h>
#include "bootpack.h"

#define MEMMAN_ADDR 0x003c0000
#define MEMMAN_FREES 4090
#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000



unsigned int memtest(unsigned int start, unsigned int end) {
    char flag486 = 0; //only >= 486 use cpu cache
    unsigned int eflag, cr0, i;
    eflag = io_load_eflags();
    if ((eflag & EFLAGS_AC_BIT) != 0) flag486 = 1; // 如果是386，即使设定AC=1，AC的值还会自动回到0
    eflag &= ~EFLAGS_AC_BIT; // AC-bit = 0
    io_store_eflags(eflag);
    
    if (flag486 != 0) //486
    {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE; /* 禁止缓存 */
        store_cr0(cr0);
    }
    
    i = memtest_sub(start, end);
    
    if (flag486 != 0) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE; /* 允许缓存 */
        store_cr0(cr0);
    }
    
    return i;
}

struct FreeInfo {
    unsigned int addr, size;
};

struct MemMan {
    /*
     * frees: 可使用内存块
     * maxfrees： 观察到的最多内存块数量
     * lostsize： 累计释放失败的内存大小
     * losts： 释放失败次数
     */
    int frees, maxfrees, lostsize, losts;
    struct FreeInfo pool[MEMMAN_FREES];
};

void memman_init(struct MemMan* man) {
    man->frees = 0;
    man->maxfrees = 0;
    man->losts = 0;
    man->lostsize = 0;
    return;
}

unsigned int memman_total(struct MemMan* man) {
    /*
     * 返回可用内存大小
     */
    unsigned int i = 0, t=0;
    for (; i<man->frees; ++i) t += man->pool[i].size;
    return t;
}

unsigned int memman_alloc(struct MemMan* man, unsigned int size) {
    unsigned int i, a;
    for (i=0; i<man->frees; ++i)
    {
        if (man->pool[i].size >= size)
        {
            a = man->pool[i].addr;
            man->pool[i].addr += size;
            man->pool[i].size -= size;
            if (man->pool[i].size == 0)
            {
                --man->frees;
                for (; i<man->frees; ++i) man->pool[i] = man->pool[i+1];
            }
            return a;
        }
    }
    return 0;
}

int memman_free(struct MemMan* man, unsigned int addr, unsigned int size) {
    int i, j;
    for (i=0; i<man->frees; ++i)
    {
        if (man->pool[i].addr > addr) break;
    }
    // pool[i-1].addr < addr < pool[i].addr
    if (i > 0) // 前面有i-1
    {
        if (man->pool[i-1].addr + man->pool[i-1].size == addr)
        {
            man->pool[i-1].size += size;
            if (i < man->frees) // 后面也有i
            {
                if (addr + size == man->pool[i].size)
                {
                    man->pool[i-1].size += man->pool[i].size;
                    --man->frees;
                    for (; i<man->frees; ++i) man->pool[i] = man->pool[i+1];
                }
            }
            return 0; // success
        }
    }
    if (i < man->frees) // 不能与前面的空间连接
    {
        if (addr + size == man->pool[i].addr)
        {
            man->pool[i].addr = addr;
            man->pool[i].size += size;
            return 0;
        }
    }
    if (i < MEMMAN_FREES) // 既不能与前面，也不能与后面的连接，腾地方插入
    {
        for (j=man->frees; j>i; --j) man->pool[j] = man->pool[j-1];
        ++man->frees;
        man->maxfrees = man->maxfrees > man->frees ? man->maxfrees : man->frees;
        man->pool[i].addr = addr;
        man->pool[i].size = size;
        return 0;
    }
    
    // 也不能往后移动，说明碎片化严重，只能放弃了
    ++man->losts;
    man->lostsize += size;
    return -1; // failed
}






extern struct FIFO8 keyfifo, mousefifo;






void HariMain (void) {
    char* vram;
	char mycursor[16*16];
	unsigned char s[40], keybuf[32], mousebuf[128];
	struct BootInfo* binfo;
    struct MOUSE_DEC mdec;
    struct MemMan* man = (struct MemMan*) MEMMAN_ADDR;
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
    memman_init(man);
	
    unsigned int total_mem = memtest(0x00400000, 0xbfffffff);
    memman_free(man, 0x00001000, 0x0009e000);
    memman_free(man, 0x00400000, total_mem - 0x00400000);
    sprintf(s, "total: %u MB  free: %u KB block: %d",
            total_mem / 1024 / 1024, memman_total(man) / 1024, man->frees);
    boxfill8(vram, binfo->scrn_x, binfo->scrn_y, COL8_000000, 0, 32, strlen(s)*8-1, 47);
	putfonts8_asc(vram, binfo->scrn_x, 0, 32, COL8_FFFFFF, s);
    
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












































