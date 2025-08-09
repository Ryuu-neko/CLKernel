// CLKernel - Programmable Interval Timer (PIT) Driver
// Revolutionary OS Architecture - System Timer Management

#include <stdint.h>

// PIT I/O ports
#define PIT_DATA0    0x40    // Channel 0 data port
#define PIT_DATA1    0x41    // Channel 1 data port  
#define PIT_DATA2    0x42    // Channel 2 data port
#define PIT_COMMAND  0x43    // Command register

// PIT frequency (1.193182 MHz)
#define PIT_FREQUENCY 1193182

// External functions
extern void kprintf_color(const char* str, uint8_t color);

// Outbound port function
static inline void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port) : "memory");
}

// Initialize PIT timer
void pit_init(uint32_t frequency) {
    // Calculate divisor for desired frequency
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    // Send command byte (Channel 0, lobyte/hibyte, rate generator)
    outb(PIT_COMMAND, 0x36);
    
    // Send divisor
    outb(PIT_DATA0, divisor & 0xFF);        // Low byte
    outb(PIT_DATA0, (divisor >> 8) & 0xFF); // High byte
    
    kprintf_color("[✓] PIT initialized at ", 0x0A);
    
    // Simple frequency display
    if (frequency == 100) {
        kprintf_color("100Hz (10ms intervals)\n", 0x0E);
    } else if (frequency == 1000) {
        kprintf_color("1000Hz (1ms intervals)\n", 0x0E);
    } else {
        kprintf_color("custom frequency\n", 0x0E);
    }
}

// Initialize IRQ system (8259 PIC)
void pic_init(void) {
    // Send ICW1 - Initialize command
    outb(0x20, 0x11); // Master PIC
    outb(0xA0, 0x11); // Slave PIC
    
    // Send ICW2 - Remap IRQs
    outb(0x21, 0x20); // Master PIC starts at interrupt 32
    outb(0xA1, 0x28); // Slave PIC starts at interrupt 40
    
    // Send ICW3 - Wire master and slave
    outb(0x21, 0x04); // Tell master PIC that slave is at IRQ2
    outb(0xA1, 0x02); // Tell slave PIC its cascade identity
    
    // Send ICW4 - Set mode
    outb(0x21, 0x01); // 8086/88 mode for master
    outb(0xA1, 0x01); // 8086/88 mode for slave
    
    // Mask all interrupts except timer (IRQ0)
    outb(0x21, 0xFE); // Master PIC: Enable only IRQ0 (timer)
    outb(0xA1, 0xFF); // Slave PIC: Mask all
    
    kprintf_color("[✓] PIC remapped (IRQ 0-15 → INT 32-47)\n", 0x0A);
}
