; =============================================================================
; CLKernel Bootloader - Next-Generation OS Kernel
; =============================================================================
; 512-byte MBR bootloader for x86_64 systems
; 
; Memory Layout:
; 0x7C00 - 0x7DFF: Bootloader (512 bytes)
; 0x8000 - 0x8FFF: Kernel loading area (4KB initially)
; 0x9000 - 0x9FFF: Stack area
;
; Boot Process:
; 1. Initialize segments and stack
; 2. Print boot message
; 3. Load kernel from disk
; 4. Switch to protected mode
; 5. Jump to kernel
; =============================================================================

[BITS 16]           ; Start in 16-bit real mode
[ORG 0x7C00]        ; BIOS loads us at 0x7C00

section .text
global _start

_start:
    ; =========================
    ; Initialize CPU State
    ; =========================
    cli                     ; Clear interrupts
    cld                     ; Clear direction flag
    
    ; Setup segments (DS, ES, SS all point to 0x0000)
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000          ; Stack at 0x9000 (grows down)
    
    ; =========================
    ; Display Boot Message
    ; =========================
    mov si, boot_msg
    call print_string
    
    ; =========================
    ; Load Kernel from Disk
    ; =========================
    ; Reset disk system
    mov ah, 0x00
    mov dl, 0x80            ; First hard disk
    int 0x13
    jc disk_error
    
    ; Read kernel sectors (LBA 1-8, 4KB kernel)
    mov ah, 0x02            ; Read sectors function
    mov al, 8               ; Number of sectors to read
    mov ch, 0               ; Cylinder 0
    mov cl, 2               ; Sector 2 (sector 1 is bootloader)
    mov dh, 0               ; Head 0
    mov dl, 0x80            ; Drive 0x80 (first hard disk)
    mov bx, 0x8000          ; Load kernel at 0x8000
    int 0x13
    jc disk_error
    
    mov si, kernel_loaded_msg
    call print_string
    
    ; =========================
    ; Setup GDT for Protected Mode
    ; =========================
    lgdt [gdt_descriptor]
    
    ; =========================
    ; Enter Protected Mode
    ; =========================
    mov eax, cr0
    or eax, 1               ; Set PE bit
    mov cr0, eax
    
    ; Far jump to flush pipeline and load CS
    jmp 0x08:protected_mode
    
; =========================
; Error Handlers
; =========================
disk_error:
    mov si, disk_error_msg
    call print_string
    jmp halt

halt:
    hlt
    jmp halt

; =========================
; Print String Function (16-bit Real Mode)
; =========================
; Input: SI = pointer to null-terminated string
print_string:
    pusha
.print_char:
    lodsb                   ; Load byte from SI into AL
    cmp al, 0
    je .done
    mov ah, 0x0E            ; BIOS teletype function
    mov bh, 0               ; Page number
    mov bl, 0x07            ; Attribute (light gray on black)
    int 0x10                ; BIOS video interrupt
    jmp .print_char
.done:
    popa
    ret

; =========================
; Protected Mode Entry Point
; =========================
[BITS 32]
protected_mode:
    ; Setup data segments in protected mode
    mov ax, 0x10            ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9000         ; Stack pointer
    
    ; Print protected mode message
    mov esi, protected_msg
    call print_string_32
    
    ; Jump to kernel entry point
    jmp 0x8000

; =========================
; Print String Function (32-bit Protected Mode)
; =========================
; Input: ESI = pointer to null-terminated string
print_string_32:
    pusha
    mov edi, 0xB8000        ; VGA text buffer
    mov ecx, 0              ; Character position counter
.print_char_32:
    lodsb                   ; Load byte from ESI
    cmp al, 0
    je .done_32
    mov [edi + ecx*2], al   ; Character
    mov byte [edi + ecx*2 + 1], 0x0F  ; Attribute (white on black)
    inc ecx
    jmp .print_char_32
.done_32:
    popa
    ret

; =========================
; Global Descriptor Table
; =========================
gdt_start:
    ; Null descriptor (required)
    dq 0x0000000000000000

    ; Code segment descriptor (0x08)
    ; Base: 0x00000000, Limit: 0xFFFFF
    ; Access: Present, Ring 0, Code, Execute/Read
    ; Flags: 32-bit, Page granularity
    dq 0x00CF9A000000FFFF

    ; Data segment descriptor (0x10)  
    ; Base: 0x00000000, Limit: 0xFFFFF
    ; Access: Present, Ring 0, Data, Read/Write
    ; Flags: 32-bit, Page granularity
    dq 0x00CF92000000FFFF

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size
    dd gdt_start                ; GDT address

; =========================
; Boot Messages
; =========================
boot_msg:           db 'CLKernel v0.1 - Next-Gen OS', 0x0D, 0x0A
                    db 'Starting Kernel...', 0x0D, 0x0A, 0
kernel_loaded_msg:  db 'Kernel loaded successfully!', 0x0D, 0x0A, 0
disk_error_msg:     db 'DISK ERROR - Boot failed!', 0x0D, 0x0A, 0
protected_msg:      db 'Protected mode active!', 0

; =========================
; Boot Signature
; =========================
times 510-($-$$) db 0   ; Pad to 510 bytes
dw 0xAA55               ; Boot signature (required for MBR)
