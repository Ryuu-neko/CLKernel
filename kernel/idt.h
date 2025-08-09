/*
 * =============================================================================
 * CLKernel - IDT (Interrupt Descriptor Table) Header
 * =============================================================================
 * File: idt.h
 * Purpose: IDT structures, interrupt handling, and async integration
 * 
 * CLKernel Innovation: Interrupts feed into async actor system!
 * Traditional kernels: ISR → direct handler → context switch
 * CLKernel approach: ISR → actor message → async processing
 * =============================================================================
 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// IDT Configuration Constants
// =============================================================================

#define IDT_MAX_DESCRIPTORS     256     // Maximum IDT entries
#define IDT_CPU_EXCEPTIONS      32      // CPU exception count (0-31)
#define IDT_IRQ_BASE           32      // IRQ base (32-47)
#define IDT_SYSCALL_BASE       128     // System call base (128-255)

// Interrupt types
#define IDT_TYPE_TASK          0x5      // Task gate
#define IDT_TYPE_INTERRUPT_16  0x6      // 16-bit interrupt gate  
#define IDT_TYPE_TRAP_16       0x7      // 16-bit trap gate
#define IDT_TYPE_INTERRUPT_32  0xE      // 32-bit interrupt gate
#define IDT_TYPE_TRAP_32       0xF      // 32-bit trap gate

// Privilege levels
#define IDT_PRIVILEGE_KERNEL   0x0      // Ring 0
#define IDT_PRIVILEGE_USER     0x3      // Ring 3

// Flags
#define IDT_FLAG_PRESENT       0x80     // Present bit
#define IDT_FLAG_DPL_MASK      0x60     // Descriptor Privilege Level mask
#define IDT_FLAG_TYPE_MASK     0x0F     // Gate type mask

// =============================================================================
// Hardware IRQ Definitions  
// =============================================================================

#define IRQ0_TIMER             0        // Programmable Interval Timer
#define IRQ1_KEYBOARD          1        // PS/2 Keyboard
#define IRQ2_CASCADE           2        // PIC cascade (never raised)
#define IRQ3_COM2_COM4         3        // Serial ports COM2/COM4
#define IRQ4_COM1_COM3         4        // Serial ports COM1/COM3
#define IRQ5_LPT2_SOUND        5        // LPT2 or Sound card
#define IRQ6_FLOPPY            6        // Floppy disk controller
#define IRQ7_LPT1              7        // LPT1 (parallel port)
#define IRQ8_RTC               8        // Real-time clock
#define IRQ9_ACPI_SCI          9        // ACPI System Control Interrupt
#define IRQ10_AVAILABLE        10       // Available for devices
#define IRQ11_AVAILABLE        11       // Available for devices
#define IRQ12_PS2_MOUSE        12       // PS/2 Mouse
#define IRQ13_FPU              13       // Floating point unit
#define IRQ14_PRIMARY_ATA      14       // Primary ATA channel
#define IRQ15_SECONDARY_ATA    15       // Secondary ATA channel

// =============================================================================
// IDT Structures
// =============================================================================

// IDT Descriptor Entry
typedef struct {
    uint16_t offset_low;        // Lower 16 bits of handler address
    uint16_t selector;          // Code segment selector
    uint8_t  reserved;          // Always zero
    uint8_t  type_attributes;   // Gate type and attributes
    uint16_t offset_high;       // Upper 16 bits of handler address
} __attribute__((packed)) idt_entry_t;

// IDT Pointer (used by LIDT instruction)
typedef struct {
    uint16_t limit;             // Size of IDT - 1
    uint32_t base;              // Address of IDT
} __attribute__((packed)) idt_ptr_t;

// Interrupt frame passed to handlers
typedef struct {
    // Pushed by processor
    uint32_t eip;               // Instruction pointer
    uint32_t cs;                // Code segment
    uint32_t eflags;            // CPU flags
    uint32_t esp;               // Stack pointer (if privilege change)
    uint32_t ss;                // Stack segment (if privilege change)
    
    // Our additions for context
    uint32_t interrupt_number;  // Which interrupt occurred
    uint32_t error_code;        // Error code (if applicable)
    
    // General purpose registers (pushed by our ISR)
    uint32_t edi, esi, ebp, esp_temp, ebx, edx, ecx, eax;
    uint32_t ds, es, fs, gs;    // Segment registers
} __attribute__((packed)) interrupt_frame_t;

// =============================================================================
// Async Integration Structures
// =============================================================================

// Interrupt message for async actor system
typedef struct {
    uint32_t interrupt_number;
    uint32_t error_code;
    uint64_t timestamp;         // When interrupt occurred
    uint32_t cpu_id;           // Which CPU handled it
    void* context_data;        // Additional context if needed
} interrupt_message_t;

// Interrupt handler registration
typedef struct {
    uint32_t interrupt_number;
    void (*handler)(interrupt_frame_t* frame);
    uint32_t target_actor_id;   // Which actor should handle this
    bool async_processing;      // Process via actor or direct call
    char description[64];       // Human-readable description
} interrupt_handler_t;

// =============================================================================
// Exception Definitions
// =============================================================================

#define EXCEPTION_DIVIDE_ERROR          0   // Division by zero
#define EXCEPTION_DEBUG                 1   // Debug exception
#define EXCEPTION_NMI                   2   // Non-maskable interrupt  
#define EXCEPTION_BREAKPOINT            3   // Breakpoint
#define EXCEPTION_OVERFLOW              4   // Overflow
#define EXCEPTION_BOUND_RANGE           5   // Bound range exceeded
#define EXCEPTION_INVALID_OPCODE        6   // Invalid opcode
#define EXCEPTION_DEVICE_NOT_AVAILABLE  7   // Device not available
#define EXCEPTION_DOUBLE_FAULT          8   // Double fault
#define EXCEPTION_COPROCESSOR_OVERRUN   9   // Coprocessor segment overrun
#define EXCEPTION_INVALID_TSS           10  // Invalid TSS
#define EXCEPTION_SEGMENT_NOT_PRESENT   11  // Segment not present
#define EXCEPTION_STACK_SEGMENT_FAULT   12  // Stack segment fault
#define EXCEPTION_GENERAL_PROTECTION    13  // General protection fault
#define EXCEPTION_PAGE_FAULT            14  // Page fault
#define EXCEPTION_RESERVED              15  // Reserved
#define EXCEPTION_FPU_ERROR             16  // x87 FPU floating-point error
#define EXCEPTION_ALIGNMENT_CHECK       17  // Alignment check
#define EXCEPTION_MACHINE_CHECK         18  // Machine check
#define EXCEPTION_SIMD_EXCEPTION        19  // SIMD floating-point exception

// =============================================================================
// Function Prototypes
// =============================================================================

// Core IDT functions
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);
void idt_load(void);

// Handler registration (async-aware)
bool idt_register_handler(uint8_t interrupt_number, 
                         void (*handler)(interrupt_frame_t*),
                         uint32_t target_actor_id,
                         bool async_processing);

bool idt_unregister_handler(uint8_t interrupt_number);

// Interrupt control
void interrupts_enable(void);
void interrupts_disable(void);
bool interrupts_enabled(void);

// Exception handlers
void exception_handler(interrupt_frame_t* frame);
void page_fault_handler(interrupt_frame_t* frame);
void general_protection_fault_handler(interrupt_frame_t* frame);

// IRQ handlers  
void irq_handler(interrupt_frame_t* frame);
void timer_irq_handler(interrupt_frame_t* frame);
void keyboard_irq_handler(interrupt_frame_t* frame);

// Async integration
void interrupt_send_to_actor(uint32_t actor_id, interrupt_message_t* msg);
void interrupt_process_async_queue(void);

// Debug and statistics
void idt_dump_table(void);
void idt_print_stats(void);

// Assembly interrupt stubs (defined in interrupt.asm)
extern void isr0(void);   // Division by zero
extern void isr1(void);   // Debug
extern void isr2(void);   // NMI
extern void isr3(void);   // Breakpoint
extern void isr4(void);   // Overflow
extern void isr5(void);   // Bound range exceeded
extern void isr6(void);   // Invalid opcode
extern void isr7(void);   // Device not available
extern void isr8(void);   // Double fault
extern void isr9(void);   // Coprocessor segment overrun
extern void isr10(void);  // Invalid TSS
extern void isr11(void);  // Segment not present
extern void isr12(void);  // Stack segment fault
extern void isr13(void);  // General protection fault
extern void isr14(void);  // Page fault
extern void isr15(void);  // Reserved
extern void isr16(void);  // FPU error
extern void isr17(void);  // Alignment check
extern void isr18(void);  // Machine check
extern void isr19(void);  // SIMD exception
extern void isr20(void);  // Reserved
extern void isr21(void);  // Reserved
extern void isr22(void);  // Reserved
extern void isr23(void);  // Reserved
extern void isr24(void);  // Reserved
extern void isr25(void);  // Reserved
extern void isr26(void);  // Reserved
extern void isr27(void);  // Reserved
extern void isr28(void);  // Reserved
extern void isr29(void);  // Reserved
extern void isr30(void);  // Reserved
extern void isr31(void);  // Reserved

// IRQ assembly stubs
extern void irq0(void);   // Timer
extern void irq1(void);   // Keyboard
extern void irq2(void);   // Cascade
extern void irq3(void);   // COM2/COM4
extern void irq4(void);   // COM1/COM3
extern void irq5(void);   // LPT2/Sound
extern void irq6(void);   // Floppy
extern void irq7(void);   // LPT1
extern void irq8(void);   // RTC
extern void irq9(void);   // ACPI SCI
extern void irq10(void);  // Available
extern void irq11(void);  // Available
extern void irq12(void);  // PS/2 Mouse
extern void irq13(void);  // FPU
extern void irq14(void);  // Primary ATA
extern void irq15(void);  // Secondary ATA

// Global IDT state
extern idt_entry_t idt_table[IDT_MAX_DESCRIPTORS];
extern idt_ptr_t idt_pointer;
extern interrupt_handler_t registered_handlers[IDT_MAX_DESCRIPTORS];

#endif // IDT_H
