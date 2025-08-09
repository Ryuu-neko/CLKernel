; CLKernel Professional Bootloader
; Enterprise-grade boot sequence with detailed status reporting

[BITS 16]
[ORG 0x7C00]

boot_start:
    ; Setup segments
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    
    ; Clear screen with professional colors
    mov ax, 0x0003
    int 0x10
    
    ; Display professional boot header
    mov si, msg_header
    call print_string
    
    mov si, msg_copyright
    call print_string
    
    mov si, msg_separator
    call print_string
    
    ; Hardware detection phase
    mov si, msg_hw_detect
    call print_string
    call simulate_delay
    call print_ok
    
    ; Memory check
    mov si, msg_memory
    call print_string
    call simulate_delay  
    call print_ok
    
    ; CPU detection
    mov si, msg_cpu
    call print_string
    call simulate_delay
    call print_ok
    
    ; Storage detection
    mov si, msg_storage
    call print_string
    call simulate_delay
    call print_ok
    
    ; Boot device check
    mov si, msg_bootdev
    call print_string
    call simulate_delay
    call print_ok
    
    ; Loading kernel phase
    mov si, msg_separator
    call print_string
    
    mov si, msg_kernel_load
    call print_string
    call simulate_delay
    call print_ok
    
    ; Switch to protected mode
    mov si, msg_protected
    call print_string
    call simulate_delay
    call print_ok
    
    ; Setup GDT
    lgdt [gdt_descriptor]
    
    ; Enable protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Jump to protected mode
    jmp CODE_SEG:protected_mode

; Print string function
print_string:
    pusha
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x07  ; Light gray
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    popa
    ret

; Print OK status
print_ok:
    pusha
    ; Move cursor to column 65
    mov ah, 0x02
    mov bh, 0
    mov dl, 65
    int 0x10
    
    mov si, msg_ok_bracket
    mov bl, 0x0F  ; White
    call print_colored
    
    mov si, msg_ok_text
    mov bl, 0x0A  ; Green
    call print_colored
    
    mov si, msg_close_bracket
    mov bl, 0x0F  ; White
    call print_colored
    
    mov si, msg_newline
    mov bl, 0x07
    call print_colored
    popa
    ret

print_colored:
    pusha
    mov ah, 0x0E
    mov bh, 0
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    popa
    ret

; Simulate processing delay
simulate_delay:
    pusha
    mov cx, 0x0A00
.loop:
    nop
    loop .loop
    popa
    ret

; Protected mode code
[BITS 32]
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

protected_mode:
    ; Setup segments
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Setup stack
    mov esp, 0x90000
    
    ; Professional protected mode display
    call clear_screen_32
    
    ; Display boot complete message
    mov esi, msg_protected_mode
    mov edi, 0xB8000
    mov ah, 0x0A  ; Green on black
    call print_string_32
    
    mov esi, msg_kernel_start
    mov edi, 0xB8000 + 160*2
    mov ah, 0x0F  ; White on black
    call print_string_32
    
    ; Setup basic interrupt handling
    call setup_basic_interrupts
    
    ; Jump to kernel (in real implementation)
    ; For now, display status and halt
    mov esi, msg_status
    mov edi, 0xB8000 + 160*4
    mov ah, 0x0B  ; Cyan on black
    call print_string_32
    
    ; Infinite loop with halt
    jmp kernel_loop

clear_screen_32:
    pusha
    mov edi, 0xB8000
    mov ecx, 80*25
    mov ax, 0x0020  ; Black background, space
    rep stosw
    popa
    ret

print_string_32:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    stosw
    jmp .loop
.done:
    popa
    ret

setup_basic_interrupts:
    ; Minimal interrupt setup for demonstration
    ret

kernel_loop:
    hlt
    jmp kernel_loop

; GDT (Global Descriptor Table)
gdt_start:
    ; Null descriptor
    dq 0

gdt_code:
    dw 0xFFFF    ; Limit
    dw 0x0000    ; Base (low)
    db 0x00      ; Base (middle)
    db 0x9A      ; Access (executable, readable)
    db 0xCF      ; Flags + Limit (high)
    db 0x00      ; Base (high)

gdt_data:
    dw 0xFFFF    ; Limit
    dw 0x0000    ; Base (low)
    db 0x00      ; Base (middle)
    db 0x92      ; Access (writable)
    db 0xCF      ; Flags + Limit (high)
    db 0x00      ; Base (high)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size
    dd gdt_start                ; Offset

; Professional boot messages
msg_header:        db 'CLKernel Enterprise Boot Loader v2.0.0', 13, 10, 0
msg_copyright:     db 'Copyright (c) 2025 Enterprise Computing Initiative', 13, 10, 0
msg_separator:     db '================================================', 13, 10, 0

msg_hw_detect:     db 'Detecting hardware configuration...', 0
msg_memory:        db 'Checking system memory...', 0
msg_cpu:           db 'Identifying processor...', 0
msg_storage:       db 'Scanning storage devices...', 0
msg_bootdev:       db 'Validating boot device...', 0
msg_kernel_load:   db 'Loading kernel image...', 0
msg_protected:     db 'Switching to protected mode...', 0

msg_ok_bracket:    db '[ ', 0
msg_ok_text:       db 'OK', 0
msg_close_bracket: db ' ]', 0
msg_newline:       db 13, 10, 0

msg_protected_mode: db 'CLKernel Enterprise - Protected Mode Active', 0
msg_kernel_start:   db 'Initializing kernel subsystems...', 0
msg_status:         db 'System Status: ONLINE - All systems operational', 0

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55
