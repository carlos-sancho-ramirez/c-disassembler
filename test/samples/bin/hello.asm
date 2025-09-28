org 0x100

addr0100:
mov dx,var0109
mov ah,0x09
int 0x21
int 0x20

var0109:
db 'Hello World!$'
