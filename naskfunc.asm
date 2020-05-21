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
GLOBAL _asm_int_handler21, _asm_int_handler27, _asm_int_handler2c, _asm_int_handler20
GLOBAL _load_cr0, _store_cr0, _memtest_sub
GLOBAL _load_tr, _task_switch4, _task_switch3, _far_jmp
EXTERN _int_handler21, _int_handler27, _int_handler2c, _int_handler20



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
    ;注意栈地址从高到低，一个结构体也好，一个数组也好，最开始的元素都在低地址
    ;esp+0 到 esp+3是函数地址
    ;一个函数包括参数，是从右到左依次入栈
    ;所以函数名字在最低地址
    MOV ECX, [ESP+4] ;esp+4 to esp+7 = addr
    MOV AL, [ESP+8] ;esp+8 to esp+11 = data
    MOV [ECX], AL
    RET

_load_gdtr:        ; void load_gdtr(int limit, int addr);
    ;使用48位GPTR寄存器的唯一办法就是LGDT命令
    ;低16位存储limit，高32位表示段开始地址
    ;x86下是小端，即数值的高位在内存中高位
    MOV    AX,[ESP+4]        ; limit
    MOV    [ESP+6], AX
    LGDT [ESP+6] ;from esp+6 to esp+12共6bytes，48bits
    RET

_load_idtr:        ; void load_idtr(int limit, int addr);
    MOV    AX,[ESP+4]        ; limit
    MOV    [ESP+6],AX
    LIDT [ESP+6]
    RET

_load_cr0:
    MOV EAX, CR0
    RET

_store_cr0:
    MOV EAX, [ESP+4]
    MOV CR0, EAX
    RET

_asm_int_handler21:
   PUSH ES
   PUSH DS
   PUSHAD
   MOV EAX, ESP
   PUSH EAX
   MOV AX, SS
   MOV DS, AX
   MOV ES, AX
   CALL _int_handler21
   POP EAX
   POPAD
   POP DS
   POP ES
   IRETD ;中断返回32位操作数大小

_asm_int_handler27:
    PUSH ES
    PUSH DS
    PUSHAD ;push了一大堆寄存器
    MOV EAX, ESP
    PUSH EAX
    MOV AX, SS
    MOV DS, AX
    MOV ES, AX
    CALL _int_handler27
    POP EAX
    POPAD ;pop了一大堆寄存器
    POP DS
    POP ES
    IRETD

_asm_int_handler2c:
    PUSH ES
    PUSH DS
    PUSHAD
    MOV EAX, ESP
    PUSH EAX
    MOV AX, SS
    MOV DS, AX
    MOV ES, AX
    CALL _int_handler2c
    POP EAX
    POPAD
    POP DS
    POP ES
    IRETD

_asm_int_handler20:
    PUSH ES
    PUSH DS
    PUSHAD
    MOV	 EAX,ESP
    PUSH EAX
    MOV	 AX,SS
    MOV	 DS,AX
    MOV	 ES,AX
    CALL _int_handler20
    POP	 EAX
    POPAD
    POP	DS
    POP	ES
    IRETD




_memtest_sub:  ; unsigned int memtest_sub(unsigned int start, unsigned int end)
;关于esp栈指针移动可以看http://www.ruanyifeng.com/blog/2018/01/assembly-language-primer.html（有错误）
;还是建议自己gcc看汇编8，注意调用call时候会先把eip（当前指令寄存器）压栈，esp自然-4，这是一个坑
    PUSH EDI            ; （由于还要使用EBX, ESI, EDI）
    PUSH ESI
    PUSH EBX
    MOV ESI, 0xaa55aa55      ; t0 = 0xaa55aa55;
    MOV EDI, 0x55aa55aa      ; t1 = 0x55aa55aa;
    MOV EAX, [ESP+12+4]      ; i = start;
mts_loop:
    MOV EBX, EAX
    ADD EBX, 0xffc        ; p = i + 0xffc;
    MOV EDX, [EBX]        ; old = *p;
    MOV [EBX], ESI        ; *p = pat0;
    XOR DWORD [EBX], 0xffffffff  ; *p ^= 0xffffffff;
    CMP EDI,[EBX]        ; if (*p != pat1) goto fin;
    JNE mts_fin
    XOR DWORD [EBX], 0xffffffff  ; *p ^= 0xffffffff;
    CMP ESI, [EBX]        ; if (*p != pat0) goto fin;
    JNE mts_fin
    MOV [EBX], EDX        ; *p = old;
    ADD EAX, 0x1000        ; i += 0x1000;
    CMP EAX, [ESP+12+8]      ; if (i <= end) goto mts_loop;
    JBE mts_loop
    POP EBX
    POP ESI
    POP EDI
    RET
mts_fin:
    MOV [EBX], EDX        ; *p = old;
    POP EBX
    POP ESI
    POP EDI
    RET
    
_load_tr: ;task register
    LTR [ESP+4]
    RET

_task_switch4: ;4为段地址，far jump
    JMP 4*8:0
    RET

_task_switch3: ;3为段地址，far jump
    JMP 3*8:0
    RET

_far_jmp:
    ;esp+0到esp+3是函数地址
    JMP FAR [ESP+4] ; esp+4开始到esp+7 -> eip esp+8到esp+11共4个字节 -> cs
    RET


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
