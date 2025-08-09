/*
 * CLKernel Enterprise - All-in-One Interrupt-Driven Kernel
 * Phase 1: Complete IDT + Timer implementation in single file
 */
#include <stdint.h>

// VGA and system state
#define VGA_MEMORY_BASE 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Colors
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW 14
#define VGA_COLOR_WHITE 15

volatile uint16_t* vga_buffer = (volatile uint16_t*)VGA_MEMORY_BASE;
static volatile uint32_t system_ticks = 0;
static volatile uint32_t uptime_seconds = 0;
static int cursor_x = 0, cursor_y = 0;

// IDT structures
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  reserved;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

// Utility functions
static inline void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port) : "memory");
}

uint8_t vga_make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

void vga_putchar(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c >= 32) {
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
        if (cursor_y >= VGA_HEIGHT) cursor_y = VGA_HEIGHT - 1;
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | c;
        cursor_x++;
    }
}

void kprintf_color(const char* str, uint8_t color) {
    while (*str) {
        vga_putchar(*str, color);
        str++;
    }
}

void kprintf_at(const char* str, int x, int y, uint8_t color) {
    for (int i = 0; str[i] && (x + i) < VGA_WIDTH; i++) {
        if (y >= 0 && y < VGA_HEIGHT) {
            vga_buffer[y * VGA_WIDTH + (x + i)] = (color << 8) | str[i];
        }
    }
}

// Timer interrupt handler
void timer_handler(void) {
    system_ticks++;
    
    if (system_ticks % 100 == 0) {
        uptime_seconds++;
        
        // Update uptime display
        char uptime_str[16];
        uptime_str[0] = 'U'; uptime_str[1] = 'P'; uptime_str[2] = ':';
        uint32_t secs = uptime_seconds % 100;
        uptime_str[3] = '0' + (secs / 10);
        uptime_str[4] = '0' + (secs % 10);
        uptime_str[5] = 's'; uptime_str[6] = '\0';
        
        kprintf_at(uptime_str, 72, 0, vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    }
    
    // Send EOI to PIC
    outb(0x20, 0x20);
}

// Inline assembly timer interrupt handler
void timer_interrupt_handler(void) {
    asm volatile (
        "cli\n\t"              // Disable interrupts
        "pusha\n\t"            // Save registers
        "call timer_handler\n\t" // Call C handler
        "popa\n\t"             // Restore registers  
        "sti\n\t"              // Enable interrupts
        "iret"                 // Return from interrupt
    );
}

// Set IDT entry
void idt_set_entry(int num, void* handler, uint16_t sel, uint8_t flags) {
    uint32_t handler_addr = (uint32_t)handler;
    idt[num].offset_low = handler_addr & 0xFFFF;
    idt[num].selector = sel;
    idt[num].reserved = 0;
    idt[num].type_attr = flags;
    idt[num].offset_high = (handler_addr >> 16) & 0xFFFF;
}

// Initialize systems
void init_systems(void) {
    // Clear screen with blue background
    uint16_t blank = (vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE) << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    cursor_x = cursor_y = 0;
    
    // Initialize IDT
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    // Clear IDT
    for (int i = 0; i < 256; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].reserved = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }
    
    // Set timer interrupt (IRQ0 -> INT32)
    idt_set_entry(32, (void*)timer_interrupt_handler, 0x08, 0x8E);
    
    // Load IDT
    asm volatile("lidt %0" : : "m"(idt_ptr));
    
    // Initialize PIC
    outb(0x20, 0x11); outb(0xA0, 0x11); // ICW1
    outb(0x21, 0x20); outb(0xA1, 0x28); // ICW2 - remap
    outb(0x21, 0x04); outb(0xA1, 0x02); // ICW3
    outb(0x21, 0x01); outb(0xA1, 0x01); // ICW4
    outb(0x21, 0xFE); outb(0xA1, 0xFF); // Enable IRQ0 only
    
    // Initialize PIT
    uint32_t divisor = 1193182 / 100; // 100Hz
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

// Main kernel entry
void kernel_main(void) {
    init_systems();
    
    // Display banner
    kprintf_color("  ██████ ██      ██   ██ ███████ ██████  ███    ██ ███████ ██      \n", 
                  vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    kprintf_color("========================================================================\n",
                  vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    kprintf_color("                ENTERPRISE INTERRUPT-DRIVEN KERNEL v1.0                \n",
                  vga_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE));
    kprintf_color("========================================================================\n\n",
                  vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    kprintf_color("🚀 PHASE 1: CORE FUNCTIONALITY IMPLEMENTATION\n\n",
                  vga_make_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLUE));
    
    kprintf_color("[✓] IDT initialized\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("[✓] PIC remapped and configured\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("[✓] PIT configured at 100Hz\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    
    kprintf_color("\n*** ENABLING INTERRUPTS - KERNEL GOES LIVE! ***\n",
                  vga_make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLUE));
    
    // THE BIG MOMENT - Enable interrupts!
    asm volatile("sti");
    
    kprintf_color("*** ENTERPRISE KERNEL IS INTERRUPT-DRIVEN! ***\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("Watch uptime counter (top-right) - Kernel is ALIVE!\n\n",
                  vga_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE));
    
    // Main kernel loop
    while (1) {
        asm volatile("hlt"); // Wait for interrupts
    }
}
