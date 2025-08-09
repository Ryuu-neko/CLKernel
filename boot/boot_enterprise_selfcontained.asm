; CLKernel Enterprise - Self-Contained Interrupt Demo  
; No kernel loading - everything in bootloader with timer interrupt

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
    
    ; Set video mode and blue background
    mov ax, 0x0003
    int 0x10
    
    mov ax, 0x0600
    mov bh, 0x17        ; White on blue
    mov cx, 0x0000
    mov dx, 0x184F
    int 0x10
    
    ; Display title
    mov si, title
    call print_str
    
    mov si, loading
    call print_str
    
    ; Setup a simple GDT for protected mode
    lgdt [gdt_descriptor]
    
    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Jump to 32-bit protected mode
    jmp 0x08:protected_mode

[BITS 32]
protected_mode:
    ; Setup data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9000
    
    ; Setup simple IDT for timer interrupt
    call setup_idt
    
    ; Initialize PIC and PIT
    call init_pic
    call init_pit
    
    ; Display success message
    call show_success
    
    ; Enable interrupts
    sti
    
    ; Main loop
main_loop:
    hlt
    jmp main_loop

; 32-bit functions
setup_idt:
    ; Setup IDT for timer interrupt (IRQ0 -> INT 32)
    mov edi, idt
    mov ecx, 256
    xor eax, eax
clear_idt:
    stosd
    stosd
    loop clear_idt
    
    ; Set timer interrupt handler
    mov eax, timer_handler
    mov word [idt + 32*8], ax       ; Low 16 bits
    mov word [idt + 32*8 + 2], 0x08 ; Code segment
    mov byte [idt + 32*8 + 5], 0x8E ; Interrupt gate
    shr eax, 16
    mov word [idt + 32*8 + 6], ax   ; High 16 bits
    
    ; Load IDT
    lidt [idt_descriptor]
    ret

init_pic:
    ; Remap PIC
    mov al, 0x11
    out 0x20, al
    out 0xA0, al
    
    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al
    
    mov al, 0x04
    out 0x21, al
    mov al, 0x02
    out 0xA1, al
    
    mov al, 0x01
    out 0x21, al
    out 0xA1, al
    
    ; Enable timer interrupt only
    mov al, 0xFE
    out 0x21, al
    mov al, 0xFF
    out 0xA1, al
    ret

init_pit:
    ; Set PIT to 100Hz
    mov al, 0x36
    out 0x43, al
    
    mov ax, 11931    ; 1193182 / 100
    out 0x40, al
    mov al, ah
    out 0x40, al
    ret

show_success:
    ; Display success message at VGA buffer
    mov esi, success_msg
    mov edi, 0xB8000
    mov ah, 0x1A     ; Green on blue
show_loop:
    lodsb
    cmp al, 0
    je show_done
    mov [edi], ax
    add edi, 2
    jmp show_loop
show_done:
    
    ; Show uptime counter location
    mov esi, uptime_msg
    mov edi, 0xB8000 + (1*160)  ; Second line
    mov ah, 0x1F     ; White on blue
uptime_loop:
    lodsb
    cmp al, 0
    je uptime_done
    mov [edi], ax
    add edi, 2
    jmp uptime_loop
uptime_done:
    ret

; Timer interrupt handler
timer_handler:
    pusha
    
    ; Increment tick counter
    inc dword [tick_counter]
    
    ; Update display every 100 ticks
    mov eax, [tick_counter]
    mov ebx, 100
    xor edx, edx
    div ebx
    cmp edx, 0
    jne timer_done
    
    ; Update uptime display
    mov esi, 0xB8000 + (0*160) + (70*2)  ; Top right
    mov ah, 0x1E     ; Yellow on blue
    
    ; Simple counter display
    mov al, 'U'
    mov [esi], ax
    add esi, 2
    mov al, 'P'
    mov [esi], ax
    add esi, 2
    mov al, ':'
    mov [esi], ax
    add esi, 2
    
    ; Show seconds (simple)
    mov eax, [tick_counter]
    mov ebx, 100
    xor edx, edx
    div ebx
    
    and eax, 0xFF   ; Keep it simple
    mov bl, 10
    xor edx, edx
    div ebx
    
    add al, '0'
    mov [esi], ax
    add esi, 2
    
    add dl, '0'
    mov al, dl
    mov [esi], ax
    
timer_done:
    ; Send EOI
    mov al, 0x20
    out 0x20, al
    
    popa
    iret

[BITS 16]
; 16-bit print function
print_str:
    pusha
    mov ah, 0x0E
    mov bl, 0x1F
print_loop:
    lodsb
    cmp al, 0
    je print_done
    int 0x10
    jmp print_loop
print_done:
    popa
    ret

; Data
title db 'CLKernel Enterprise - Self-Contained Interrupt Demo', 13, 10, 10, 0
loading db 'Initializing interrupt system...', 13, 10, 0
success_msg db 'INTERRUPT SYSTEM ACTIVE - KERNEL ALIVE!', 0
uptime_msg db 'Watch uptime counter (top-right)!', 0

tick_counter dd 0

; GDT
gdt_start:
    dd 0x0, 0x0                ; Null descriptor
    dd 0x0000FFFF, 0x00CF9A00   ; Code segment  
    dd 0x0000FFFF, 0x00CF9200   ; Data segment
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; IDT (will be set up in protected mode)
idt_descriptor:
    dw 256*8 - 1
    dd idt

; Reserve space for IDT
idt:
    times 256*8 db 0

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55
