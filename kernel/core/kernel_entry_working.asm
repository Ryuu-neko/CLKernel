; Working Kernel Entry - linked to 0x1000
[BITS 32]
section .text

global _start
extern _kernel_main

_start:
    ; Setup stack
    mov esp, 0x90000    ; Stack at 0x90000
    mov ebp, esp
    
    ; Call C kernel
    call _kernel_main
    
    ; Infinite halt if kernel returns
    cli
halt_forever:
    hlt
    jmp halt_forever
