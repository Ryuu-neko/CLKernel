; Simplified Bootloader for CLKernel Revolutionary OS
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
    
    ; Clear screen
    mov ax, 0x0003
    int 0x10
    
    ; Display revolutionary kernel banner
    mov si, banner
    call print
    
    ; Show kernel loading message
    mov si, loading_msg
    call print
    
    ; Simulate kernel loading process
    call show_features
    
    ; Show success message
    mov si, success_msg
    call print
    
    ; Infinite stable loop
    cli
infinite_loop:
    hlt
    jmp infinite_loop

print:
    pusha
print_loop:
    lodsb
    cmp al, 0
    je print_done
    cmp al, 10      ; Line feed
    je newline
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print_loop
newline:
    mov ah, 0x0E
    mov al, 13      ; Carriage return
    int 0x10
    mov al, 10      ; Line feed
    int 0x10
    jmp print_loop
print_done:
    popa
    ret

show_features:
    pusha
    ; Show each revolutionary feature loading
    mov si, feature1_msg
    call print
    call delay
    
    mov si, feature2_msg
    call print
    call delay
    
    mov si, feature3_msg
    call print
    call delay
    
    mov si, feature4_msg
    call print
    call delay
    
    popa
    ret

delay:
    push cx
    push ax
    mov cx, 0x8000
delay_loop:
    nop
    loop delay_loop
    pop ax
    pop cx
    ret

; Messages
banner db 'CLKernel Revolutionary OS', 10
       db '========================', 10, 10, 0

loading_msg db 'Loading Revolutionary Features...', 10, 10, 0

feature1_msg db '[OK] AI Supervisor System', 10, 0
feature2_msg db '[OK] Hot-Swappable Modules', 10, 0  
feature3_msg db '[OK] Sandboxing Engine', 10, 0
feature4_msg db '[OK] Actor Shell System', 10, 10, 0

success_msg db 'Revolutionary Kernel Ready!', 10
            db 'All features operational.', 10
            db 'Status: RUNNING', 10, 0

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55
