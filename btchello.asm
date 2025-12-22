
addr0000_0000:
mov dx,base_segment + 0x0100
mov [cs:varDataSegment],dx
mov ah,0x30
int 0x21                                        ; Get DOS version number
mov bp,[0x0002]
mov bx,[0x002C]
mov ds,dx
mov [varDosVersion - seg0100],ax
mov [varInitialEs - seg0100],es
mov [varPsp2C - seg0100],bx
mov [varPsp02 - seg0100],bp
mov word [var01096 - seg0100],0xFFFF
call funcSetInterruptionVectors
les di,[varEnvironmentFarPtr - seg0100]
mov ax,di
mov bx,ax
mov cx,0x7FFF

scanEnvVarsLoop:                                ; was addr0000_0039
cmp word [es:di],0x3738                         ; looks like it is looking for the environment variable named '87'... wtf?
jnz jumpToNextEnvVar
mov dx,[es:di+0x02]
cmp dl,0x3D
jnz jumpToNextEnvVar
and dh,0xDF
inc word [var01096 - seg0100]                   ; Increases the variable if 87 environment variable is present
cmp dh,0x59
jnz jumpToNextEnvVar
inc word [var01096 - seg0100]                   ; Increases the variable if the 87 environment variable starts with value 0x59 or 0x79... ?

jumpToNextEnvVar:                               ; was addr0000_0059
repne
scasb
jcxz abnormalTerminationError
inc bx
cmp [es:di],al
jnz scanEnvVarsLoop
or ch,0x80
neg cx
mov [varEnvironmentFarPtr - seg0100],cx
mov cx,0x0001
shl bx,cl
add bx,+0x08
and bx,-0x08
mov [var0108E - seg0100],bx

; Starts the setup of the stack by working out different stuff
mov dx,ds
sub bp,dx                                       ; BP is equals to the value at psp02 - ds => psp02 - cs - 0x0100
mov di,[var01206 - seg0100]
cmp di,0x0200
jae addr0000_0090
mov di,0x0200
mov [var01206 - seg0100],di

addr0000_0090:
add di,0x0472
jb abnormalTerminationError
add di,[var01204 - seg0100]
jb abnormalTerminationError
mov cl,0x04
shr di,cl
inc di
cmp bp,di
jb abnormalTerminationError
cmp word [var01206 - seg0100],+0x00
jz addr0000_00B3
cmp word [var01204 - seg0100],+0x00
jnz addr0000_00C1

addr0000_00B3:
mov di,0x1000
cmp bp,di
ja addr0000_00C1
mov di,bp
jmp addr0000_00C1

abnormalTerminationError:                       ; was addr0000_00BE
jmp printAbnormalTerminationErrorAndExit

addr0000_00C1:
mov bx,di                                       ; Here DI is the minimum between 0x1000 and (psp02 - cs - 0x0100)
add bx,dx
mov [var010A4 - seg0100],bx
mov [var010A8 - seg0100],bx
mov ax,[varInitialEs - seg0100]
sub bx,ax
mov es,ax
mov ah,0x4A
push di
int 0x21                                        ; int21-4A -> Modify Allocated Memory Block. BX=number of paragraphs for the new block. ES=Currently allocated block
                                                ; If I am understanding properly, the PSP is an allocated block that can be resized. So, its MCB (Memory control block - https://www.stanislavs.org/helppc/memory_control_block.html) must be 1 paragraph before.
pop di
shl di,cl
cli
mov ss,dx                                       ; SS is set to the same DS. This allow mixing global variable and local variable (stack) pointers.
mov sp,di                                       ; SP is now (the minimum between 0x1000 and (psp02 - cs - 0x0100)) * 16
sti

xor ax,ax
mov es,[cs:varDataSegment]
mov di,0x042C
mov cx,0x0472
sub cx,di
repe
stosb
push cs
call word [var0141E - seg0100]
call addr0000_027C

addr0000_00FB:
call addr0000_0367
mov ah,0x00
int 0x1A
mov [var01098 - seg0100],dx
mov [var0109A - seg0100],cx
call word [var01422 - seg0100]
push word [var01088 - seg0100]
push word [var01086 - seg0100]
push word [var01084 - seg0100]
call addr0000_01FA
push ax
call addr0000_0247

funcExitProcess:                                ; was addr0000_0121. Expects error code as parameter in stack
mov ds,[cs:varDataSegment]
call funcRestoreInitialInterruptionHandlers
push cs
call word [var01420 - seg0100]
xor ax,ax
mov si,ax
mov cx,0x002F
nop
cld

loopStartAt0137:                                ; was addr0000_0137
add al,[si]
adc ah,0x00
inc si
loop loopStartAt0137
sub ax,0x0D37
nop
jz funcTerminateProcess
mov cx,0x0019                                   ; This is the length of the message
nop
mov dx,msgNullPointerAssignment - seg0100
call funcPrintError

funcTerminateProcess:                           ; was addr0000_014F
mov bp,sp
mov ah,0x4C
mov al,[bp+0x02]                                ; If I am right, [bp + 0] will contain the funcExitProcess return IP, and [bp + 2] the param pushed. So, 0x01F8 and 0x0003 respectivelly.
int 0x21

funcInt0Handler:                                ; was addr0000_0158
mov cx,0x000E                                   ; this is the length of msgDivideError
nop
mov dx,msgDivideError - seg0100
jmp funcPrintErrorAndExitProcess

funcSetInterruptionVectors:                     ; was addr0000_0162
push ds
mov ax,0x3500
int 0x21
mov [varInitialInt0FarPtr - seg0100],bx
mov [varInitialInt0FarPtr - seg0100 + 2],es
mov ax,0x3504
int 0x21
mov [varInitialInt4FarPtr - seg0100 + 2],bx
mov [varInitialInt4FarPtr - seg0100 + 2],es
mov ax,0x3505
int 0x21
mov [varInitialInt5FarPtr - seg0100],bx
mov [varInitialInt5FarPtr - seg0100 + 2],es
mov ax,0x3506
int 0x21
mov [varInitialInt6FarPtr - seg0100],bx
mov [varInitialInt6FarPtr - seg0100 + 2],es
mov ax,0x2500
mov dx,cs
mov ds,dx
mov dx,funcInt0Handler
int 0x21
pop ds
ret

funcRestoreInitialInterruptionHandlers:         ; was addr0000_01A5
push ds
mov ax,0x2500
lds dx,[varInitialInt0FarPtr - seg0100]
int 0x21
pop ds
push ds
mov ax,0x2504
lds dx,[varInitialInt4FarPtr - seg0100]
int 0x21
pop ds
push ds
mov ax,0x2505
lds dx,[varInitialInt5FarPtr - seg0100]
int 0x21
pop ds
push ds
mov ax,0x2506
lds dx,[varInitialInt6FarPtr - seg0100]
int 0x21
pop ds
ret

addr0000_01D2:
mov word [var01096-seg0100],0x0000
retf

addr0000_01D9:
ret

funcPrintError:                                 ; was addr0000_01DA
mov ah,0x40
mov bx,0x0002
int 0x21                                        ; Write to File/Device using handle. BX=Handle number. CX=Number of bytes to write. DS:DX=Content to write
ret

printAbnormalTerminationErrorAndExit:           ; was addr0000_01E2
mov cx,0x001E                                   ; This is the length of the error message
nop
mov dx,0x0056                                   ; This is pointing to the message 'Abnormal program termination'... but the disassemble did not found it.

funcPrintErrorAndExitProcess:                   ; was addr0000_01E9
mov ds,[cs:varDataSegment]
call funcPrintError
mov ax,0x0003                                   ; This is the error code to be returned in AL when calling int 21-4C
push ax
call funcExitProcess

varDataSegment:                                 ; was var001F8
dw 0x0000

addr0000_01FA:
push bp
mov bp,sp
mov ax,0x0194                                   ; This should be "msgHello - seg0100"... but it is not discovered by the disassembler
push ax
call addr0000_08AF
pop cx
xor ax,ax
jmp addr0000_0209

addr0000_0209:
pop bp
ret

addr0000_020B:                                  ; Standard call function. It ends in block addr0000_0241 with ret 0x0002.
                                                ;  bp + 4 (word)
push bp
mov bp,sp
push si
mov si,[bp+0x04]
or si,si
jl addr0000_022A
cmp si,+0x58
jbe addr0000_021E

addr0000_021B:
mov si,0x0057

addr0000_021E:
mov [var011A2-seg0100],si
mov al,[si+0x01A4]
cbw
xchg ax,si
jmp addr0000_0237

addr0000_022A:
neg si
cmp si,+0x23
ja addr0000_021B
mov word [var011A2-seg0100],0xFFFF

addr0000_0237:
mov ax,si
mov [var01094-seg0100],ax
mov ax,0xFFFF
jmp addr0000_0241

addr0000_0241:
pop si
pop bp
ret 0x0002

addr0000_0246:
ret

addr0000_0247:
push bp
mov bp,sp
jmp addr0000_0256

addr0000_024C:
mov bx,[var01208-seg0100]
shl bx,1
call word [bx+0x042C]

addr0000_0256:
mov ax,[var01208-seg0100]
dec word [var01208-seg0100]
or ax,ax
jnz addr0000_024C
call word [var011FE-seg0100]
call word [var01200-seg0100]
call word [var01202-seg0100]
; Gap at 26d

var00276:
dw 0x0000

var00278:
dw 0x0000
; Gap at 27a

addr0000_027C:
pop [cs:var00276]                               ; This is the returning IP... so 0x00FB
mov [cs:var00278],ds
cld
mov es,[varInitialEs-seg0100]
mov si,0x0080
xor ah,ah
lodsb
inc ax
mov bp,es
xchg dx,si
xchg ax,bx
mov si,[varEnvironmentFarPtr-seg0100]
add si,+0x02
mov cx,0x0001
cmp byte [varDosVersion-seg0100],0x03
jb addr0000_02BA
mov es,[varPsp2C-seg0100]
mov di,si
mov cl,0x7F
xor al,al
repne
scasb
jcxz addr0000_032D
xor cl,0x7F

addr0000_02BA:
sub sp,+0x02
mov ax,0x0001
add ax,bx
add ax,cx
and ax,0xFFFE
mov di,sp
sub di,ax
jb addr0000_032D
mov sp,di
mov ax,es
mov ds,ax
mov ax,ss
mov es,ax
push cx
dec cx
repe
movsb
xor al,al
stosb
mov ds,bp
xchg si,dx
xchg bx,cx
mov ax,bx
mov dx,ax
inc bx

addr0000_02E9:
call addr0000_0305
ja addr0000_02F5

addr0000_02EE:
jb addr0000_0330
call addr0000_0305
ja addr0000_02EE

addr0000_02F5:
cmp al,0x20
jz addr0000_0301
cmp al,0x0D
jz addr0000_0301
cmp al,0x09
jnz addr0000_02E9

addr0000_0301:
xor al,al
jmp addr0000_02E9

addr0000_0305:
or ax,ax
jz addr0000_0310
inc dx
stosb
or al,al
jnz addr0000_0310
inc bx

addr0000_0310:
xchg ah,al
xor al,al
stc
jcxz addr0000_032C
lodsb
dec cx
sub al,0x22
jz addr0000_032C
add al,0x22
cmp al,0x5C
jnz addr0000_032A
cmp byte [si],0x22
jnz addr0000_032A
lodsb
dec cx

addr0000_032A:
or si,si

addr0000_032C:
ret

addr0000_032D:
jmp printAbnormalTerminationErrorAndExit

addr0000_0330:
pop cx
add cx,dx
mov ds,[cs:var00278]
mov [var01084 - seg0100],bx
inc bx
add bx,bx
mov si,sp
mov bp,sp
sub bp,bx
jb addr0000_032D
mov sp,bp
mov [var01086 - seg0100],bp

addr0000_034D:
jcxz addr0000_035D
mov [bp+0x00],si
add bp,+0x02

addr0000_0355:
lodsb
or al,al
loopne addr0000_0355
jz addr0000_034D

addr0000_035D:
xor ax,ax
mov [bp+0x00],ax
jmp word [cs:var00276]

addr0000_0367:
mov cx,[varEnvironmentFarPtr - seg0100]
push cx
call addr0000_04B6
pop cx
mov di,ax
or ax,ax
jz addr0000_039A
push ds
push ds
pop es
mov ds,[varPsp2C - seg0100]
xor si,si
cld
repe
movsb
pop ds
mov di,ax
push es
push word [var0108E - seg0100]
call addr0000_04B6
add sp,+0x02
mov bx,ax
pop es
mov [var01088 - seg0100],ax
or ax,ax
jnz addr0000_039D

addr0000_039A:
jmp printAbnormalTerminationErrorAndExit

addr0000_039D:
xor ax,ax
mov cx,0xFFFF

addr0000_03A2:
mov [bx],di
add bx,+0x02
repne
scasb
cmp [es:di],al
jnz addr0000_03A2
mov [bx],ax
ret

addr0000_03D7:
push bp
mov bp,sp
push si
push di
mov di,[bp+0x04]
mov ax,[di+0x06]
mov [var0146E - seg0100],ax
cmp ax,di
jnz addr0000_03F1
mov word [var0146E - seg0100],0x0000
jmp addr0000_0401

addr0000_03F1:
mov si,[di+0x04]
mov bx,[var0146E - seg0100]
mov [bx+0x04],si
mov ax,[var0146E - seg0100]
mov [si+0x06],ax

addr0000_0401:
pop di
pop si
pop bp
ret

addr0000_0405:
push bp
mov bp,sp
push si
push di
mov di,[bp+0x04]
mov ax,[bp+0x06]
sub [di],ax
mov si,[di]
add si,di
mov ax,[bp+0x06]
inc ax
mov [si],ax
mov [si+0x02],di
mov ax,[var0146C - seg0100]
cmp ax,di
jnz addr0000_042C
mov [var0146C - seg0100],si
jmp addr0000_0434

addr0000_042C:
mov di,si
add di,[bp+0x06]
mov [di+0x02],si

addr0000_0434:
mov ax,si
add ax,0x0004
jmp addr0000_043B

addr0000_043B:
pop di
pop si
pop bp
ret

addr0000_043F:
push bp
mov bp,sp
push si
mov ax,[bp+0x04]
xor dx,dx
and ax,0xFFFF
and dx,0x0000
push dx
push ax
call addr0000_054D
pop cx
pop cx
mov si,ax
cmp si,-0x01
jnz addr0000_0461
xor ax,ax
jmp addr0000_0479

addr0000_0461:
mov ax,[var0146C - seg0100]
mov [si+0x02],ax
mov ax,[bp+0x04]
inc ax
mov [si],ax
mov [var0146C - seg0100],si
mov ax,[var0146C - seg0100]
add ax,0x0004
jmp addr0000_0479

addr0000_0479:
pop si
pop bp
ret

addr0000_047C:
push bp
mov bp,sp
push si
mov ax,[bp+0x04]
xor dx,dx
and ax,0xFFFF
and dx,0x0000
push dx
push ax
call addr0000_054D
pop cx
pop cx
mov si,ax
cmp si,-0x01
jnz addr0000_049E
xor ax,ax
jmp addr0000_04B3

addr0000_049E:
mov [var01470 - seg0100],si
mov [var0146C - seg0100],si
mov ax,[bp+0x04]
inc ax
mov [si],ax
mov ax,si
add ax,0x0004
jmp addr0000_04B3

addr0000_04B3:
pop si
pop bp
ret

addr0000_04B6:
push bp
mov bp,sp
push si
push di
mov di,[bp+0x04]
or di,di
jz addr0000_04C7
cmp di,-0x0C
jbe addr0000_04CB

addr0000_04C7:
xor ax,ax
jmp addr0000_0525

addr0000_04CB:
mov ax,di
add ax,0x000B
and ax,0xFFF8
mov di,ax
cmp word [var01470 - seg0100],+0x00
jnz addr0000_04E3
push di
call addr0000_047C
pop cx
jmp addr0000_0525

addr0000_04E3:
mov si,[var0146E - seg0100]
mov ax,si
or ax,ax
jz addr0000_051E

addr0000_04ED:
mov ax,[si]
mov dx,di
add dx,+0x28
cmp ax,dx
jb addr0000_0501
push di
push si
call addr0000_0405
pop cx
pop cx
jmp addr0000_0525

addr0000_0501:
mov ax,[si]
cmp ax,di
jb addr0000_0515
push si
call addr0000_03D7
pop cx
inc word [si]
mov ax,si
add ax,0x0004
jmp addr0000_0525

addr0000_0515:
mov si,[si+0x06]
cmp si,[var0146E - seg0100]
jnz addr0000_04ED

addr0000_051E:
push di
call addr0000_043F
pop cx
jmp addr0000_0525

addr0000_0525:
pop di
pop si
pop bp
ret

addr0000_054D:
push bp
mov bp,sp
mov ax,[bp+0x04]
mov dx,[bp+0x06]
add ax,[var0109E - seg0100]
adc dx,+0x00
mov cx,ax
add cx,0x0100
adc dx,+0x00
or dx,dx
jnz addr0000_0574
cmp cx,sp
jae addr0000_0574
xchg ax,[var0109E - seg0100]
jmp addr0000_057F

addr0000_0574:
mov word [var01094 - seg0100],0x0008
mov ax,0xFFFF
jmp addr0000_057F

addr0000_057F:
pop bp
ret

addr0000_05A1:
push bp
mov bp,sp
sub sp,0x008A
push si
push di
mov ax,[bp+0x08]
inc ax
cmp ax,0x0002
jae addr0000_05B8
xor ax,ax
jmp addr0000_06AE

addr0000_05B8:
mov bx,[bp+0x04]
shl bx,1
test word [bx+0x034A],0x8000
jz addr0000_05D7
push word [bp+0x08]
push word [bp+0x06]
push word [bp+0x04]
call addr0000_06B4
add sp,+0x06
jmp addr0000_06AE

addr0000_05D7:
mov bx,[bp+0x04]
shl bx,1
and word [bx+0x034A],0xFDFF
mov ax,[bp+0x06]
mov [bp-0x0084],ax
mov ax,[bp+0x08]
mov [bp-0x0088],ax
lea si,[bp-0x0082]
jmp addr0000_0663

addr0000_05F6:
dec word [bp-0x0088]
mov bx,[bp-0x0084]
inc word [bp-0x0084]
mov al,[bx]
mov [bp-0x0085],al
cmp al,0x0A
jnz addr0000_0610
mov byte [si],0x0D
inc si

addr0000_0610:
mov al,[bp-0x0085]
mov [si],al
inc si
lea ax,[bp-0x0082]
mov dx,si
sub dx,ax
cmp dx,0x0080
jl addr0000_0663
lea ax,[bp-0x0082]
mov di,si
sub di,ax
push di
lea ax,[bp-0x0082]
push ax
push word [bp+0x04]
call addr0000_06B4
add sp,+0x06
mov [bp-0x008A],ax
cmp ax,di
jz addr0000_065F
cmp word [bp-0x008A],+0x00
jae addr0000_0650
mov ax,0xFFFF
jmp addr0000_065D

addr0000_0650:
mov ax,[bp+0x08]
sub ax,[bp-0x0088]
add ax,[bp-0x008A]
sub ax,di

addr0000_065D:
jmp addr0000_06AE

addr0000_065F:
lea si,[bp-0x0082]

addr0000_0663:
cmp word [bp-0x0088],+0x00
jz addr0000_066D
jmp addr0000_05F6

addr0000_066D:
lea ax,[bp-0x0082]
mov di,si
sub di,ax
mov ax,di
or ax,ax
jbe addr0000_06A9
push di
lea ax,[bp-0x0082]
push ax
push word [bp+0x04]
call addr0000_06B4
add sp,+0x06
mov [bp-0x008A],ax
cmp ax,di
jz addr0000_06A9
cmp word [bp-0x008A],+0x00
jae addr0000_069E
mov ax,0xFFFF
jmp addr0000_06A7

addr0000_069E:
mov ax,[bp+0x08]
add ax,[bp-0x008A]
sub ax,di

addr0000_06A7:
jmp addr0000_06AE

addr0000_06A9:
mov ax,[bp+0x08]
jmp addr0000_06AE

addr0000_06AE:
pop di
pop si
mov sp,bp
pop bp
ret

addr0000_06B4:
push bp
mov bp,sp
mov bx,[bp+0x04]
shl bx,1
test word [bx+0x034A],0x0800
jz addr0000_06D4
mov ax,0x0002
push ax
xor ax,ax
push ax
push ax
push word [bp+0x04]
call moveFilePosition
mov sp,bp

addr0000_06D4:
mov ah,0x40
mov bx,[bp+0x04]
mov cx,[bp+0x08]
mov dx,[bp+0x06]
int 0x21
jb addr0000_06F2
push ax
mov bx,[bp+0x04]
shl bx,1
or word [bx+0x034A],0x1000
pop ax
jmp addr0000_06F8

addr0000_06F2:
push ax
call addr0000_020B
jmp addr0000_06F8

addr0000_06F8:
pop bp
ret

moveFilePosition:                               ; was addr0000_06FA.
                                                ; C declared function that expects the following arguments:
                                                ;  bp + 4: Handle
                                                ;  bp + 6 (dword): new position for the file
                                                ;  bp + 0x0A (byte): type of move.
push bp
mov bp,sp
mov bx,[bp+0x04]
shl bx,1
and word [bx+0x034A],0xFDFF
mov ah,0x42
mov al,[bp+0x0A]
mov bx,[bp+0x04]
mov cx,[bp+0x08]
mov dx,[bp+0x06]
int 0x21                                        ; int21-42: Move current position within a file open via handle
                                                ; AL: Type of move:
                                                ;  0: From beginning of file
                                                ;  1: From current location
                                                ;  2: From end of file
                                                ; BX: Handle
                                                ; CX:DX: Number of bytes to move.
                                                ; Returns the new position in DX:AX if CF not set.
jb addr0000_071C
jmp addr0000_0723

addr0000_071C:
push ax
call addr0000_020B
cwd
jmp addr0000_0723

addr0000_0723:
pop bp
ret

addr0000_0831:
push bp
mov bp,sp
push si
push di
mov si,[bp+0x04]
mov ax,[si+0x0E]
cmp ax,si
jz addr0000_0845
mov ax,0xFFFF
jmp addr0000_08AB

addr0000_0845:
cmp word [si],+0x00
jl addr0000_0877
test word [si+0x02],0x0008
jnz addr0000_085D
mov ax,[si+0x0A]
mov dx,si
add dx,+0x05
cmp ax,dx
jnz addr0000_0873

addr0000_085D:
mov word [si],0x0000
mov ax,[si+0x0A]
mov dx,si
add dx,+0x05
cmp ax,dx
jnz addr0000_0873
mov ax,[si+0x08]
mov [si+0x0A],ax

addr0000_0873:
xor ax,ax
jmp addr0000_08AB

addr0000_0877:
mov di,[si+0x06]
add di,[si]
inc di
sub [si],di
push di
mov ax,[si+0x08]
mov [si+0x0A],ax
push ax
mov al,[si+0x04]
cbw
push ax
call addr0000_05A1
add sp,+0x06
cmp ax,di
jz addr0000_08A7
test word [si+0x02],0x0200
jnz addr0000_08A7
or word [si+0x02],0x0010
mov ax,0xFFFF
jmp addr0000_08AB

addr0000_08A7:
xor ax,ax
jmp addr0000_08AB

addr0000_08AB:
pop di
pop si
pop bp
ret

addr0000_08AF:
push bp
mov bp,sp
mov ax,addr0000_09E5
push ax
mov ax,0x021A
push ax
push word [bp+0x04]
lea ax,[bp+0x06]
push ax
call addr0000_0AD7
jmp addr0000_08C6

addr0000_08C6:
pop bp
ret

addr0000_08C8:
push bp
mov bp,sp
mov bx,[bp+0x06]
dec word [bx]
push word [bp+0x06]
mov al,[bp+0x04]
cbw
push ax
call addr0000_08E1
mov sp,bp
jmp addr0000_08DF

addr0000_08DF:
pop bp
ret

addr0000_08E1:
push bp
mov bp,sp
sub sp,+0x02
push si
mov si,[bp+0x06]
mov al,[bp+0x04]
mov [bp-0x01],al

addr0000_08F1:
inc word [si]
jge addr0000_092B
mov al,[bp-0x01]
inc word [si+0x0A]
mov bx,[si+0x0A]
mov [bx-0x01],al
test word [si+0x02],0x0008
jz addr0000_0923
cmp byte [bp-0x01],0x0A
jz addr0000_0914
cmp byte [bp-0x01],0x0D
jnz addr0000_0923

addr0000_0914:
push si
call addr0000_0831
pop cx
or ax,ax
jz addr0000_0923
mov ax,0xFFFF
jmp addr0000_09CA

addr0000_0923:
mov al,[bp-0x01]
mov ah,0x00
jmp addr0000_09CA

addr0000_092B:
dec word [si]
test word [si+0x02],0x0090
jnz addr0000_093B
test word [si+0x02],0x0002
jnz addr0000_0946

addr0000_093B:
or word [si+0x02],0x0010
mov ax,0xFFFF
jmp addr0000_09CA

addr0000_0946:
or word [si+0x02],0x0100
cmp word [si+0x06],+0x00
jz addr0000_0975
cmp word [si],+0x00
jz addr0000_0966
push si
call addr0000_0831
pop cx
or ax,ax
jz addr0000_0964
mov ax,0xFFFF
jmp addr0000_09CA

addr0000_0964:
jmp addr0000_0970

addr0000_0966:
mov ax,[si+0x06]
mov dx,0xFFFF
sub dx,ax
mov [si],dx

addr0000_0970:
jmp addr0000_08F1

addr0000_0975:
cmp byte [bp-0x01],0x0A
jnz addr0000_099A
test word [si+0x02],0x0040
jnz addr0000_099A
mov ax,0x0001
push ax
mov ax,0x03A4
push ax
mov al,[si+0x04]
cbw
push ax
call addr0000_06B4
add sp,+0x06
cmp ax,0x0001
jnz addr0000_09B2

addr0000_099A:
mov ax,0x0001
push ax
lea ax,[bp+0x04]
push ax
mov al,[si+0x04]
cbw
push ax
call addr0000_06B4
add sp,+0x06
cmp ax,0x0001
jz addr0000_09C3

addr0000_09B2:
test word [si+0x02],0x0200
jnz addr0000_09C3
or word [si+0x02],0x0010
mov ax,0xFFFF
jmp addr0000_09CA

addr0000_09C3:
mov al,[bp-0x01]
mov ah,0x00
jmp addr0000_09CA

addr0000_09CA:
pop si
mov sp,bp
pop bp
ret

addr0000_09E5:                                  ; The end of this function is in block addr0000_0AA0. There, the function finishes with ret 0x0006, which means that this si an standard call with 3 words as parameters.
                                                ;  bp + 4: Looks like a near pointer to an struct of at least 12 bytes, like:
                                                ;   si (word)
                                                ;   si + 2 (word): instruction "test" is used against this... so I guess they are flags.
                                                ;   si + 4 (signed byte): cbw is used with this value to obtain its word representation.
                                                ;   si + 5 (byte?): I do not see any use of this yet
                                                ;   si + 6 (word)
                                                ;   si + 8 (near pointer): It value is used to read other part of the memory.
                                                ;   si + 0x0A (near pointer): It value is used to read other part of the memory.
                                                ;  bp + 6: number of bytes stored in the buffer
                                                ;  bp + 8: pointer to the buffer containing the string of characters
push bp
mov bp,sp
sub sp,+0x02                                    ; Reserves a space of 2 bytes for local variables
push si
push di
mov si,[bp+0x04]
mov di,[bp+0x06]
mov [bp-0x02],di
test word [si+0x02],0x0008
jz addr0000_0A23
jmp addr0000_0A19

addr0000_09FF:
push si
mov bx,[bp+0x08]
inc word [bp+0x08]
mov al,[bx]
cbw
push ax
call addr0000_08E1
pop cx
pop cx
cmp ax,0xFFFF
jnz addr0000_0A19
xor ax,ax
jmp addr0000_0AA0

addr0000_0A19:
mov ax,di
dec di
or ax,ax
jnz addr0000_09FF
jmp addr0000_0A9B

addr0000_0A23:
test word [si+0x02],0x0040
jz addr0000_0A62
cmp word [si+0x06],+0x00
jz addr0000_0A62
mov ax,[si+0x06]
cmp ax,di
jae addr0000_0A62
cmp word [si],+0x00
jz addr0000_0A49
push si
call addr0000_0831
pop cx
or ax,ax
jz addr0000_0A49
xor ax,ax
jmp addr0000_0AA0

addr0000_0A49:
push di
push word [bp+0x08]
mov al,[si+0x04]
cbw
push ax
call addr0000_06B4
add sp,+0x06
cmp ax,di
jae addr0000_0A60
xor ax,ax
jmp addr0000_0AA0

addr0000_0A60:
jmp addr0000_0A9B

addr0000_0A62:
jmp addr0000_0A94

addr0000_0A64:
inc word [si]
jge addr0000_0A7D
mov bx,[bp+0x08]
inc word [bp+0x08]
mov al,[bx]
inc word [si+0x0A]
mov bx,[si+0x0A]
mov [bx-0x01],al
mov ah,0x00
jmp addr0000_0A8B

addr0000_0A7D:
push si
mov bx,[bp+0x08]
inc word [bp+0x08]
push word [bx]
call addr0000_08C8
pop cx
pop cx

addr0000_0A8B:
cmp ax,0xFFFF
jnz addr0000_0A94
xor ax,ax
jmp addr0000_0AA0

addr0000_0A94:
mov ax,di
dec di
or ax,ax
jnz addr0000_0A64

addr0000_0A9B:
mov ax,[bp-0x02]
jmp addr0000_0AA0

addr0000_0AA0:
pop di
pop si
mov sp,bp
pop bp
ret 0x0006

addr0000_0AD7:
                                                ; Expected values in stack:
                                                ;  bp + 0x06 (near pointer - 2 bytes): Pointer to the unresolved message to display. By unresolved it is understood that the message can contain "%d", "%s" and so on.
push bp
mov bp,sp
sub sp,0x0098                                   ; Reserves space for local variables:
                                                ;  bp - 0x96 (near pointer - 2 bytes): Pointer to the buffer where resolved text is stored within this stack.
                                                ;  bp - 0x55 (byte): Length of the buffer for the resolved string
                                                ;  bp - 0x54 (string of characters, 0x50 bytes capacity): Buffer where resolved text is stored
push si
push di
mov word [bp-0x58],0x0000
mov byte [bp-0x55],0x50
mov word [bp-0x02],0x0000
jmp addr0000_0B31

addr0000_0AFD:
mov [di],al
inc di
dec byte [bp-0x55]
jle addr0000_0B30

addr0000_0B05:
push bx
push cx
push dx
push es
lea ax,[bp-0x54]
sub di,ax                                       ; Now DI has the number of characters already stored in the target buffer
lea ax,[bp-0x54]
push ax
push di
push word [bp+0x08]
call word [bp+0x0A]                             ; This must be a standard call function with 3 words as parameters. Like addr0000_09E5
or ax,ax
jnz addr0000_0B22
mov word [bp-0x02],0x0001

addr0000_0B22:
mov byte [bp-0x55],0x50
add [bp-0x58],di
lea di,[bp-0x54]
pop es
pop dx
pop cx
pop bx

addr0000_0B30:
ret

addr0000_0B31:
push es
cld
lea di,[bp-0x54]
mov [bp-0x0096],di
mov di,[bp-0x0096]                              ; DI is initialised with a pointer to the beginning of the buffer to store the resolved message.
mov si,[bp+0x06]                                ; SI is initialised with a pointer to the beginning of the unresolved message.

addr0000_0B41:
lodsb
or al,al
jz addr0000_0B57                                ; if char is '\0'... then we finish resolving the message
cmp al,0x25
jz addr0000_0B5A                                ; if char is '%'... then we need to resolve the placeholder.

addr0000_0B4A:
mov [di],al
inc di
dec byte [bp-0x55]
jg addr0000_0B41
call addr0000_0B05                              ; we need to enlarge the buffer maybe?
jmp addr0000_0B41

addr0000_0B57:
jmp addr0000_0FD6

addr0000_0B5A:
mov [bp-0x008A],si
lodsb
cmp al,0x25
jz addr0000_0B4A
mov [bp-0x0096],di
xor cx,cx
mov [bp-0x008C],cx
mov [bp-0x0098],cx
mov [bp-0x008D],cl
mov word [bp-0x0092],0xFFFF
mov word [bp-0x0090],0xFFFF
jmp addr0000_0B84

addr0000_0B84:
xor ah,ah
mov dx,ax
mov bx,ax
sub bl,0x20
cmp bl,0x60
jae addr0000_0BD9
mov bl,[bx+0x03BD]
mov ax,bx
cmp ax,0x0017
jbe addr0000_0BA0
jmp addr0000_0FC4

addr0000_0BA0:
mov bx,ax
shl bx,1
jmp word [cs:bx+0x0BA9]

addr0000_0BD9:
jmp addr0000_0FC4

addr0000_0FC4:
mov si,[bp-0x008A]
mov di,[bp-0x0096]
mov al,0x25

addr0000_0FCE:
call addr0000_0AFD
lodsb
or al,al
jnz addr0000_0FCE

addr0000_0FD6:
cmp byte [bp-0x55],0x50
jge addr0000_0FDF
call addr0000_0B05

addr0000_0FDF:
pop es
cmp word [bp-0x02],+0x00
jz addr0000_0FED
mov ax,0xFFFF
jmp addr0000_0FF2

addr0000_0FED:
mov ax,[bp-0x58]
jmp addr0000_0FF2

addr0000_0FF2:
pop di
pop si
mov sp,bp
pop bp
ret 0x0008

seg0100:

msgNullPointerAssignment:                       ; was var0102F
db `Null pointer assignment\r\n`

msgDivideError:                                 ; was var01048
db `Divide error\r\n`

; msgAbnormalProgramTermination:                ; this should be var01056... but the disassembler did not found it!
; db ´Abnormal program termination´

varInitialInt0FarPtr:                           ; was var01074
dw 0x0000
dw 0x0000

varInitialInt4FarPtr:                           ; was var01078
dw 0x0000
dw 0x0000

varInitialInt5FarPtr:                           ; was var0107C
dw 0x0000
dw 0x0000

varInitialInt6FarPtr:                           ; was var01080
dw 0x0000
dw 0x0000

var01084:
dw 0x0000

var01086:
dw 0x0000

var01088:
dw 0x0000

varEnvironmentFarPtr:                           ; was var0108A
dw 0x0000

varPsp2C:                                       ; was var0108C
dw 0x0000

var0108E:
dw 0x0000

varInitialEs:                                   ; was var01090
dw 0x0000

varDosVersion:                                  ; was var01092
dw 0x0000

var01094:
dw 0x0000

var01096:
dw 0x0000

var01098:
dw 0x0000

var0109A:
dw 0x0000

var0109E:
dw 0x0472

var010A4:
dw 0x0000

var010A8:
dw 0x0000

varPsp02:                                       ; was var010AC
dw 0x0000

; msgHello:                                     ; this should be var01194, but it is not discovered by the disassembler
; db `Hello World!\n`, 0x00

var011A2:
dw 0x0000

var011FE:
dw 0x0246

var01200:
dw 0x0246

var01202:
dw 0x0246

var01204:
dw 0x0000

var01206:
dw 0x1000

var01208:
dw 0x0000

var0141E:
dw 0x01D2

var01420:
dw 0x01D2

var01422:
dw 0x01D9

var0146C:
dw 0x0000

var0146E:
dw 0x0000

var01470:
dw 0x0000
