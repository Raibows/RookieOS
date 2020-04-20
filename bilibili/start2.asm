	org 07c00h
	mov ax, cs
	mov ds, ax
	mov es, ax
	call DispStr
	jmp $

DispStr:
	mov ax, BootMessage
	mov bp, ax
	mov cx, 16
	mov ax, 0x1301
	mov bx, 000ch
	mov dl, 0
	int 10h
	ret

BootMessage: db "hello, OS!"


times 510-($-$$) db 0
dw 0xaa55
