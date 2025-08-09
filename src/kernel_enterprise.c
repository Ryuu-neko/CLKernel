/*
 * CLKernel - Enterprise Interrupt-Driven Kernel Entry Point
 * Phase 1: IDT + Timer Interrupt Implementation
 */
#include <stdint.h>

// VGA Constants
#define VGA_MEMORY_BASE 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Color constants
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1  
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW        14
#define VGA_COLOR_WHITE         15

// Global state
static volatile uint32_t system_ticks = 0;
static volatile uint32_t uptime_seconds = 0;
static int cursor_x = 0, cursor_y = 0;
volatile uint16_t* vga_buffer = (volatile uint16_t*)VGA_MEMORY_BASE;

// Function prototypes
void idt_init(void);
void pit_init(void);
void pic_init(void);
void timer_handler(void);

// VGA utility functions
uint8_t vga_make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

void vga_clear_screen(void) {
    uint16_t blank = (vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE) << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    cursor_x = 0;
    cursor_y = 0;
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
        if (cursor_y >= VGA_HEIGHT) {
            cursor_y = VGA_HEIGHT - 1;
        }
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

// I/O port functions
static inline void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    asm volatile("inb %1, %0" : "=a"(data) : "Nd"(port) : "memory");
    return data;
}

// Update uptime display
void update_uptime_display(void) {
    // Simple uptime display - format: "UP:XXs"
    char uptime_str[16];
    uptime_str[0] = 'U'; uptime_str[1] = 'P'; uptime_str[2] = ':';
    
    uint32_t secs = uptime_seconds % 100; // Show 0-99 seconds
    uptime_str[3] = '0' + (secs / 10);
    uptime_str[4] = '0' + (secs % 10);
    uptime_str[5] = 's';
    uptime_str[6] = '\0';
    
    kprintf_at(uptime_str, 72, 0, vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
}

// Timer interrupt handler (called from assembly)
void timer_handler(void) {
    system_ticks++;
    
    // Update uptime every 100 ticks (1 second at 100Hz)
    if (system_ticks % 100 == 0) {
        uptime_seconds++;
        update_uptime_display();
    }
    
    // Send EOI to PIC
    outb(0x20, 0x20);
}

// IDT entry structure
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  reserved;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

// IDT pointer structure  
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t idt_ptr;

// External assembly interrupt handlers
extern void isr32(void);

// Set IDT entry
void idt_set_entry(int num, uint32_t handler, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].selector = sel;
    idt[num].reserved = 0;
    idt[num].type_attr = flags;
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
}

// Initialize IDT
void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    // Clear IDT
    for (int i = 0; i < 256; i++) {
        idt_set_entry(i, 0, 0, 0);
    }
    
    // Set up timer interrupt (IRQ0 -> INT32)
    idt_set_entry(32, (uint32_t)isr32, 0x08, 0x8E);
    
    // Load IDT
    asm volatile("lidt %0" : : "m"(idt_ptr));
    
    kprintf_color("[✓] IDT initialized with timer interrupt\n", 
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
}

// Initialize PIC (Programmable Interrupt Controller)
void pic_init(void) {
    // Remap PIC to avoid conflicts with CPU exceptions
    outb(0x20, 0x11); // ICW1 - Initialize master PIC
    outb(0xA0, 0x11); // ICW1 - Initialize slave PIC
    
    outb(0x21, 0x20); // ICW2 - Master PIC offset (INT 32)
    outb(0xA1, 0x28); // ICW2 - Slave PIC offset (INT 40)
    
    outb(0x21, 0x04); // ICW3 - Tell master about slave at IRQ2
    outb(0xA1, 0x02); // ICW3 - Tell slave its cascade identity
    
    outb(0x21, 0x01); // ICW4 - 8086 mode for master
    outb(0xA1, 0x01); // ICW4 - 8086 mode for slave
    
    // Unmask timer interrupt (IRQ0) only
    outb(0x21, 0xFE); // Enable IRQ0 only
    outb(0xA1, 0xFF); // Mask all slave interrupts
    
    kprintf_color("[✓] PIC initialized and remapped\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
}

// Initialize PIT (Programmable Interval Timer) 
void pit_init(void) {
    uint32_t divisor = 1193182 / 100; // 100Hz frequency
    
    outb(0x43, 0x36); // Command: channel 0, low/high byte, rate generator
    outb(0x40, divisor & 0xFF);        // Low byte
    outb(0x40, (divisor >> 8) & 0xFF); // High byte
    
    kprintf_color("[✓] PIT initialized at 100Hz\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
}

// Main kernel entry point
void kernel_main(void) {
    // Clear screen with blue background
    vga_clear_screen();
    
    // Display kernel banner
    kprintf_color("  ██████ ██      ██   ██ ███████ ██████  ███    ██ ███████ ██      \n", 
                  vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    kprintf_color(" ██      ██      ██  ██  ██      ██   ██ ████   ██ ██      ██      \n",
                  vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    kprintf_color(" ██      ██      █████   █████   ██████  ██ ██  ██ █████   ██      \n",
                  vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    kprintf_color(" ██      ██      ██  ██  ██      ██   ██ ██  ██ ██ ██      ██      \n",
                  vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    kprintf_color("  ██████ ███████ ██   ██ ███████ ██   ██ ██   ████ ███████ ███████ \n",
                  vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    
    kprintf_color("========================================================================\n",
                  vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    kprintf_color("                ENTERPRISE INTERRUPT-DRIVEN KERNEL v1.0                \n",
                  vga_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE));
    kprintf_color("========================================================================\n\n",
                  vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    kprintf_color("🚀 PHASE 1: CORE FUNCTIONALITY IMPLEMENTATION\n\n",
                  vga_make_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLUE));
    
    // Initialize interrupt system
    kprintf_color("[INIT] Setting up interrupt system...\n",
                  vga_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE));
    
    idt_init();
    pic_init();
    pit_init();
    
    kprintf_color("\n[READY] Interrupt system initialized!\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("[READY] Timer interrupt active at 100Hz\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("[READY] Watch uptime counter (top-right)!\n\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    
    kprintf_color("*** ENTERPRISE KERNEL IS NOW INTERRUPT-DRIVEN! ***\n",
                  vga_make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLUE));
    
    // Enable interrupts - THE BIG MOMENT!
    asm volatile("sti");
    
    kprintf_color("*** INTERRUPTS ENABLED - KERNEL IS ALIVE! ***\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    
    // Kernel main loop - now interrupt driven!
    while (1) {
        asm volatile("hlt"); // Wait for interrupts
    }
}
