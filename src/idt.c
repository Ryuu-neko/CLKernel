// CLKernel - Interrupt Descriptor Table (IDT) Management
// Revolutionary OS Architecture - Interrupt Handling System

#include <stdint.h>

// IDT Entry structure (64-bit per entry in x86-32)
typedef struct {
    uint16_t offset_low;    // Lower 16 bits of handler address
    uint16_t selector;      // Code segment selector
    uint8_t  reserved;      // Always 0
    uint8_t  type_attr;     // Type and attributes
    uint16_t offset_high;   // Higher 16 bits of handler address
} __attribute__((packed)) idt_entry_t;

// IDT Pointer structure
typedef struct {
    uint16_t limit;         // Size of IDT - 1
    uint32_t base;          // Address of IDT
} __attribute__((packed)) idt_ptr_t;

// IDT with 256 entries
static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

// External assembly interrupt handlers
extern void isr0(void);   // Division by zero
extern void isr32(void);  // Timer interrupt (IRQ0)

// Timer variables
static volatile uint32_t timer_ticks = 0;
static volatile uint32_t uptime_seconds = 0;

// VGA functions (from vga.c)
extern void vga_set_cursor(int x, int y);
extern void kprintf_color(const char* str, uint8_t color);
extern void kprintf_at(const char* str, int x, int y, uint8_t color);

// Forward declarations
void update_uptime_display(void);

// Set an IDT entry
static void idt_set_entry(int num, uint32_t handler, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].selector = sel;
    idt[num].reserved = 0;
    idt[num].type_attr = flags;
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
}

// Initialize IDT
void idt_init(void) {
    // Set up IDT pointer
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    // Clear IDT
    for (int i = 0; i < 256; i++) {
        idt_set_entry(i, 0, 0, 0);
    }
    
    // Set up exception handlers
    idt_set_entry(0, (uint32_t)isr0, 0x08, 0x8E);   // Division by zero
    
    // Set up IRQ handlers (remapped to 32-47)
    idt_set_entry(32, (uint32_t)isr32, 0x08, 0x8E); // Timer (IRQ0)
    
    // Load IDT
    asm volatile("lidt %0" : : "m"(idt_ptr));
    
    kprintf_color("[✓] IDT initialized with 256 entries\n", 0x0A); // Green
}

// Timer interrupt handler (called from assembly)
void timer_handler(void) {
    timer_ticks++;
    
    // Update uptime every 100 ticks (assuming 100Hz timer)
    if (timer_ticks % 100 == 0) {
        uptime_seconds++;
        update_uptime_display();
    }
    
    // Send EOI (End of Interrupt) to PIC
    asm volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20) : "memory");
}

// Update uptime display on screen
void update_uptime_display(void) {
    char uptime_str[32];
    
    // Format uptime string
    uint32_t hours = uptime_seconds / 3600;
    uint32_t minutes = (uptime_seconds % 3600) / 60;
    uint32_t seconds = uptime_seconds % 60;
    
    // Simple sprintf implementation
    uptime_str[0] = 'U'; uptime_str[1] = 'p'; uptime_str[2] = 't'; 
    uptime_str[3] = 'i'; uptime_str[4] = 'm'; uptime_str[5] = 'e'; 
    uptime_str[6] = ':'; uptime_str[7] = ' ';
    
    // Hours
    uptime_str[8] = '0' + (hours / 10);
    uptime_str[9] = '0' + (hours % 10);
    uptime_str[10] = ':';
    
    // Minutes  
    uptime_str[11] = '0' + (minutes / 10);
    uptime_str[12] = '0' + (minutes % 10);
    uptime_str[13] = ':';
    
    // Seconds
    uptime_str[14] = '0' + (seconds / 10);
    uptime_str[15] = '0' + (seconds % 10);
    uptime_str[16] = '\0';
    
    // Display at top-right corner
    kprintf_at(uptime_str, 62, 0, 0x1F); // White on blue
}

// General exception handler
void exception_handler(uint32_t int_no) {
    kprintf_color("EXCEPTION: ", 0x0C); // Red
    
    switch(int_no) {
        case 0:
            kprintf_color("Division by Zero!\n", 0x0C);
            break;
        default:
            kprintf_color("Unknown Exception!\n", 0x0C);
            break;
    }
    
    // Halt system
    for(;;) {
        asm volatile("hlt");
    }
}
