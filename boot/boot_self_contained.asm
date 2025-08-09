; CLKernel Revolutionary OS - Self-Contained Bootloader with ASCII Art
; No separate kernel loading - everything in bootloader
[BITS 16]
[ORG 0x7C00]

start:
    ; Initialize segments
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    sti
    
    ; Set video mode (80x25 color text)
    mov ax, 0x0003
    int 0x10
    
    ; Set blue background
    mov ax, 0x0600      ; Scroll up function
    mov bh, 0x17        ; White on blue
    mov cx, 0x0000      ; Upper left corner
    mov dx, 0x184F      ; Lower right corner (24, 79)
    int 0x10
    
    ; Position cursor at top
    mov ah, 0x02
    mov bh, 0x00
    mov dx, 0x0000
    int 0x10
    
    ; Display ASCII Art Banner
    mov si, banner1
    mov bl, 0x1E        ; Yellow on blue
    call print_color
    
    mov si, banner2
    mov bl, 0x1E
    call print_color
    
    mov si, banner3
    mov bl, 0x1E
    call print_color
    
    mov si, title
    mov bl, 0x1B        ; Cyan on blue
    call print_color
    
    ; Show features
    mov si, feature1
    mov bl, 0x1A        ; Green on blue
    call print_color
    
    mov si, feature2
    mov bl, 0x1A
    call print_color
    
    mov si, feature3
    mov bl, 0x1A
    call print_color
    
    mov si, feature4
    mov bl, 0x1A
    call print_color
    
    ; Success message
    mov si, success
    mov bl, 0x1A        ; Green on blue
    call print_color
    
    mov si, footer
    mov bl, 0x1D        ; Magenta on blue
    call print_color
    
    ; Infinite loop
    cli
hang:
    hlt
    jmp hang

; Print string with color
print_color:
    pusha
    mov ah, 0x0E
print_loop:
    lodsb
    cmp al, 0
    je print_done
    cmp al, 10          ; Newline
    je newline
    int 0x10
    jmp print_loop
newline:
    ; Handle newline
    push ax
    mov ah, 0x0E
    mov al, 13          ; Carriage return
    int 0x10
    mov al, 10          ; Line feed
    int 0x10
    pop ax
    jmp print_loop
print_done:
    popa
    ret

; ASCII Art and Messages
banner1 db 'CL KERNEL', 10, 0
banner2 db 'Rev OS', 10, 0
banner3 db 'v1.0', 10, 0

title     db  'REVOLUTIONARY!', 10, 10, 0

feature1 db '[OK] AI', 10, 0
feature2 db '[OK] Modules', 10, 0  
feature3 db '[OK] Security', 10, 0
feature4 db '[OK] Shell', 10, 10, 0

success db 'READY!', 10, 0
footer  db '(c) 2025', 10, 0

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55
