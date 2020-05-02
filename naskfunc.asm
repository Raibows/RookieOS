; naskfunc
; TAB=4

[FORMAT "WCOFF"]        ; 制作目标文件的模式  
[BITS 32]            ; 制作32位模式用的机器语言
[INSTRSET "i486p"] ;说明使用32位寄存器，例如EAX
; 制作目标文件的信息
[FILE "naskfunc.asm"]      ; 源文件名信息

; 程序中包含的函数名，函数名前必须加_
GLOBAL _io_hlt, _io_cli, _io_sti, _io_stihlt     
GLOBAL _io_in8, _io_in16, _io_in32
GLOBAL _io_out8, _io_out16, _io_out32
GLOBAL _io_load_eflags, _io_store_eflags
GLOBAL _write_mem8
GLOBAL _load_gdtr, _load_idtr
    


; 以下是实际的函数

[SECTION .text]    ; 目标文件中写了这些后再写程序

_io_hlt:  ; void io_hlt(void);
    HLT
    RET

_io_cli:
    CLI ;clear interupt flags 屏蔽中断
    RET

_io_sti:
    STI ;set interupt flags 恢复中断
    RET

_io_stihlt:
    STI
    HLT
    RET

_io_in8:
    MOV EDX, [ESP+4] ;esp+4 = port
    MOV EAX, 0
    IN AL, DX
    RET

_io_in16:
    MOV EDX, [ESP+4]
    MOV EAX, 0
    IN AX, DX
    RET

_io_in32:
    MOV EDX, [ESP+4]
    IN EAX, DX

_io_out8: ;void io_out8(int port, int data)
    MOV EDX, [ESP+4]
    MOV AL, [ESP+8]
    OUT DX, AL
    RET

_io_out16:
    MOV EDX, [ESP+4]
    MOV EAX, [ESP+8] ;注意AX不能直接赋值的
    OUT DX, AX
    RET
    
_io_out32:
    MOV EDX, [ESP+4]
    MOV EAX, [ESP+8]
    OUT DX, EAX
    RET

_io_load_eflags:
    PUSHFD ;PUSH EFLAGS push flags double-word
    POP EAX ;根据c语言规定，有RET的，EAX即为返回值
    RET

_io_store_eflags: ;void io_store_eflags(int eflags)
    MOV EAX, [ESP+4]
    PUSH EAX ;将eax压入栈
    POPFD ;POP EFLAGS，eflags=弹出来的eax，因为不能直接mov eflags, eax
    RET


_write_mem8: ; void write_mem8(int addr, int data)
    MOV ECX, [ESP+4] ;esp+4 = addr
    MOV AL, [ESP+8] ;esp+8 = data
    MOV [ECX], AL
    RET

_load_gdtr:		; void load_gdtr(int limit, int addr);
	MOV	AX,[ESP+4]		; limit
	MOV	[ESP+6], AX
	LGDT [ESP+6]
	RET

_load_idtr:		; void load_idtr(int limit, int addr);
	MOV	AX,[ESP+4]		; limit
	MOV	[ESP+6],AX
	LIDT [ESP+6]
	RET