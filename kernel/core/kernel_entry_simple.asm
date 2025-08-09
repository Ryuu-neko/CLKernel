; Simple kernel entry for test
[BITS 32]
section .text

global _start
extern _kernel_main

_start:
    ; Setup stack
    mov esp, stack_top
    mov ebp, esp
    
    ; Clear interrupt flag for stability
    cli
    
    ; Call kernel
    call _kernel_main
    
    ; If kernel returns (should never happen), halt forever
    cli
.halt_forever:
    hlt
    jmp .halt_forever

; Simple stack
section .bss
align 4
stack_bottom:
    resb 16384  ; 16KB stack
stack_top:
