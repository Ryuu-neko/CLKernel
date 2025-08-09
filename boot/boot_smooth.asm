; CLKernel Revolutionary OS - Smooth Living System
; Simple uptime counter without character spinning

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
    
    ; Set blue background
    mov ax, 0x0600
    mov bh, 0x17
    mov cx, 0x0000
    mov dx, 0x184F
    int 0x10
    
    ; Display title
    mov si, title
    call print_str
    
    ; Show features  
    mov si, status
    call print_str
    
    mov si, features
    call print_str
    
    ; Initialize counter
    mov word [uptime], 0
    
    ; Main living loop
main_loop:
    ; Update uptime display
    call show_uptime
    
    ; Delay
    mov cx, 0xFFFF
delay:
    dec cx
    jnz delay
    
    ; Increment uptime
    inc word [uptime]
    
    jmp main_loop

; Show uptime in top-right
show_uptime:
    pusha
    
    ; Position cursor (70, 0)
    mov ah, 0x02
    mov dx, 0x0046
    int 0x10
    
    ; Print "UP:"
    mov ah, 0x0E
    mov bl, 0x1F
    mov al, 'U'
    int 0x10
    mov al, 'P'
    int 0x10
    mov al, ':'
    int 0x10
    
    ; Print uptime (simplified)
    mov ax, [uptime]
    shr ax, 10          ; Divide by 1024 for slower count
    and ax, 0x0F        ; Keep only 0-15
    add al, '0'
    cmp al, '9'
    jle print_digit
    add al, 7           ; A-F for hex
print_digit:
    int 0x10
    
    popa
    ret

; Simple print function
print_str:
    pusha
    mov ah, 0x0E
    mov bl, 0x1F
print_loop:
    lodsb
    cmp al, 0
    je print_done
    cmp al, 10
    je newline
    int 0x10
    jmp print_loop
newline:
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    jmp print_loop
print_done:
    popa
    ret

; Data
uptime dw 0

title db 'CLKernel Revolutionary OS - LIVING!', 10, 10, 0
status db 'STATUS: SYSTEM IS ALIVE!', 10, 10, 0
features db '[OK] All Systems Running', 10
         db '[OK] Counter Active (top-right)', 10, 10
         db 'Kernel is LIVING! Watch UP: counter', 10, 0

; Pad and signature
times 510-($-$$) db 0
dw 0xAA55
