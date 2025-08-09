/*
 * =============================================================================
 * CLKernel - Hardware I/O Functions Header
 * =============================================================================
 * File: io.h
 * Purpose: Hardware I/O port access function declarations
 * =============================================================================
 */

#ifndef IO_H
#define IO_H

#include <stdint.h>

// Function prototypes
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void outw(uint16_t port, uint16_t value);
uint16_t inw(uint16_t port);

#endif // IO_H
