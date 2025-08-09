; CLKernel - Interrupt Service Routines (Assembly)
; Revolutionary OS Architecture - Low-level Interrupt Handlers

[BITS 32]

section .text

; Exception handler macro
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push 0          ; Push dummy error code
    push %1         ; Push interrupt number
    jmp isr_common_stub
%endmacro

; IRQ handler macro  
%macro IRQ 2
global isr%1
isr%1:
    cli
    push 0          ; Push dummy error code
    push %1         ; Push IRQ number
    jmp irq_common_stub
%endmacro

; Define exception handlers
ISR_NOERRCODE 0     ; Division by zero

; Define IRQ handlers
IRQ 32, 0           ; Timer (IRQ0)

; Common exception handler
extern exception_handler
isr_common_stub:
    pusha           ; Push all general purpose registers
    
    mov ax, ds      ; Save data segment
    push eax
    
    mov ax, 0x10    ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call exception_handler
    
    pop eax         ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa            ; Restore all general purpose registers
    add esp, 8      ; Clean up error code and interrupt number
    sti             ; Re-enable interrupts
    iret            ; Return from interrupt

; Common IRQ handler
extern timer_handler
irq_common_stub:
    pusha           ; Push all general purpose registers
    
    mov ax, ds      ; Save data segment
    push eax
    
    mov ax, 0x10    ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call specific IRQ handler based on interrupt number
    mov eax, [esp + 36]  ; Get interrupt number from stack
    cmp eax, 32
    je call_timer
    jmp irq_done
    
call_timer:
    call timer_handler
    jmp irq_done
    
irq_done:
    pop eax         ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa            ; Restore all general purpose registers
    add esp, 8      ; Clean up error code and interrupt number
    sti             ; Re-enable interrupts
    iret            ; Return from interrupt
