; Working Bootloader for CLKernel Revolutionary OS
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
    
    ; Show loading message
    mov si, loading_msg
    call print
    
    ; Load kernel from floppy (not hard disk!)
    mov si, load_msg
    call print
    
    ; Read kernel from sector 2 onwards
    mov ah, 0x02        ; Read sectors
    mov al, 10          ; Read 10 sectors (5KB kernel)
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Start from sector 2
    mov dh, 0           ; Head 0
    mov dl, 0           ; FLOPPY DRIVE (0x00, not 0x80!)
    mov bx, 0x1000      ; Load kernel at 0x1000
    int 0x13
    jc disk_error
    
    mov si, success_msg
    call print
    
    ; Setup GDT for protected mode
    lgdt [gdt_descriptor]
    
    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Jump to 32-bit code
    jmp 0x08:protected_start

disk_error:
    mov si, error_msg
    call print
    jmp hang

print:
    pusha
print_loop:
    lodsb
    cmp al, 0
    je print_done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print_loop
print_done:
    popa
    ret

hang:
    cli
    hlt
    jmp hang

; Messages
loading_msg db 'CLKernel Revolutionary OS', 13, 10
            db 'Loading kernel...', 13, 10, 0
load_msg    db 'Reading from floppy disk...', 13, 10, 0
success_msg db 'Kernel loaded! Entering protected mode...', 13, 10, 0
error_msg   db 'ERROR: Failed to load kernel!', 13, 10, 0

; GDT
gdt_start:
    ; Null descriptor
    dq 0
    
    ; Code segment
    dw 0xFFFF       ; Limit
    dw 0x0000       ; Base (low)
    db 0x00         ; Base (middle)
    db 10011010b    ; Access byte
    db 11001111b    ; Granularity
    db 0x00         ; Base (high)
    
    ; Data segment
    dw 0xFFFF       ; Limit
    dw 0x0000       ; Base (low)
    db 0x00         ; Base (middle)
    db 10010010b    ; Access byte
    db 11001111b    ; Granularity
    db 0x00         ; Base (high)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]
protected_start:
    ; Set up segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Jump to kernel (loaded at 0x1000)
    jmp 0x1000

; Pad to 510 bytes and boot signature
times 510-($-$$) db 0
dw 0xAA55
