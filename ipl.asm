; NoobOS
CYLS EQU 10




ORG 0x7c00 ;指定程序装载的位置
;下面用于描述FAT12个格式的软盘
JMP entry
DB 0x90
DB "IPL45678" ;启动区的名称可以是任意的字符串，但长度必须是8字节
DW 512; 每一个扇区的大小，必须是512字节
DB 1 ;簇的大小（必须为1个扇区)
DW 1 ;FAT的起始位置（一般从第一个扇区开始）
DB 2 ;FAT的个数 必须是2
DW 224;根目录的大小 一般是224项
DW 2880; 该磁盘的大小 必须是2880扇区
DB 0xf0;磁盘的种类 必须是0xf0
DW 9;FAT的长度 必须是9扇区
DW 18;1个磁道(track) 有几个扇区 必须是18
DW 2; 磁头个数 必须是2
DD 0; 不使用分区，必须是0
DD 2880; 重写一次磁盘大小
DB 0,0,0x29 ;扩展引导标记 固定0x29
DD 0xffffffff ;卷列序号
DB "NoobOS789AB" ;磁盘的名称（11个字节）
DB "FAT12   " ;磁盘的格式名称（8字节）
TIMES 18 DB 0; 先空出18字节 这里与原文写法不同
;程序核心

entry:

    MOV	AX, 0			; 初始化寄存器
    MOV	SS, AX
    MOV	SP, 0x7c00 
    MOV	DS, AX ;实际上每次寻址都是DS * 16 + 指定地址，例如[AX] = [DS:AX]
    
    ;读盘内存最开始加载是在0x8000，只不过0x8000-0x81ff是最初的512字节留给启动区ipl
    MOV AX, 0x0820
    MOV ES, AX
    MOV CH, 0 ;柱面0
    MOV DH, 0 ;磁头0
    MOV CL, 2 ;扇区2，扇区是从1开始的，1-18，1个sector 512B，ipl程序位于第一个扇区，我们想要加载next即第二个sector

readloop:
    MOV SI, 0 ;记录读盘失败次数

retry:
    MOV AH, 0x02 ;读盘
    MOV AL, 1 ;读一个扇区
    MOV BX, 0 
    MOV DL, 0x00 ;A驱动器
    INT 0x13 ;BIOS INT13 AH=0x02 表示读盘
    JNC next ;没出错jump if not carry flag, flag != 1
    ADD SI, 1
    CMP SI, 5
    JAE error; jump if above or equal si >= 5
    ;重置驱动器
    MOV AH, 0x00
    MOV DL, 0x00
    INT 0x13
    JMP retry

next:
    MOV AX, ES
    ADD AX, 0x0020 
    MOV ES, AX 
    ;相当于给ES+0x0020，因为真正地址是ES * 16 + BX，
    ;0x0020 * 16 = 0x0200 即 512，一个扇区512字节，因此我们缓存地址也要前移动
    ADD CL, 1
    CMP CL, 18
    JBE readloop ;jump if below or equal cl <= 18
    ;这里抛出一个疑问，就是用循环方式一个一个读取sector，也可以在cl=2时，指定al=17，直接从2-18共17个sectors全部读入
    ;作者指出这样有问题，因此采用循环方式
    
    ;柱面0磁头0扇区18的下一个是柱面0磁头1扇区1
    MOV CL, 1 ;从扇区1开始读
    ADD DH, 1 ;换磁头
    CMP DH, 2
    JB readloop ;jump if below dh < 2
    MOV DH, 0 ;磁头0
    ADD CH, 1 ;换下一个柱面
    CMP CH, CYLS
    JB readloop ;读cyls个柱面，cyls在开头已经宏定义

    MOV [0x0ff0], CH ;记录ipl读了多少柱面
    JMP 0xc200


error:
    MOV SI, msg

putloop:
    MOV AL, [SI]
    ADD SI, 1
    CMP AL, 0
    JE fin
    MOV AH, 0x0e
    MOV BX, 15
    INT 0x10
    JMP putloop

fin:
    HLT
    JMP fin

msg:
    DW 0x0a0a
    DB "load failed"
    DW 0x0a0a
    DB 0
    TIMES 510 - ($-$$) DB 0
    DW 0xaa55
    ;
    DB	0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
    RESB 4600
    DB	0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
    RESB 1469432