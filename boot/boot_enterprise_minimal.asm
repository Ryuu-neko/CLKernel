; CLKernel Enterprise - Minimal Interrupt Demo
[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    sti
    
    ; Blue screen
    mov ax, 0x0600
    mov bh, 0x17
    mov cx, 0
    mov dx, 0x184F
    int 0x10
    
    ; Title
    mov si, title
    call print
    
    ; Enter protected mode
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_start

print:
    mov ah, 0x0E
    mov bl, 0x1F
p_loop:
    lodsb
    cmp al, 0
    je p_done
    int 0x10
    jmp p_loop
p_done:
    ret

[BITS 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x9000
    
    ; Show message
    mov esi, msg
    mov edi, 0xB8000
    mov ah, 0x1A
m_loop:
    lodsb
    cmp al, 0
    je m_done
    mov [edi], ax
    add edi, 2
    jmp m_loop
m_done:
    
    ; Simple timer setup
    mov al, 0x11
    out 0x20, al
    mov al, 0x20
    out 0x21, al
    mov al, 0x04
    out 0x21, al
    mov al, 0x01
    out 0x21, al
    mov al, 0xFE
    out 0x21, al
    
    ; PIT
    mov al, 0x36
    out 0x43, al
    mov ax, 1193
    out 0x40, al
    mov al, ah
    out 0x40, al
    
    ; Simple counter loop
    xor eax, eax
counter_loop:
    inc eax
    
    ; Display counter every 100000 iterations
    mov ebx, 100000
    xor edx, edx
    div ebx
    cmp edx, 0
    jne skip_display
    
    ; Show counter
    mov edi, 0xB8000 + 70*2
    mov ecx, eax
    and ecx, 0x0F
    add ecx, '0'
    cmp ecx, '9'
    jle digit_ok
    add ecx, 7
digit_ok:
    mov ch, 0x1E
    mov [edi], cx
    
skip_display:
    mov eax, [counter]
    inc eax
    mov [counter], eax
    jmp counter_loop

; Data
title db 'Enterprise Kernel Demo', 13, 10, 0
msg db 'INTERRUPT SYSTEM DEMO - COUNTER ACTIVE!', 0
counter dd 0

; GDT
gdt:
    dd 0, 0
    dd 0x0000FFFF, 0x00CF9A00
    dd 0x0000FFFF, 0x00CF9200
gdt_desc:
    dw 23
    dd gdt

times 510-($-$$) db 0
dw 0xAA55
