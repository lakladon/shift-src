; Я хз что написать

MAGIC_NUMBER equ 0x1BADB002
FLAGS equ 0x00000003
CHECKSUM equ -(MAGIC_NUMBER + FLAGS)

extern _start

section .text.multiboot
align 4
; Для boot.asm: переход через заголовок multiboot к исполняемому коду
; Это 3-байтная инструкция jmp
jmp entry_from_boot

; Заголовок Multiboot (должен быть в первых 8 КБ, выровнен по 4 байта)align 4
    dd MAGIC_NUMBER
    dd FLAGS
    dd CHECKSUM

; Настройка GDT для загрузки через GRUB (multiboot)
gdt_start:
    ; Null descriptor
    dq 0x0000000000000000

    ; Сегмент данных: база=0, предел=4ГБ, присутствует, чтение/запись, 32-бит
    dw 0xFFFF           ; Limit (bits 0-15)
    dw 0x0000           ; Base (bits 0-15)
    db 0x00             ; Base (bits 16-23)
    db 0x9A             ; Present, privilege=0, code, readable
    db 0xCF             ; Granularity=4KB, 32-bit, limit (bits 16-19)
    db 0x00             ; Base (bits 24-31)

; Сегмент данных: база=0, предел=4ГБ, присутствует, чтение/запись, 32-бит
    dw 0x0000           ; Base (bits 0-15)
    db 0x00             ; Base (bits 16-23)
    db 0x92             ; Present, privilege=0, data, read/write
    db 0xCF             ; Granularity=4KB, 32-bit, limit (bits 16-19)
    db 0x00             ; Base (bits 24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ 0x08
DATA_SEG equ 0x10

global multiboot_entry
multiboot_entry:
entry_from_boot:
    lgdt [gdt_descriptor]
    
    jmp CODE_SEG:reload_cs
    
reload_cs:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov esp, 0x90000
    
    jmp _start
