struct BootInfo;
struct Sheet;
struct SEGMENT_DESCRIPTOR;
struct GATE_DESCRIPTOR;
//struct FIFO8;
struct FIFO32;
struct MOUSE_DEC;
struct FreeInfo;
struct MemMan;
struct Sheet;
struct SheetControl;
struct Timer;
struct TimerControl;
struct TSS32;
struct Task;
struct TaskControl;
struct TaskLevel;
struct FileInfo;


#define NULL ((void *)0)

/* debug.c */
void debug_print(char* info);
void debug_init(struct Sheet* sht_back);
#define assert(x, info) \
    if (!(x))           \
    {                   \
        debug_print(info);    \
    }
    
/*asmhead.asm*/
struct BootInfo {
    /*
     * 0x0ff0-0x0fff
     */
	char cyls, leds, vmode, reserve;
	short scrn_x, scrn_y;
	char* vram;
};

struct FileInfo {
    /*
     * name为文件名
     * ext为后缀
     * type为文件类型
     * cluster_id为文件开始的簇号
     */
    unsigned char name[8], ext[3], type;
    char reserve[10];
    unsigned short time, date, cluster_id;
    unsigned int size;
};

#define ADR_BOOTINFO 0x00000ff0
#define ADR_DISKIMG 0X00100000


/*naskfunc.asm*/
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void write_mem8(int addr, int data);
void asm_int_handler21(void);
void asm_int_handler27(void);
void asm_int_handler2c(void);
void asm_int_handler20(void);
int load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);
void far_jmp(int eip, int cs);
unsigned int memtest_sub(unsigned int start, unsigned int end);

/*graphic.c*/
#define LINE_GAP 20 // 一行文字占少y轴空间
int check_pos(int x, int low, int high);
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(char* vram, int XSIZE, unsigned char color, int x0, int y0, int x1, int y1);
void init_screen(struct Sheet* sht, int XSIZE, int YSIZE);
void putfont8(char* vram, int XSIZE, int x, int y, char color, char* font);
void putfonts8_asc(char* vram, int XSIZE, int x, int y, char color, unsigned char* fonts);
void init_mouse_cursor8(char* mouse, char bc);
void putblock8_8(char* vram, int XSIZE, int pxsize, int pysize, int px0, int py0, char* buf, int bxsize);


#define COL8_000000 0   //黑 
#define COL8_FF0000 1   //梁红 
#define COL8_00FF00 2   //亮绿 
#define COL8_FFFF00 3   //亮黄 
#define COL8_0000FF 4   //亮蓝 
#define COL8_FF00FF 5   //亮紫 
#define COL8_00FFFF 6   //浅亮蓝
#define COL8_FFFFFF 7   //白 
#define COL8_C6C6C6 8   //亮灰 
#define COL8_840000 9   //暗红 
#define COL8_008400 10  //暗绿 
#define COL8_848400 11  //暗黄 
#define COL8_000084 12  //暗青 
#define COL8_840084 13  //暗紫 
#define COL8_008484 14  //浅暗蓝
#define COL8_848484 15  //暗灰   


/* dsctbl.c */
//descriptor table
struct SEGMENT_DESCRIPTOR {
	/*
	段描述符记录段的起始地址，段的大小，段的属性（如访问权限）
	注意段寄存器只有8bytes
	1.base就是基址，但是分成mid，low是为了兼容80286
	2.limit是指段的大小，一个段最大是4GB，需要32位表示，即4字节
	但容量有限，只好用另外一种方法，limit_low+limit_high=3bytes=24位，高4位表示段属性，低20位表示段大小；
	其中有一个标志位，表示采用page计量，计算机中一个page=4kB，4kB * 2^20 =4GB正好
	3.段的属性，12位，limit_high高4位+access_right8位=12位
	因此总的ar=12位，高4位表示扩展访问权限，GD00，G代表page，D代表使用16位模式或32位模式
	低8位表示一些权限控制，可查表
	*/
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
	// 8 bytes
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};


void set_segmdesc(struct SEGMENT_DESCRIPTOR* sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR* gd, int offset, int selector, int ar);
void init_gdt_idt(void);
#define ADR_IDT	0x0026f800
#define LIMIT_IDT 0x000007ff
#define ADR_GDT	0x00270000
#define LIMIT_GDT 0x0000ffff
#define ADR_BOTPAK 0x00280000
#define LIMIT_BOTPAK 0x0007ffff
#define AR_DATA32_RW 0x4092
#define AR_CODE32_ER 0x409a
#define AR_TSS32 0x0089
#define AR_INTGATE32 0x008e


/*fifo.c*/
//struct FIFO8 {
//    unsigned char* buf;
//    int w, r, size, free, flags;
//};
struct FIFO32 {
    /*
     * buf用作缓冲区数组
     * w为下一个写入
     * r为下一个读（循环读写）
     * size - 写入尚未读取的数据 = free
     * task用来唤醒任务，当向fifo写数据时，自动唤醒关联task，可设置为NULL
     */
    int* buf;
    int w, r, size, free, flags;
    struct Task* task;
};
//void fifo8_init(struct FIFO8* fifo, int size, unsigned char* buf);
//int fifo8_put(struct FIFO8* fifo, unsigned char data);
//int fifo8_get(struct FIFO8* fifo);
//int fifo8_status(struct FIFO8* fifo);
void fifo32_init(struct FIFO32* fifo, int size, int* buf, struct Task* task);
int fifo32_put(struct FIFO32* fifo, int data);
int fifo32_get(struct FIFO32* fifo);
int fifo32_status(struct FIFO32* fifo);


/*int.c*/
void init_pic(void);
void int_handler27(int *esp);
#define PIC0_ICW1 0x0020
#define PIC0_OCW2 0x0020
#define PIC0_IMR 0x0021
#define PIC0_ICW2 0x0021
#define PIC0_ICW3 0x0021
#define PIC0_ICW4 0x0021
#define PIC1_ICW1 0x00a0
#define PIC1_OCW2 0x00a0
#define PIC1_IMR 0x00a1
#define PIC1_ICW2 0x00a1
#define PIC1_ICW3 0x00a1
#define PIC1_ICW4 0x00a1



/*keyboard and mouse*/
struct MOUSE_DEC{
    unsigned char buf[3], phase;
    int x, y, btn;
};
void int_handler21(int* esp);
void int_handler2c(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32* fifo, int data);
void enable_mouse(struct FIFO32* fifo, int data, struct MOUSE_DEC* mdec);
int mouse_decode(struct MOUSE_DEC* mdec, unsigned char data);

#define PORT_KEYDAT	0x0060
#define PORT_KEYSTA	0x0064
#define PORT_KEYCMD	0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47
#define KEYCMD_SENDTO_MOUSE	0xd4
#define MOUSECMD_ENABLE	0xf4




/*memory.c*/
#define ADR_MEMMAN 0x003c0000
#define MEMMAN_FREES 4090
#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

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

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MemMan* man);
unsigned int memman_total(struct MemMan* man);
unsigned int memman_alloc(struct MemMan* man, unsigned int size);
int memman_free(struct MemMan* man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4kB(struct MemMan* man, unsigned int size);
int memman_free_4kB(struct MemMan* man, unsigned int addr, unsigned int size);


/*sheet.c*/
#define MAX_SHEETS 256
#define SHEET_USE 1

struct Sheet {
    /*
     * buf为该图层内容的地址
     * col_inv透明色号
     * height表示图层高度, -1为隐藏状态
     * cursor_x记录光标极限 (默认make_window的情况下）
     * cursor_y记录光标极限
     */
    unsigned char* buf;
    int bxsize, bysize, vx, vy, col_inv, flags, height;
    struct SheetControl* ctl;
    int cursor_x_low, cursor_y_low;
    int cursor_x_high, cursor_y_high;
};

struct SheetControl {
    /*
     * top表示最上层图层高度
     * sheets指针用来访问堆叠起来的图层，高度从小到大
     * sheets_pool存储真正的buf内容，可以被释放、分配，作为图层pool
     * map相当于一个地图，他记录vram上面的点属于哪一个sheet
     */
    unsigned char* vram;
    unsigned char* map;
    int xsize, ysize, top;
    struct Sheet* sheets[MAX_SHEETS];
    struct Sheet sheets_pool[MAX_SHEETS];
};
struct SheetControl* sheetcontroll_init(struct MemMan* man, unsigned char* vram, int xsize, int ysize);
struct Sheet* sheet_alloc(struct SheetControl* ctl);
void sheet_setbuf(struct Sheet* sht, unsigned char* buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct Sheet* sht, int height);
void sheet_refreshall(struct SheetControl* ctl);
void sheet_slide(struct Sheet* sht, int vx0, int vy0);
void sheet_free(struct Sheet* sht);
void sheet_refresh(struct Sheet* sht, int bx0, int by0, int bx1, int by1);
void sheet_refreshsub(struct SheetControl* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);



/*timer.c*/
#define MAX_TIMER 500
#define TIMER_COUNT_PER_SECOND 100

struct Timer {
    /*
     * timeout，用来倒计时，当为0时，向fifo发送数据
     * flags 记录定时器状态
     */
    struct Timer* next;
    unsigned int timeout;
    struct FIFO32* fifo;
    unsigned char flags;
    int data;
};

struct TimerControl {
    /*
     * count用来计数中断次数，每100=1秒，因为每秒100次中断
     * next_expire_time用来记录下一个到期的时刻
     * guard_timer是链表中的哨兵
     */
    unsigned int count, next_expire_time;
    struct Timer* guard_timer;
    struct Timer timers[MAX_TIMER];
};

void init_pit(void);
void int_handler20(int* esp);
struct Timer* timer_alloc(void);
void timer_free(struct Timer* timer);
void timer_init(struct Timer* timer, struct FIFO32* fifo, int data);
void timer_settime(struct Timer* timer, unsigned int timeout);


/* multitask.c */

#define MAX_TASKS 1000 // 最大任务数量
#define MAX_LEVEL_TASKS 100 // 单个level的最大任务数量
#define MAX_LEVELS 10 // task level num
#define TASK_START_GDT 3 // 定义从GDT的几号开始分配给TSS


struct TSS32 {
    /*
     * task status segment
     * 第一行表示的信息是这些寄存器的状态（会不会被写入啊等等）
     * 第二行存储这些32位寄存器的信息
     * 第三行为16位寄存器
     *
     */
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    int es, cs, ss, ds, fs, gs;
    int ldtr, iomap;
};

struct Task {
    /*
     * gdt_id记录任务的GDT编号
     * priority为优先级倍数，基础为1 * 0.01
     * stack记录段结束地址（低地址），方便回收
     */
    int gdt_id, priority, level;
    unsigned char flags;
    unsigned int stack;
    struct TSS32 tss;
    struct FIFO32 fifo;
};

struct TaskLevel {
    /*
     * running_num记录正在运行的任务数量
     * now_task记录当前运行的任务
     */
    int running_num;
    int now_task;
    struct Task* tasks[MAX_LEVEL_TASKS];
};


struct TaskControl {
    /*
     * now_lv：正在运行的task_level
     * is_lv_change：是否需要切换下一个level
     */
    int now_lv;
    char is_lv_change;
    struct TaskLevel level[MAX_LEVELS];
    struct Task pool[MAX_TASKS];
};

struct Task* task_init(struct MemMan* memman);
struct Task* task_alloc(void);
void task_run(struct Task* task, int level, int priority);
void task_switch(void);
void task_sleep(struct Task* task);
struct Task* task_now(void);
void task_add(struct Task* task);
void task_remove(struct Task* task);
void task_switch_level(void);
void task_idle(void);
void task_free(struct Task* task);

/* file.c */
void file_unzip_fat(int* fat, unsigned char* img);
void file_loadfile(int cluster_id, int size, char* buf, int* fat, unsigned char* img);
int file_locate(struct FileInfo* fileinfo, unsigned char* filename);


/* window.c */
void putfonts8_asc_sht(struct Sheet* sht, int x, int y, int c, int bc, char* s);
void make_window(struct MemMan* man, struct Sheet* sht, int xsize, int ysize, int color, int col_inv, char* title, char is_act);
void make_title(struct Sheet* sht, char* title, char is_act, char is_close_btn);


/* console.c */
struct ConsoleControl {
    struct Task** contask;
    struct Sheet** consht;
    struct SheetControl* shtctl;
    int task_b_num;
    struct Sheet** bshts;
    struct Task** btasks;
    struct MemMan* man;
    struct FileInfo* fileinfo;
    struct SEGMENT_DESCRIPTOR* gdt;
    int cur_x, cur_y, cur_c;
};
void console_init(struct SheetControl* shtctl, struct MemMan* man, struct Sheet** consht,
                  struct Task** contask, struct Task** task_b, struct Sheet** sht_b, int task_b_num);
//void console_task(struct Sheet* sht, unsigned int total_mem, struct Task* task_b, struct Sheet* sht_b);
void console_task(void);
void task_count_main(struct Sheet* sht_win_b);

















