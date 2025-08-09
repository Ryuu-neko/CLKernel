; =============================================================================
; CLKernel Entry Point - Assembly Bridge to C Kernel
; =============================================================================
; Purpose: Transition from bootloader to C kernel main function
; Sets up proper stack and calls kernel_main()
; =============================================================================

[BITS 32]           ; 32-bit protected mode
section .text.entry

global _start
extern kernel_main
extern _kernel_stack_top

_start:
    ; =========================
    ; Setup Kernel Stack
    ; =========================
    mov esp, _kernel_stack_top  ; Set stack pointer to top of kernel stack
    mov ebp, esp                ; Set base pointer
    
    ; =========================
    ; Clear EFLAGS
    ; =========================
    push 0
    popf
    
    ; =========================
    ; Initialize Segments
    ; =========================
    ; Ensure all segment registers are properly set
    mov ax, 0x10        ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; =========================
    ; Call C Kernel Main
    ; =========================
    call kernel_main
    
    ; =========================
    ; Kernel Should Never Return
    ; =========================
    ; If kernel_main returns, halt the system
    cli
halt_loop:
    hlt
    jmp halt_loop
