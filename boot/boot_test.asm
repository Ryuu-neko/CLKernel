; Minimal test bootloader
[BITS 16]
[ORG 0x7C00]

start:
    ; Setup segments
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    
    ; Print test message
    mov si, msg
    call print
    
    ; Simple infinite loop
.hang:
    hlt
    jmp .hang

print:
    lodsb
    cmp al, 0
    je .done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print
.done:
    ret

msg db 'CLKernel Revolutionary OS - BOOT TEST OK!', 13, 10, 0

; Pad to 510 bytes
times 510-($-$$) db 0
dw 0xAA55  ; Boot signature
