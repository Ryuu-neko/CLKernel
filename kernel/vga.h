/*
 * =============================================================================
 * CLKernel - VGA Display Header
 * =============================================================================
 * File: vga.h
 * Purpose: VGA display function declarations and constants
 * =============================================================================
 */

#ifndef VGA_H
#define VGA_H

#include <stdint.h>

// VGA color constants
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
#define VGA_COLOR_LIGHT_BROWN   14
#define VGA_COLOR_YELLOW        14   // Alias for light brown
#define VGA_COLOR_WHITE         15

// Function prototypes
void vga_clear_screen(void);
void vga_set_color(uint8_t color);
void vga_putchar_at(char c, uint8_t row, uint8_t col, uint8_t color);
void vga_putchar(char c);
void vga_scroll(void);
void vga_update_cursor(void);
void kputs(const char* str);
int kprintf(const char* format, ...);

#endif // VGA_H
