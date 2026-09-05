[org 0x7C00]
[bits 16]

start:
    cld

    mov ax, cs
    mov ds, ax

    mov [boot_drive], dl

    cli
    xor ax, ax
    mov ss, ax
    mov sp, 0x7000
    sti

    ; Print Stage 1 message
    mov si, msg
.print_loop:
    lodsb
    or al, al
    jz .done_print
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x07
    int 0x10
    jmp .print_loop
.done_print:

    ; Load Stage 2 to 0x0000:0x8000 (physical 0x00008000)
    mov ax, 0x0000
    mov es, ax
    mov bx, 0x8000

    mov ah, 0x02        ; BIOS read sectors
    mov al, 1           ; Stage 2 is 247 bytes → 1 sector
    mov ch, 0
    mov cl, 2           ; sector 2
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    cmp ah, 0
    jne disk_error

    ; Jump to Stage 2 at 0x0000:0x8000
    jmp 0x0000:0x8000

disk_error:
    mov si, err
.err_loop:
    lodsb
    or al, al
    jz .halt
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x0C
    int 0x10
    jmp .err_loop

.halt:
    cli
    hlt
    jmp .halt

msg db "Stage 1 successful!", 0
err db "Disk read error!", 0
boot_drive db 0

times 510-($-$$) db 0
dw 0xAA55


