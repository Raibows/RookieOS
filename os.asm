CYLS EQU 0x0ff0
LEDS EQU 0x0ff1
VMODE EQU 0x0ff2 ;颜色数目
SCRNX EQU 0x0ff4 ;屏幕分辨率
SCRNY EQU 0x0ff6
VRAM EQU 0x0ff8 ;图像缓冲区开始地址
;设置上面这些都是为了保存boot info



ORG 0xc200
MOV AL, 0x13 ;VGA显卡 320*200*8位
MOV AH, 0x00
INT 0x10
MOV BYTE [VMODE], 8
MOV WORD [SCRNX], 320
MOV WORD [SCRNY], 200
MOV DWORD [VRAM], 0x000a0000

;用BIOS取的键盘指示灯状态，例如【NumLock】
MOV AH, 0x02
INT 0x16
MOV [LEDS], AL

fin:
    HLT 
    JMP fin