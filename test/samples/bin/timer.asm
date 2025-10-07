org 0x100

addr0100:
xor ax,ax
mov es,ax
mov ax,[es:0x0020]
mov [var0167],ax
mov ax,[es:0x0022]
mov [var0169],ax
mov ax,addr013C
cli
mov [es:0x0020],ax
mov ax,cs
mov [es:0x0022],ax
sti

addr0121:
xor ax,ax
cmp [var013B],al
jnz addr0121
mov ax,[var0167]
cli
mov [es:0x0020],ax
mov ax,[var0169]
mov [es:0x0022],ax
sti
int 0x20

var013B:
db 0x5B

addr013C:
push ds
push cs
pop ds
push ax
xor ax,ax
cmp [var013B],al
jz addr0164
sub byte [var013B],0x01
mov al,[var013B]
push bx
mov bl,0x12
div bl
add al,0x31
mov [var016B],al
push dx
mov dx,var016B
mov ah,0x09
int 0x21
pop dx
pop bx

addr0164:
pop ax
pop ds
db 0xEA

var0167:
dw 0x0000

var0169:
dw 0x0000

var016B:
db 0x78
db 0x24
