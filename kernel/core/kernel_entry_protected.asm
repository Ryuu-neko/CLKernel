; Kernel entry for protected mode
[BITS 32]
section .text

global _start
extern _kernel_main

_start:
    ; Setup data segments
    mov ax, 0x10    ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Setup stack
    mov esp, stack_top
    mov ebp, esp
    
    ; Call kernel
    call _kernel_main
    
    ; If kernel returns, halt
    cli
.halt_forever:
    hlt
    jmp .halt_forever

; Stack
section .bss
align 4
stack_bottom:
    resb 16384  ; 16KB stack
stack_top:
