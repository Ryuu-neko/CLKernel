/*
 * =============================================================================
 * CLKernel - PIC (Programmable Interrupt Controller) Implementation
 * =============================================================================  
 * File: pic.c
 * Purpose: 8259 PIC initialization and IRQ management
 * 
 * The PIC must be properly configured for modern systems to avoid conflicts
 * with CPU exceptions and enable proper IRQ handling for the async system.
 * =============================================================================
 */

#include "pic.h"
#include "kernel.h"
#include "../io.h"
#include "vga.h"

// I/O delay function for slow hardware
static void io_wait(void)
{
    // Use an unused port for delay
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}

/*
 * Initialize the PIC controllers
 */
void pic_init(void)
{
    kprintf("[PIC] Initializing 8259 Programmable Interrupt Controllers...\n");
    
    // Save current masks
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    
    // Start initialization sequence (ICW1)
    outb(PIC1, ICW1_INIT | ICW1_ICW4);  // Master PIC
    io_wait();
    outb(PIC2, ICW1_INIT | ICW1_ICW4);  // Slave PIC
    io_wait();
    
    // ICW2: Vector offsets
    outb(PIC1_DATA, IRQ_BASE);          // Master PIC: IRQs 0-7 -> interrupts 32-39
    io_wait();
    outb(PIC2_DATA, IRQ_BASE + 8);      // Slave PIC: IRQs 8-15 -> interrupts 40-47  
    io_wait();
    
    // ICW3: Tell master about slave at IRQ2, tell slave its cascade identity
    outb(PIC1_DATA, 4);                 // Master: slave at IRQ2 (binary 0100)
    io_wait();
    outb(PIC2_DATA, 2);                 // Slave: cascade identity 2
    io_wait();
    
    // ICW4: 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    // Restore saved masks (but initially mask all except timer and keyboard)
    outb(PIC1_DATA, 0xFC);  // Mask all except IRQ0 (timer) and IRQ1 (keyboard)
    outb(PIC2_DATA, 0xFF);  // Mask all slave IRQs initially
    
    kprintf("[PIC] Master PIC: IRQs 0-7 mapped to interrupts 32-39\n");
    kprintf("[PIC] Slave PIC: IRQs 8-15 mapped to interrupts 40-47\n");
    kprintf("[PIC] Timer (IRQ0) and Keyboard (IRQ1) unmasked\n");
    kprintf("[PIC] PIC initialization complete\n");
}

/*
 * Send End of Interrupt (EOI) signal
 */
void pic_send_eoi(uint8_t irq)
{
    // If IRQ came from slave PIC, send EOI to both
    if (irq >= 8) {
        outb(PIC2, PIC_EOI);
    }
    
    // Always send EOI to master PIC
    outb(PIC1, PIC_EOI);
}

/*
 * Mask (disable) an IRQ
 */
void pic_mask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

/*
 * Unmask (enable) an IRQ
 */
void pic_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
    
    kprintf("[PIC] Unmasked IRQ %d\n", irq);
}

/*
 * Mask all IRQs
 */
void pic_mask_all(void)
{
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    kprintf("[PIC] All IRQs masked\n");
}

/*
 * Get Interrupt Request Register
 */
uint16_t pic_get_irr(void)
{
    // Read IRR from both PICs
    outb(PIC1, 0x0A);  // OCW3: Read IRR
    outb(PIC2, 0x0A);
    
    return (inb(PIC2) << 8) | inb(PIC1);
}

/*
 * Get In-Service Register  
 */
uint16_t pic_get_isr(void)
{
    // Read ISR from both PICs
    outb(PIC1, 0x0B);  // OCW3: Read ISR
    outb(PIC2, 0x0B);
    
    return (inb(PIC2) << 8) | inb(PIC1);
}
