
addr0000_0000:
mov dx,base_segment+0x0100
mov [cs:varDataSegment],dx
mov ah,0x30
int 0x21                                    ; Get DOS version number
mov bp,[0x0002]
mov bx,[0x002C]
mov ds,dx
mov [varDosVersion-seg0100],ax
mov [varInitialEs-seg0100],es
mov [varPsp2C-seg0100],bx
mov [varPsp02-seg0100],bp
mov word [var01096 - seg0100],0xFFFF
call funcSetInterruptionVectors
les di,[varEnvironmentFarPtr - seg0100]
mov ax,di
mov bx,ax
mov cx,0x7FFF

scanEnvVarsLoop:                            ; was addr0000_0039
cmp word [es:di],0x3738                     ; looks like it is looking for the environment variable named '87'... wtf?
jnz jumpToNextEnvVar
mov dx,[es:di+0x02]
cmp dl,0x3D
jnz jumpToNextEnvVar
and dh,0xDF
inc word [var01096-seg0100]                 ; Increases the variable if 87 environment variable is present
cmp dh,0x59
jnz jumpToNextEnvVar
inc word [var01096-seg0100]                 ; Increases the variable if the 87 environment variable starts with value 0x59 or 0x79... ?

jumpToNextEnvVar:                           ; was addr0000_0059
repne
scasb
jcxz abnormalTerminationError
inc bx
cmp [es:di],al
jnz scanEnvVarsLoop
or ch,0x80
neg cx
mov [varEnvironmentFarPtr-seg0100],cx
mov cx,0x0001
shl bx,cl
add bx,+0x08                                ; BX is the number of environment variable found * 2 + 8
and bx,-0x08
mov [var0108E-seg0100],bx

; Starts the setup of the stack by working out different stuff
mov dx,ds
sub bp,dx                                   ; BP is equals to the value at psp02 - ds => psp02 - cs - 0x0100
mov di,[var01206-seg0100]
cmp di,0x0200
jae addr0000_0090
mov di,0x0200
mov [var01206-seg0100],di

addr0000_0090:
add di,0x0472
jb abnormalTerminationError
add di,[var01204-seg0100]
jb abnormalTerminationError
mov cl,0x04
shr di,cl
inc di
cmp bp,di
jb abnormalTerminationError
cmp word [var01206-seg0100],+0x00
jz addr0000_00B3
cmp word [var01204-seg0100],+0x00
jnz addr0000_00C1

addr0000_00B3:
mov di,0x1000
cmp bp,di
ja addr0000_00C1
mov di,bp
jmp addr0000_00C1

abnormalTerminationError:                   ; was addr0000_00BE
jmp printAbnormalTerminationErrorAndExit

addr0000_00C1:
mov bx,di                                   ; Here DI is the minimum between 0x1000 and (psp02 - cs - 0x0100)
add bx,dx
mov [var010A4-seg0100],bx
mov [var010A8-seg0100],bx
mov ax,[varInitialEs-seg0100]
sub bx,ax
mov es,ax
mov ah,0x4A
push di
int 0x21                                    ; int21-4A -> Modify Allocated Memory Block. BX=number of paragraphs for the new block. ES=Currently allocated block
                                            ; If I am understanding properly, the PSP is an allocated block that can be resized. So, its MCB (Memory control block - https://www.stanislavs.org/helppc/memory_control_block.html) must be 1 paragraph before.
pop di
shl di,cl
cli
mov ss,dx                                   ; SS is set to the same DS
mov sp,di                                   ; SP is now (the minimum between 0x1000 and (psp02 - cs - 0x0100)) * 16
sti

xor ax,ax
mov es,[cs:varDataSegment]
mov di,0x042C
mov cx,0x0472
sub cx,di
repe
stosb
push cs
call word [var0141E-seg0100]
call addr0000_027C
; Gap at fb

funcExitProcess:                            ; was addr0000_0121. Expects error code as parameter in stack
mov ds,[cs:varDataSegment]
call funcRestoreInitialInterruptionHandlers
push cs
call word [var01420 - seg0100]
xor ax,ax
mov si,ax
mov cx,0x002F
nop
cld

loopStartAt0137:                            ; was addr0000_0137
add al,[si]
adc ah,0x00
inc si
loop loopStartAt0137
sub ax,0x0D37
nop
jz funcTerminateProcess
mov cx,0x0019
nop
mov dx,0x002F
call funcPrintError

funcTerminateProcess:                       ; was addr0000_014F
mov bp,sp
mov ah,0x4C
mov al,[bp+0x02]                            ; If I am right, [bp + 0] will contain the funcExitProcess return IP, and [bp + 2] the param pushed. So, 0x01F8 and 0x0003 respectivelly.
int 0x21

funcInt0Handler:                            ; was addr0000_0158
mov cx,0x000E                               ; length of msgDivideError message
nop
mov dx,msgDivideError - seg0100
jmp funcPrintErrorAndExitProcess

funcSetInterruptionVectors:                 ; was addr0000_0162
push ds
mov ax,0x3500
int 0x21
mov [varInitialInt0FarPtr - seg0100],bx
mov [varInitialInt0FarPtr - seg0100 + 2],es
mov ax,0x3504
int 0x21
mov [varInitialInt4FarPtr - seg0100],bx
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

funcRestoreInitialInterruptionHandlers:     ; was addr0000_01A5
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
; Gap at 1d9

funcPrintError:                             ; was addr0000_01DA
mov ah,0x40
mov bx,0x0002
int 0x21                                    ; Write to File/Device using handle. BX=Handle number. CX=Number of bytes to write. DS:DX=Content to write
ret

printAbnormalTerminationErrorAndExit:       ; addr0000_01E2
mov cx,0x001E
nop
mov dx,0x0056

funcPrintErrorAndExitProcess:               ; was addr0000_01E9
mov ds,[cs:varDataSegment]
call funcPrintError
mov ax,0x0003                               ; This is the error code to be returned in AL when calling int 21-4C
push ax
call funcExitProcess

varDataSegment:                             ; was var001F8
dw 0x0000

var00276:
dw 0x0000

var00278:
dw 0x0000

addr0000_027C:
pop [cs:var00276]                           ; This is the returning IP... so 0x00FB
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
mov [0x0084],bx
inc bx
add bx,bx
mov si,sp
mov bp,sp
sub bp,bx
jb addr0000_032D
mov sp,bp
mov [0x0086],bp

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

seg0100:

; There is an undiscovered message here at var0102F (length 0x0019). Referenced from funcExitProcess

msgDivideError:   ; was var01048
db `Divide error\r\n`

; There is an undiscovered message here at var01056 (Abnormal program termination - length 0x001E).

varInitialInt0FarPtr:                       ; was var01074
dw 0x0000
dw 0x0000

varInitialInt4FarPtr:                       ; was var01078
dw 0x0000
dw 0x0000

varInitialInt5FarPtr:                       ; was var0107C
dw 0x0000
dw 0x0000

varInitialInt6FarPtr:                       ; was var01080
dw 0x0000
dw 0x0000

varEnvironmentFarPtr:   ; was var0108A. Starts at 0 but after scanning the environment variables is set to the end of the environment.
dw 0x0000

varPsp2C:                                   ; was var0108C. According to wikipedia: Environment segment
dw 0x0000

var0108E:                                   ; This stores (the number of environment variables found * 2 + 8) & 0xFFF8... why? I do not know
dw 0x0000

varInitialEs:                               ; was var01090
dw 0x0000

varDosVersion:                              ; was var01092
dw 0x0000

var01096:
dw 0x0000

var010A4:
dw 0x0000

var010A8:
dw 0x0000

varPsp02:                                   ; was var010AC. According to wikipedia: Segment of the first byte beyond the memory allocated to the program.
dw 0x0000

var01204:
dw 0x0000

var01206:
dw 0x1000

var0141E:
dw 0x01D2

var01420:
dw addr0000_01D2

; There is an undiscovered segment of data from var0142C to var01472 that is set to 0 just after setting up the stack.
; Not sure why it is not directly all set to 0 if this belong to the loaded file itself.
