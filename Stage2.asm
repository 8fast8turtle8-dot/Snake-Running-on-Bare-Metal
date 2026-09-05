; ============================
; TurtlOS Stage 2 Loader (CHS + debug)
; Loads kernel into 0x00010000, dumps bytes, then jumps
; ============================

[org 0x8000]
[bits 16]

start:
    cli
    cld

    mov ax, cs
    mov ds, ax
    mov [boot_drive], dl

    mov si, msg_stage2
    call print

    ; -------------------------
    ; Enable A20
    ; -------------------------
    in   al, 0x92
    or   al, 00000010b
    out  0x92, al
    mov si, msg_a20
    call print

    ; -------------------------
    ; Load kernel using CHS
    ; -------------------------
    mov si, msg_kernel
    call print

    mov ax, 0x1000          ; ES = 0x1000 → physical 0x00010000
    mov es, ax
    xor bx, bx              ; offset = 0

    mov dl, [boot_drive]    ; floppy drive number

    mov ah, 0x02            ; BIOS read sectors
    mov al, 50              ; read 10 sectors
    mov ch, 0               ; cylinder 0
    mov dh, 0               ; head 0
    mov cl, 3               ; sector 3 (kernel start)  <-- CHANGE TO 3
    int 0x13
    jc disk_error

    mov si, msg_loaded
    call print

    ; -------------------------
    ; Show first bytes of kernel in real mode
    ; -------------------------
    mov si, msg_dump_rm
    call print

    mov ax, 0x1000
    mov ds, ax
    mov bx, 0               ; offset 0 in 0x00010000
    mov cx, 16              ; dump 16 bytes
dump_rm_loop:
    mov al, [bx]
    call print_hex8
    inc bx
    loop dump_rm_loop

    ; Restore DS
    mov ax, cs
    mov ds, ax

    ; -------------------------
    ; Load GDT
    ; -------------------------
    lgdt [gdt_descriptor]
    mov si, msg_gdt
    call print

    ; -------------------------
    ; Enter protected mode
    ; -------------------------
    mov eax, cr0
    or  eax, 1
    mov cr0, eax

    jmp 0x08:pm_entry


print:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x0F
    int 0x10
    jmp print
.done:
    ret

; print AL as two hex digits
print_hex8:
    pusha
    mov ah, al
    shr al, 4
    and al, 0x0F
    cmp al, 10
    jl .digit1
    add al, 'A' - 10
    jmp .out1
.digit1:
    add al, '0'
.out1:
    mov bh, 0
    mov bl, 0x0F
    mov ah, 0x0E
    int 0x10

    mov al, ah
    and al, 0x0F
    cmp al, 10
    jl .digit2
    add al, 'A' - 10
    jmp .out2
.digit2:
    add al, '0'
.out2:
    mov ah, 0x0E
    int 0x10

    ; space
    mov al, ' '
    mov ah, 0x0E
    int 0x10

    popa
    ret

disk_error:
    mov si, err
    call print

    ; show AH error code
    mov al, ah
    mov si, msg_err_code
    call print
    call print_hex8

.halt:
    cli
    hlt
    jmp .halt

; -------------------------
; GDT (flat 4GB)
; -------------------------
gdt_start:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

msg_stage2   db 0x0D,0x0A,"[Stage 2] Started",0
msg_a20      db 0x0D,0x0A,"[Stage 2] A20 enabled",0
msg_kernel   db 0x0D,0x0A,"[Stage 2] Loading kernel...",0
msg_loaded   db 0x0D,0x0A,"[Stage 2] Kernel loaded.",0
msg_dump_rm  db 0x0D,0x0A,"[Stage 2] Dump (real mode) first 16 bytes:",0
msg_gdt      db 0x0D,0x0A,"[Stage 2] GDT loaded",0
msg_err_code db 0x0D,0x0A,"[Stage 2] Disk error code: ",0
err          db 0x0D,0x0A,"Disk read error!",0

boot_drive db 0

; ============================
; 32-bit mode
; ============================

[bits 32]

pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x00900000

    ; TEST: write 'S' in protected mode
    mov dword [0xB8000], 0x0F530053    ; 'S'

    ; (optional) dump again in 32-bit later if you want

    ; Jump to kernel loaded at 0x00010000
    jmp 0x00010000

.halt32:
    cli
    hlt
    jmp .halt32

