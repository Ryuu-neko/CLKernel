/*
 * =============================================================================
 * CLKernel - VGA Display Driver
 * =============================================================================
 * File: vga.c
 * Purpose: VGA text mode display functionality
 * =============================================================================
 */

#include "vga.h"
#include "../io.h"
#include <stdarg.h>

// VGA hardware constants
#define VGA_MEMORY_BASE     0xB8000
#define VGA_WIDTH           80
#define VGA_HEIGHT          25
#define VGA_BUFFER_SIZE     (VGA_WIDTH * VGA_HEIGHT * 2)

// Current cursor position and color
static uint8_t current_row = 0;
static uint8_t current_col = 0;
static uint8_t current_color = VGA_COLOR_WHITE;

// VGA buffer pointer
static volatile char* vga_buffer = (volatile char*)VGA_MEMORY_BASE;

/*
 * Clear the entire screen
 */
void vga_clear_screen(void)
{
    uint16_t blank = (current_color << 8) | ' ';
    
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        ((volatile uint16_t*)vga_buffer)[i] = blank;
    }
    
    current_row = 0;
    current_col = 0;
    vga_update_cursor();
}

/*
 * Set text color
 */
void vga_set_color(uint8_t color)
{
    current_color = color;
}

/*
 * Put a character at specific position
 */
void vga_putchar_at(char c, uint8_t row, uint8_t col, uint8_t color)
{
    if (row >= VGA_HEIGHT || col >= VGA_WIDTH) {
        return;
    }
    
    int index = (row * VGA_WIDTH + col) * 2;
    vga_buffer[index] = c;
    vga_buffer[index + 1] = color;
}

/*
 * Put character at current cursor position
 */
void vga_putchar(char c)
{
    if (c == '\n') {
        current_col = 0;
        current_row++;
    } else if (c == '\r') {
        current_col = 0;
    } else if (c == '\t') {
        current_col = (current_col + 8) & ~7; // Align to 8-character boundary
    } else if (c >= 32) { // Printable characters
        vga_putchar_at(c, current_row, current_col, current_color);
        current_col++;
    }
    
    // Handle line wrapping
    if (current_col >= VGA_WIDTH) {
        current_col = 0;
        current_row++;
    }
    
    // Handle scrolling
    if (current_row >= VGA_HEIGHT) {
        vga_scroll();
        current_row = VGA_HEIGHT - 1;
    }
    
    vga_update_cursor();
}

/*
 * Scroll screen up by one line
 */
void vga_scroll(void)
{
    // Move all lines up by one
    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            int src_index = (row * VGA_WIDTH + col) * 2;
            int dst_index = ((row - 1) * VGA_WIDTH + col) * 2;
            vga_buffer[dst_index] = vga_buffer[src_index];
            vga_buffer[dst_index + 1] = vga_buffer[src_index + 1];
        }
    }
    
    // Clear the last line
    uint16_t blank = (current_color << 8) | ' ';
    for (int col = 0; col < VGA_WIDTH; col++) {
        ((volatile uint16_t*)vga_buffer)[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = blank;
    }
}

/*
 * Update hardware cursor position
 */
void vga_update_cursor(void)
{
    uint16_t position = current_row * VGA_WIDTH + current_col;
    
    // Send high byte
    outb(0x3D4, 0x0E);
    outb(0x3D5, (position >> 8) & 0xFF);
    
    // Send low byte
    outb(0x3D4, 0x0F);
    outb(0x3D5, position & 0xFF);
}

/*
 * Print string
 */
void kputs(const char* str)
{
    while (*str) {
        vga_putchar(*str);
        str++;
    }
}

/*
 * Convert integer to string
 */
static char* itoa(int value, char* str, int base)
{
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int tmp_value;
    
    // Handle negative numbers (only for base 10)
    if (value < 0 && base == 10) {
        *ptr++ = '-';
        value = -value;
        ptr1++;
    }
    
    // Convert to string
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
    } while (value);
    
    *ptr-- = '\0';
    
    // Reverse string
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    
    return str;
}

/*
 * Kernel printf function
 */
int kprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    char buffer[32];
    int count = 0;
    
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 'd':
                case 'i': {
                    int value = va_arg(args, int);
                    itoa(value, buffer, 10);
                    kputs(buffer);
                    count += 1;
                    break;
                }
                case 'x': {
                    int value = va_arg(args, int);
                    itoa(value, buffer, 16);
                    kputs(buffer);
                    count += 1;
                    break;
                }
                case 's': {
                    char* str = va_arg(args, char*);
                    kputs(str);
                    count += 1;
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    vga_putchar(c);
                    count += 1;
                    break;
                }
                case '%':
                    vga_putchar('%');
                    break;
                default:
                    vga_putchar('%');
                    vga_putchar(*format);
                    break;
            }
        } else {
            vga_putchar(*format);
        }
        format++;
    }
    
    va_end(args);
    return count;
}

/*
 * Print colored string
 */
void kprintf_color(const char* str, uint8_t color)
{
    uint8_t old_color = current_color;
    vga_set_color(color);
    kputs(str);
    vga_set_color(old_color);
}

/*
 * Print string at specific position
 */
void kprintf_at(const char* str, int x, int y, uint8_t color)
{
    if (y >= VGA_HEIGHT || x >= VGA_WIDTH) {
        return;
    }
    
    int i = 0;
    while (str[i] && (x + i) < VGA_WIDTH) {
        vga_putchar_at(str[i], y, x + i, color);
        i++;
    }
}

/*
 * Set cursor position
 */
void vga_set_cursor(int x, int y)
{
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        current_col = x;
        current_row = y;
        vga_update_cursor();
    }
}
