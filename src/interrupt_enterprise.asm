; CLKernel Enterprise - Interrupt Service Routines
; Phase 1: Timer interrupt implementation

[BITS 32]

section .text

; Timer interrupt handler (IRQ0 -> INT32)
global isr32
isr32:
    cli                    ; Disable interrupts
    pusha                  ; Save all general purpose registers
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10           ; Kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C timer handler
    extern timer_handler
    call timer_handler
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore general purpose registers
    popa
    
    ; Re-enable interrupts and return
    sti
    iret
