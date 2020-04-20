; Raibowsos
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
DB "CHERRY-OS  " ;磁盘的名称（11个字节）
DB "FAT12   " ;磁盘的格式名称（8字节）
TIMES 18 DB 0; 先空出18字节 这里与原文写法不同
;程序核心

entry:
    MOV AX, 0
    MOV SP, 0x7c00
    MOV SS, AX
    MOV DS, AX
    MOV ES, AX
    MOV BX, 10
    MOV SI, msg

putloop:
    MOV AL, [SI]
    ADD SI, 1
    CMP AL, 0
    JE fin
    MOV AH, 0x0e
    INT 0x10
    JMP putloop

msg:
    DW 0x0a0a
    DB "hello, Raibowsos"
    DW 0x0a0a
    TIMES 510 - ($-$$) DB 0
    DW 0xaa55

fin:
    HLT
    JMP fin
    