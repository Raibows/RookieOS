/*asmhead.asm*/
struct BootInfo {
	char cyls, leds, vmode, reserve;
	short scrn_x, scrn_y;
	char* vram;
};
#define ADR_BOOTINFO 0x00000ff0


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
int load_cr0(void);
void store_cr0(int cr0);
unsigned int memtest_sub(unsigned int start, unsigned int end);

/*graphic.c*/
int check_pos(int x, int low, int high);
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(char* vram, int XSIZE, int YSIZE, unsigned char color, int x0, int y0, int x1, int y1);
void init_screen(char* vram, int XSIZE, int YSIZE);
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
#define AR_INTGATE32 0x008e


/*fifo.c*/
struct FIFO8 {
    unsigned char* buf;
    int w, r, size, free, flags;
};
void fifo8_init(struct FIFO8* fifo, int size, unsigned char* buf);
int fifo8_put(struct FIFO8* fifo, unsigned char data);
int fifo8_get(struct FIFO8* fifo);
int fifo8_status(struct FIFO8* fifo);



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
void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC* mdec);
int mouse_decode(struct MOUSE_DEC* mdec, unsigned char data);

#define PORT_KEYDAT	0x0060
#define PORT_KEYSTA	0x0064
#define PORT_KEYCMD	0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47
#define KEYCMD_SENDTO_MOUSE	0xd4
#define MOUSECMD_ENABLE	0xf4



















