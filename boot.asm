[bits 16]
[org 0x7c00]

xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00
mov [a], dl

mov bx, 0x1000
mov si, 1
mov di, 40

r:
mov ax, si
xor dx, dx
mov cx, 36
div cx
mov ch, al

mov ax, dx
xor dx, dx
mov cx, 18
div cx
mov dh, al
mov cl, dl
inc cl

mov ah, 0x02
mov al, 1
mov dl, [a]
int 0x13
jc z

add bx, 512
inc si
dec di
jnz r

cli
lgdt [g]
mov eax, cr0
or eax, 1
mov cr0, eax
jmp 0x08:y

z:
jmp z

[bits 32]
y:
mov ax, 0x10
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
mov esp, 0x90000
jmp 0x1000

g:
dw h - f - 1
dd f

f:
dq 0
dw 0xffff
dw 0
db 0
db 0x9a
db 0xcf
db 0
dw 0xffff
dw 0
db 0
db 0x92
db 0xcf
db 0
h:

a:
db 0

times 510-($-$$) db 0
dw 0xaa55