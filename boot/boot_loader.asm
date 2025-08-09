; CLKernel Revolutionary Bootloader - Loads Colorful Kernel
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
    
    ; Display loading message
    mov si, loading_msg
    call print
    
    ; Load kernel from sector 2 (after bootloader)
    mov ah, 0x02        ; Read sectors
    mov al, 8           ; Read 8 sectors (4KB)
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Start from sector 2
    mov dh, 0           ; Head 0
    mov dl, 0x00        ; Drive 0 (floppy)
    mov bx, 0x1000      ; Load to 0x1000
    int 0x13
    jc disk_error
    
    ; Show success
    mov si, success_msg
    call print
    
    ; Setup GDT for protected mode
    cli
    lgdt [gdt_descriptor]
    
    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Far jump to kernel
    jmp 0x08:0x1000
    
disk_error:
    mov si, error_msg
    call print
    hlt

print:
    pusha
print_loop:
    lodsb
    cmp al, 0
    je print_done
    cmp al, 10
    je newline
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print_loop
newline:
    mov ah, 0x0E
    mov al, 13
    int 0x10
    mov al, 10
    int 0x10
    jmp print_loop
print_done:
    popa
    ret

; GDT for protected mode
gdt_start:
    dd 0x0, 0x0                 ; Null descriptor

gdt_code:
    dw 0xFFFF                   ; Limit low
    dw 0x0000                   ; Base low
    db 0x00                     ; Base middle
    db 10011010b                ; Access byte
    db 11001111b                ; Granularity
    db 0x00                     ; Base high

gdt_data:
    dw 0xFFFF                   ; Limit low
    dw 0x0000                   ; Base low
    db 0x00                     ; Base middle
    db 10010010b                ; Access byte
    db 11001111b                ; Granularity
    db 0x00                     ; Base high
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size
    dd gdt_start                ; Offset

; Messages
loading_msg db 'CLKernel Revolutionary OS - Loading...', 13, 10, 0
success_msg db 'Kernel loaded! Switching to protected mode...', 13, 10, 0
error_msg db 'Disk read error!', 13, 10, 0

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55
