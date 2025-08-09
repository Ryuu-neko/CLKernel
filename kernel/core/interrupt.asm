; =============================================================================
; CLKernel - Interrupt Service Routines (ISR) and IRQ Handlers
; =============================================================================
; File: interrupt.asm
; Purpose: Low-level interrupt handling assembly code
; 
; CLKernel Innovation: All interrupts save full context and transition 
; seamlessly to C handlers which can then feed the async actor system
; =============================================================================

[BITS 32]
section .text

; External C functions
extern exception_handler
extern irq_handler
extern idt_stats

; Constants
%define IRQ_BASE 32

; =============================================================================
; Macro to create ISR stubs without error codes
; =============================================================================
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli                     ; Disable interrupts
    push 0                  ; Push dummy error code
    push %1                 ; Push interrupt number
    jmp isr_common_stub     ; Jump to common handler
%endmacro

; =============================================================================
; Macro to create ISR stubs with error codes  
; =============================================================================
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli                     ; Disable interrupts
                           ; Error code already pushed by CPU
    push %1                ; Push interrupt number
    jmp isr_common_stub    ; Jump to common handler
%endmacro

; =============================================================================
; Macro to create IRQ stubs
; =============================================================================
%macro IRQ 2
global irq%1
irq%1:
    cli                     ; Disable interrupts
    push 0                  ; Push dummy error code
    push %2                 ; Push IRQ number + IRQ_BASE
    jmp irq_common_stub     ; Jump to common IRQ handler
%endmacro

; =============================================================================
; CPU Exception ISRs (0-31)
; =============================================================================

ISR_NOERRCODE 0     ; Division By Zero Exception
ISR_NOERRCODE 1     ; Debug Exception
ISR_NOERRCODE 2     ; Non Maskable Interrupt Exception
ISR_NOERRCODE 3     ; Breakpoint Exception
ISR_NOERRCODE 4     ; Into Detected Overflow Exception
ISR_NOERRCODE 5     ; Out of Bounds Exception
ISR_NOERRCODE 6     ; Invalid Opcode Exception
ISR_NOERRCODE 7     ; No Coprocessor Exception
ISR_ERRCODE   8     ; Double Fault Exception (error code)
ISR_NOERRCODE 9     ; Coprocessor Segment Overrun Exception
ISR_ERRCODE   10    ; Bad TSS Exception (error code)
ISR_ERRCODE   11    ; Segment Not Present Exception (error code)
ISR_ERRCODE   12    ; Stack Fault Exception (error code)
ISR_ERRCODE   13    ; General Protection Fault Exception (error code)
ISR_ERRCODE   14    ; Page Fault Exception (error code)
ISR_NOERRCODE 15    ; Unknown Interrupt Exception
ISR_NOERRCODE 16    ; Coprocessor Fault Exception
ISR_ERRCODE   17    ; Alignment Check Exception (error code)
ISR_NOERRCODE 18    ; Machine Check Exception
ISR_NOERRCODE 19    ; SIMD Floating-Point Exception

; Reserved exceptions (20-31)
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; =============================================================================
; Hardware IRQ Handlers (32-47)
; =============================================================================

IRQ 0,  32      ; Timer (IRQ0)
IRQ 1,  33      ; Keyboard (IRQ1)
IRQ 2,  34      ; Cascade (IRQ2)
IRQ 3,  35      ; COM2/COM4 (IRQ3)
IRQ 4,  36      ; COM1/COM3 (IRQ4)
IRQ 5,  37      ; LPT2/Sound (IRQ5)
IRQ 6,  38      ; Floppy (IRQ6)
IRQ 7,  39      ; LPT1 (IRQ7)
IRQ 8,  40      ; RTC (IRQ8)
IRQ 9,  41      ; ACPI SCI (IRQ9)
IRQ 10, 42      ; Available (IRQ10)
IRQ 11, 43      ; Available (IRQ11)
IRQ 12, 44      ; PS/2 Mouse (IRQ12)
IRQ 13, 45      ; FPU (IRQ13)
IRQ 14, 46      ; Primary ATA (IRQ14)
IRQ 15, 47      ; Secondary ATA (IRQ15)

; =============================================================================
; Common ISR Handler (CPU Exceptions)
; =============================================================================
isr_common_stub:
    ; Save all general purpose registers
    pusha
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10        ; Kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Prepare interrupt frame for C handler
    ; Stack layout from top to bottom:
    ; - gs, fs, es, ds (just pushed)
    ; - edi, esi, ebp, esp, ebx, edx, ecx, eax (pusha)
    ; - interrupt_number (pushed by ISR macro)
    ; - error_code (pushed by CPU or ISR macro)
    ; - eip, cs, eflags, esp, ss (pushed by CPU)
    
    ; Push pointer to interrupt frame
    mov eax, esp
    push eax
    
    ; Call C exception handler
    cld                 ; Clear direction flag for string operations
    call exception_handler
    
    ; Clean up stack (remove frame pointer)
    add esp, 4
    
    ; Restore segment registers
    pop gs
    pop fs  
    pop es
    pop ds
    
    ; Restore general purpose registers
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    sti                 ; Re-enable interrupts
    iret

; =============================================================================
; Common IRQ Handler (Hardware Interrupts)
; =============================================================================
irq_common_stub:
    ; Save all general purpose registers  
    pusha
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10        ; Kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Prepare interrupt frame for C handler
    mov eax, esp
    push eax
    
    ; Call C IRQ handler
    cld                 ; Clear direction flag
    call irq_handler
    
    ; Clean up stack
    add esp, 4
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore general purpose registers
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    sti                 ; Re-enable interrupts
    iret

; =============================================================================
; Advanced Features for CLKernel
; =============================================================================

; Fast system call entry point (future use)
global syscall_entry
syscall_entry:
    ; TODO: Implement fast system call interface
    ; This will be used for high-performance system calls
    ; that bypass the traditional interrupt mechanism
    cli
    push eax            ; System call number
    push ebx            ; Argument 1
    push ecx            ; Argument 2
    push edx            ; Argument 3
    ; ... handle syscall
    sti
    ret

; Context switch helper (for async scheduler)
global context_switch
context_switch:
    ; TODO: Implement context switching for async actors
    ; This will save current actor state and restore new actor state
    ; Essential for cooperative multitasking in the async system
    pusha
    push ds
    push es
    push fs
    push gs
    
    ; Save current stack pointer to old actor
    ; mov [old_actor_esp], esp
    
    ; Load new actor stack pointer
    ; mov esp, [new_actor_esp]
    
    pop gs
    pop fs
    pop es
    pop ds
    popa
    ret

; Atomic operations for lock-free data structures
global atomic_compare_and_swap
atomic_compare_and_swap:
    ; TODO: Implement CAS for lock-free actor message queues
    ; This is crucial for the async actor system performance
    push ebp
    mov ebp, esp
    
    mov eax, [ebp + 8]      ; old_value
    mov ebx, [ebp + 12]     ; new_value  
    mov ecx, [ebp + 16]     ; memory_location
    
    lock cmpxchg [ecx], ebx ; Atomic compare and swap
    
    pop ebp
    ret

; Performance counter for profiling
global read_timestamp_counter
read_timestamp_counter:
    ; Read CPU timestamp counter for performance monitoring
    rdtsc                   ; Result in EDX:EAX
    ret

; CPU identification and features
global get_cpu_features
get_cpu_features:
    ; Get CPU features for optimization
    push ebx
    push ecx
    push edx
    
    mov eax, 1              ; CPUID function 1
    cpuid                   ; Get CPU features
    
    ; Return features in EAX
    mov eax, edx
    
    pop edx
    pop ecx  
    pop ebx
    ret

; =============================================================================
; Memory barriers for SMP systems (future)
; =============================================================================

global memory_barrier_full
memory_barrier_full:
    ; Full memory barrier
    mfence
    ret

global memory_barrier_read
memory_barrier_read:
    ; Read memory barrier
    lfence  
    ret

global memory_barrier_write
memory_barrier_write:
    ; Write memory barrier
    sfence
    ret

; =============================================================================
; Debug and Profiling Support
; =============================================================================

global debug_breakpoint
debug_breakpoint:
    ; Software breakpoint for debugging
    int 3
    ret

global get_stack_pointer
get_stack_pointer:
    ; Get current stack pointer
    mov eax, esp
    ret

global get_instruction_pointer  
get_instruction_pointer:
    ; Get current instruction pointer
    mov eax, [esp]          ; Return address is current IP
    ret

; =============================================================================
; CLKernel Async Support Functions
; =============================================================================

global yield_to_scheduler
yield_to_scheduler:
    ; Cooperative yield point for async actors
    ; TODO: Save current actor state and switch to scheduler
    pusha
    
    ; Save FPU state if needed
    ; fsave [current_actor_fpu_state]
    
    ; Mark current actor as yielding
    ; mov [current_actor_state], ACTOR_STATE_WAITING
    
    ; Switch to scheduler context
    ; call scheduler_select_next_actor
    
    popa
    ret

global actor_message_send_fast
actor_message_send_fast:
    ; Fast path for actor message sending
    ; TODO: Lock-free message queue operations
    push ebp
    mov ebp, esp
    
    ; Get parameters
    mov eax, [ebp + 8]      ; target_actor_id
    mov ebx, [ebp + 12]     ; message_ptr
    mov ecx, [ebp + 16]     ; message_size
    
    ; TODO: Implement fast message queuing
    ; - Find target actor mailbox
    ; - Atomic enqueue operation
    ; - Signal target actor if needed
    
    mov eax, 1              ; Return success for now
    
    pop ebp
    ret
