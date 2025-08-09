/*
 * =============================================================================
 * CLKernel - IDT (Interrupt Descriptor Table) Implementation  
 * =============================================================================
 * File: idt.c
 * Purpose: IDT setup, interrupt handling with async actor integration
 * 
 * Revolutionary Feature: Interrupts are processed through the async actor
 * system, enabling non-blocking interrupt handling and better fault isolation
 * =============================================================================
 */

#include "idt.h"
#include "kernel.h"
#include "../io.h"
#include "vga.h"
#include "pic.h"

// =============================================================================
// Global IDT State
// =============================================================================

idt_entry_t idt_table[IDT_MAX_DESCRIPTORS];
idt_ptr_t idt_pointer;
interrupt_handler_t registered_handlers[IDT_MAX_DESCRIPTORS];

// Statistics for monitoring and AI supervisor
static struct {
    uint64_t total_interrupts;
    uint64_t exceptions;
    uint64_t irqs;
    uint64_t async_messages_sent;
    uint32_t last_interrupt;
    uint64_t last_interrupt_time;
} idt_stats;

// =============================================================================
// IDT Initialization
// =============================================================================

void idt_init(void)
{
    kprintf("[IDT] Initializing Interrupt Descriptor Table...\n");
    
    // Clear IDT table and statistics
    for (int i = 0; i < IDT_MAX_DESCRIPTORS; i++) {
        idt_table[i].offset_low = 0;
        idt_table[i].selector = 0;
        idt_table[i].reserved = 0;
        idt_table[i].type_attributes = 0;
        idt_table[i].offset_high = 0;
        
        registered_handlers[i].interrupt_number = 0;
        registered_handlers[i].handler = 0;
        registered_handlers[i].target_actor_id = 0;
        registered_handlers[i].async_processing = false;
    }
    
    // Clear statistics
    idt_stats.total_interrupts = 0;
    idt_stats.exceptions = 0;
    idt_stats.irqs = 0;
    idt_stats.async_messages_sent = 0;
    idt_stats.last_interrupt = 0;
    idt_stats.last_interrupt_time = 0;
    
    // Set up IDT pointer
    idt_pointer.limit = (sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS) - 1;
    idt_pointer.base = (uint32_t)&idt_table;
    
    // Install CPU exception handlers (0-31)
    kprintf("[IDT] Installing CPU exception handlers...\n");
    idt_set_gate(0, (uint32_t)isr0, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);   // Division by zero
    idt_set_gate(1, (uint32_t)isr1, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);   // Debug
    idt_set_gate(2, (uint32_t)isr2, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);   // NMI
    idt_set_gate(3, (uint32_t)isr3, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);   // Breakpoint
    idt_set_gate(4, (uint32_t)isr4, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);   // Overflow
    idt_set_gate(5, (uint32_t)isr5, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);   // Bound range exceeded
    idt_set_gate(6, (uint32_t)isr6, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);   // Invalid opcode
    idt_set_gate(7, (uint32_t)isr7, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);   // Device not available
    idt_set_gate(8, (uint32_t)isr8, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);   // Double fault
    idt_set_gate(9, (uint32_t)isr9, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);   // Coprocessor overrun
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Invalid TSS
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Segment not present
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Stack fault
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // General protection
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Page fault
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Reserved
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // FPU error
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Alignment check
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Machine check
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // SIMD exception
    
    // Reserved exceptions (20-31)
    for (int i = 20; i < 32; i++) {
        idt_set_gate(i, (uint32_t)isr31, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);
    }
    
    // Install IRQ handlers (32-47)
    kprintf("[IDT] Installing IRQ handlers...\n");
    idt_set_gate(32, (uint32_t)irq0, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);  // Timer
    idt_set_gate(33, (uint32_t)irq1, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);  // Keyboard
    idt_set_gate(34, (uint32_t)irq2, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);  // Cascade
    idt_set_gate(35, (uint32_t)irq3, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);  // COM2/COM4
    idt_set_gate(36, (uint32_t)irq4, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);  // COM1/COM3
    idt_set_gate(37, (uint32_t)irq5, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);  // LPT2/Sound
    idt_set_gate(38, (uint32_t)irq6, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);  // Floppy
    idt_set_gate(39, (uint32_t)irq7, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);  // LPT1
    idt_set_gate(40, (uint32_t)irq8, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);  // RTC
    idt_set_gate(41, (uint32_t)irq9, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32);  // ACPI SCI
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Available
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Available
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // PS/2 Mouse
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // FPU
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Primary ATA
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_FLAG_PRESENT | IDT_TYPE_INTERRUPT_32); // Secondary ATA
    
    // Initialize PIC (Programmable Interrupt Controller)
    pic_init();
    
    // Load the IDT
    idt_load();
    
    kprintf("[IDT] IDT installed with %d descriptors\n", IDT_MAX_DESCRIPTORS);
    kprintf("[IDT] Exception handlers: 0-31\n");
    kprintf("[IDT] IRQ handlers: 32-47\n");
    kprintf("[IDT] Ready for async interrupt processing\n");
}

/*
 * Set an IDT gate entry
 */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags)
{
    idt_table[num].offset_low = handler & 0xFFFF;
    idt_table[num].selector = selector;
    idt_table[num].reserved = 0;
    idt_table[num].type_attributes = flags;
    idt_table[num].offset_high = (handler >> 16) & 0xFFFF;
}

/*
 * Load IDT using assembly
 */
void idt_load(void)
{
    asm volatile ("lidt (%0)" : : "r" (&idt_pointer));
}

// =============================================================================
// Handler Registration (Async-Aware)  
// =============================================================================

bool idt_register_handler(uint8_t interrupt_number, 
                         void (*handler)(interrupt_frame_t*),
                         uint32_t target_actor_id,
                         bool async_processing)
{
    if (interrupt_number >= IDT_MAX_DESCRIPTORS) {
        return false;
    }
    
    if (registered_handlers[interrupt_number].handler != 0) {
        kprintf("[IDT] Warning: Replacing existing handler for interrupt %d\n", interrupt_number);
    }
    
    registered_handlers[interrupt_number].interrupt_number = interrupt_number;
    registered_handlers[interrupt_number].handler = handler;
    registered_handlers[interrupt_number].target_actor_id = target_actor_id;
    registered_handlers[interrupt_number].async_processing = async_processing;
    
    return true;
}

bool idt_unregister_handler(uint8_t interrupt_number)
{
    if (interrupt_number >= IDT_MAX_DESCRIPTORS) {
        return false;
    }
    
    registered_handlers[interrupt_number].handler = 0;
    registered_handlers[interrupt_number].target_actor_id = 0;
    registered_handlers[interrupt_number].async_processing = false;
    
    return true;
}

// =============================================================================
// Interrupt Control
// =============================================================================

void interrupts_enable(void)
{
    asm volatile ("sti");
}

void interrupts_disable(void)
{
    asm volatile ("cli");
}

bool interrupts_enabled(void)
{
    uint32_t flags;
    asm volatile ("pushf; pop %0" : "=r" (flags));
    return (flags & (1 << 9)) != 0; // IF flag
}

// =============================================================================
// Exception Handlers
// =============================================================================

void exception_handler(interrupt_frame_t* frame)
{
    idt_stats.exceptions++;
    idt_stats.total_interrupts++;
    idt_stats.last_interrupt = frame->interrupt_number;
    
    const char* exception_messages[] = {
        "Division By Zero",
        "Debug Exception", 
        "Non Maskable Interrupt",
        "Breakpoint Exception",
        "Into Detected Overflow",
        "Out of Bounds Exception",
        "Invalid Opcode Exception",
        "No Coprocessor Exception",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Bad TSS",
        "Segment Not Present",
        "Stack Fault",
        "General Protection Fault",
        "Page Fault",
        "Unknown Interrupt Exception",
        "Coprocessor Fault",
        "Alignment Check Exception",
        "Machine Check Exception",
        "SIMD Floating-Point Exception"
    };
    
    vga_set_color(VGA_COLOR_RED);
    kprintf("\n*** CPU EXCEPTION ***\n");
    kprintf("Exception: %s (%d)\n", 
            frame->interrupt_number < 20 ? exception_messages[frame->interrupt_number] : "Reserved",
            frame->interrupt_number);
    kprintf("Error Code: 0x%x\n", frame->error_code);
    kprintf("EIP: 0x%x, CS: 0x%x, EFLAGS: 0x%x\n", frame->eip, frame->cs, frame->eflags);
    kprintf("EAX: 0x%x, EBX: 0x%x, ECX: 0x%x, EDX: 0x%x\n", 
            frame->eax, frame->ebx, frame->ecx, frame->edx);
    vga_set_color(VGA_COLOR_WHITE);
    
    // Send to AI supervisor for analysis (if enabled)
    if (kernel_state.ai_supervisor_active) {
        interrupt_message_t msg = {
            .interrupt_number = frame->interrupt_number,
            .error_code = frame->error_code,
            .timestamp = 0, // TODO: Get real timestamp
            .cpu_id = 0,    // TODO: Get CPU ID
            .context_data = frame
        };
        
        // TODO: Send to AI supervisor actor
        // interrupt_send_to_actor(AI_SUPERVISOR_ACTOR_ID, &msg);
    }
    
    // For now, halt on exceptions (later: attempt recovery)
    kprintf("[EXCEPTION] System halted for safety\n");
    while (1) {
        asm volatile ("hlt");
    }
}

void page_fault_handler(interrupt_frame_t* frame)
{
    uint32_t fault_address;
    asm volatile ("mov %%cr2, %0" : "=r" (fault_address));
    
    kprintf("[PAGE_FAULT] Virtual address: 0x%x\n", fault_address);
    kprintf("[PAGE_FAULT] Error code: 0x%x\n", frame->error_code);
    
    // TODO: Implement intelligent page fault handling
    // - Demand paging
    // - Copy-on-write
    // - Stack expansion
    // - Swapping
    
    exception_handler(frame);
}

void general_protection_fault_handler(interrupt_frame_t* frame)
{
    kprintf("[GPF] General Protection Fault detected\n");
    kprintf("[GPF] Segment selector: 0x%x\n", frame->error_code);
    
    // TODO: Analyze GPF cause and attempt recovery
    exception_handler(frame);
}

// =============================================================================  
// IRQ Handlers
// =============================================================================

void irq_handler(interrupt_frame_t* frame)
{
    idt_stats.irqs++;
    idt_stats.total_interrupts++;
    idt_stats.last_interrupt = frame->interrupt_number;
    
    uint8_t irq_number = frame->interrupt_number - IRQ_BASE;
    
    // Check if we have a registered handler
    if (registered_handlers[frame->interrupt_number].handler) {
        if (registered_handlers[frame->interrupt_number].async_processing) {
            // Process via async actor system
            interrupt_message_t msg = {
                .interrupt_number = frame->interrupt_number,
                .error_code = frame->error_code,
                .timestamp = 0, // TODO: Get timestamp
                .cpu_id = 0,    // TODO: Get CPU ID
                .context_data = frame
            };
            
            interrupt_send_to_actor(registered_handlers[frame->interrupt_number].target_actor_id, &msg);
            idt_stats.async_messages_sent++;
        } else {
            // Direct call (for performance-critical interrupts)
            registered_handlers[frame->interrupt_number].handler(frame);
        }
    } else {
        // Default IRQ handling
        switch (irq_number) {
            case IRQ0_TIMER:
                timer_irq_handler(frame);
                break;
            case IRQ1_KEYBOARD:
                keyboard_irq_handler(frame);
                break;
            default:
                kprintf("[IRQ] Unhandled IRQ %d\n", irq_number);
                break;
        }
    }
    
    // Send End of Interrupt (EOI) to PIC
    pic_send_eoi(irq_number);
}

void timer_irq_handler(interrupt_frame_t* frame)
{
    static uint32_t timer_ticks = 0;
    timer_ticks++;
    
    // Update kernel uptime every second (assuming 100Hz timer)
    if (timer_ticks % 100 == 0) {
        kernel_state.uptime = timer_ticks / 100;
    }
    
    // Trigger scheduler (cooperative multitasking point)
    // TODO: Send message to scheduler actor
    // scheduler_yield();
}

void keyboard_irq_handler(interrupt_frame_t* frame)
{
    uint8_t scancode = inb(0x60); // Read keyboard scancode
    
    // TODO: Send scancode to keyboard driver actor
    // For now, just acknowledge the interrupt
    kprintf("[KEYBOARD] Scancode: 0x%x\n", scancode);
}

// =============================================================================
// Async Integration  
// =============================================================================

void interrupt_send_to_actor(uint32_t actor_id, interrupt_message_t* msg)
{
    // TODO: Implement actual actor message sending
    // This is where CLKernel's async magic happens!
    // actor_send_message(actor_id, msg, sizeof(interrupt_message_t));
    
    kprintf("[ASYNC] Would send interrupt %d to actor %d\n", msg->interrupt_number, actor_id);
}

void interrupt_process_async_queue(void)
{
    // TODO: Process pending interrupt messages from actor system
    // This function would be called by the main kernel loop
}

// =============================================================================
// Debug and Statistics
// =============================================================================

void idt_print_stats(void)
{
    kprintf("[IDT] Interrupt Statistics:\n");
    kprintf("      Total interrupts: %d\n", (uint32_t)idt_stats.total_interrupts);
    kprintf("      CPU exceptions: %d\n", (uint32_t)idt_stats.exceptions);  
    kprintf("      Hardware IRQs: %d\n", (uint32_t)idt_stats.irqs);
    kprintf("      Async messages sent: %d\n", (uint32_t)idt_stats.async_messages_sent);
    kprintf("      Last interrupt: %d\n", idt_stats.last_interrupt);
}

void idt_dump_table(void)
{
    kprintf("[IDT] Descriptor Table Dump:\n");
    for (int i = 0; i < 32; i++) {
        if (idt_table[i].type_attributes & IDT_FLAG_PRESENT) {
            uint32_t handler = (idt_table[i].offset_high << 16) | idt_table[i].offset_low;
            kprintf("      [%d] Handler: 0x%x, Selector: 0x%x, Flags: 0x%x\n", 
                    i, handler, idt_table[i].selector, idt_table[i].type_attributes);
        }
    }
}
