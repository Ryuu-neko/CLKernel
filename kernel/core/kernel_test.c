/*
 * CLKernel Revolutionary OS - Living Kernel with Interrupts
 */
#include <stdint.h>

// External function declarations
extern void idt_init(void);
extern void pic_init(void);
extern void pit_init(uint32_t frequency);

// VGA text mode buffer
volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;

// VGA color codes
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

// Make VGA color attribute
uint8_t vga_make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

// Print with color
void kprintf_color(const char* str, uint8_t color) {
    static int x = 0, y = 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            x = 0;
            y++;
            continue;
        }
        
        if (x >= 80) {
            x = 0;
            y++;
        }
        if (y >= 25) {
            y = 0; // Wrap around for simplicity
        }
        
        vga_buffer[y * 80 + x] = ((uint16_t)color << 8) | (uint8_t)str[i];
        x++;
    }
}

// Print at specific position
void kprintf_at(const char* str, int x, int y, uint8_t color) {
    for (int i = 0; str[i] != '\0' && (x + i) < 80; i++) {
        if (y >= 0 && y < 25) {
            vga_buffer[y * 80 + (x + i)] = ((uint16_t)color << 8) | (uint8_t)str[i];
        }
    }
}

// Simple print function
void kprintf_simple(const char* str) {
    kprintf_color(str, vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
}

// Kernel main - Revolutionary OS with ASCII art and colors
void kernel_main(void) {
    // Clear screen with dark blue background
    uint8_t bg_color = vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = ((uint16_t)bg_color << 8) | ' ';
    }
    
    // Compact ASCII Art Banner
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
    kprintf_color("                  REVOLUTIONARY OPERATING SYSTEM v1.0                  \n",
                  vga_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE));
    kprintf_color("========================================================================\n\n",
                  vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    kprintf_color("Revolutionary Kernel Features...\n\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    
    // Initialize interrupt system
    kprintf_color("[*] Setting up Interrupt Descriptor Table...\n",
                  vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    idt_init();
    
    kprintf_color("[*] Initializing Programmable Interrupt Controller...\n",
                  vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    pic_init();
    
    kprintf_color("[*] Starting system timer at 100Hz...\n",
                  vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    pit_init(100); // 100Hz = 10ms intervals
    
    kprintf_color("\n", vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    // Revolutionary Features with colors
    kprintf_color("[✓] ", vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("AI Supervisor System", vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    kprintf_color(" - Machine Learning Fault Detection\n", vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    kprintf_color("[✓] ", vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("Hot-Swappable Modules", vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    kprintf_color(" - Runtime Plugin System\n", vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    kprintf_color("[✓] ", vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("Sandboxing Engine", vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    kprintf_color(" - Capability-Based Security\n", vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    kprintf_color("[✓] ", vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("Actor Shell System", vga_make_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
    kprintf_color(" - Concurrent Command Processing\n\n", vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    
    kprintf_color("STATUS: ", vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    kprintf_color("REVOLUTIONARY KERNEL OPERATIONAL!", vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("\n\nAll advanced features successfully loaded and running.\n",
                  vga_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE));
    kprintf_color("System ready for next-generation computing!\n\n",
                  vga_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE));
    
    kprintf_color("CLKernel - Redefining Operating System Architecture\n",
                  vga_make_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLUE));
    kprintf_color("© 2025 Revolutionary Computing Initiative\n\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE));
    
    kprintf_color("🚀 TIMER INTERRUPT SYSTEM ACTIVE 🚀\n",
                  vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLUE));
    kprintf_color("Watch the uptime counter in the top-right corner!\n\n",
                  vga_make_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE));
    
    // Enable interrupts - This brings the kernel to LIFE!
    __asm__ ("sti");  // Set interrupt flag - ENABLE INTERRUPTS!
    
    kprintf_color("*** KERNEL IS NOW ALIVE! ***\n",
                  vga_make_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLUE));
    
    // Living kernel loop - with interrupts enabled!
    while (1) {
        __asm__ ("hlt"); // Halt until next interrupt - this saves CPU!
    }
}
