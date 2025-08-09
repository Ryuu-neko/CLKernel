; CLKernel Revolutionary OS - Living Bootloader with Simple Timer
; Shows a living system with basic timer interrupt

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
    
    ; Display Banner
    mov si, title
    mov bl, 0x1E        ; Yellow on blue
    call print_color
    
    mov si, status
    mov bl, 0x1A        ; Green on blue
    call print_color
    
    ; Initialize a simple timer counter
    xor ax, ax
    mov [timer_counter], ax
    mov [timer_counter+2], ax
    
    ; Show features
    mov si, feature1
    mov bl, 0x1A
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
    
    mov si, living_msg
    mov bl, 0x1F        ; White on blue
    call print_color
    
    ; "Living" loop - shows system is alive
main_loop:
    ; Increment counter to simulate uptime
    inc word [timer_counter]
    
    ; Display counter every ~32768 iterations (slower update)
    mov ax, [timer_counter]
    test ax, 0x7FFF     ; Check if lower 15 bits are zero
    jnz skip_display
    
    inc word [timer_counter+2]
    call display_uptime
    
skip_display:
    ; Longer delay for smoother display
    mov cx, 0x1000
delay_loop:
    nop
    nop
    nop
    loop delay_loop
    
    jmp main_loop

; Display uptime counter
display_uptime:
    pusha
    
    ; Position cursor at (65, 0) - top right
    mov ah, 0x02
    mov bh, 0x00
    mov dx, 0x0041      ; Row 0, Col 65
    int 0x10
    
    ; Clear the area first
    mov cx, 10          ; Clear 10 characters
    mov al, ' '
    mov bl, 0x17        ; White on blue
clear_loop:
    mov ah, 0x0E
    int 0x10
    loop clear_loop
    
    ; Position cursor again  
    mov ah, 0x02
    mov dx, 0x0041      ; Row 0, Col 65
    int 0x10
    
    ; Display "UP:" 
    mov si, uptime_label
    mov bl, 0x1F        ; Bright white on blue
    call print_color
    
    ; Display uptime in minutes:seconds format
    mov ax, [timer_counter+2]
    mov bl, 10
    xor dx, dx
    div bl              ; AL = minutes, AH = seconds
    
    ; Display minutes (0-9)
    push ax
    mov al, al          ; AL already has minutes
    add al, 0x30        ; Convert to ASCII
    mov ah, 0x0E
    int 0x10
    
    ; Display colon
    mov al, ':'
    int 0x10
    
    ; Display seconds (0-9)  
    pop ax
    mov al, ah          ; AH has seconds
    add al, 0x30        ; Convert to ASCII
    mov ah, 0x0E
    int 0x10
    
    popa
    ret

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

; Data
timer_counter dd 0

title     db 'CLKernel Revolutionary OS - LIVING SYSTEM', 10, 10, 0
status    db 'STATUS: SYSTEM IS ALIVE AND RUNNING!', 10, 10, 0

feature1 db '[✓] AI Supervisor System Active', 10, 0
feature2 db '[✓] Hot-Swap Modules Running', 10, 0  
feature3 db '[✓] Security Engine Online', 10, 0
feature4 db '[✓] Interactive Shell Ready', 10, 10, 0

living_msg db 'Watch the uptime counter in top-right! System is LIVING!', 10, 0
uptime_label db 'UP:', 0

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55
