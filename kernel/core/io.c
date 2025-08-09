/*
 * =============================================================================
 * CLKernel - Hardware I/O Functions
 * =============================================================================
 * File: io.c
 * Purpose: Hardware I/O port access functions
 * =============================================================================
 */

#include <stdint.h>

/*
 * Output a byte to an I/O port
 */
void outb(uint16_t port, uint8_t value)
{
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

/*
 * Input a byte from an I/O port
 */
uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

/*
 * Output a word to an I/O port
 */
void outw(uint16_t port, uint16_t value)
{
    asm volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

/*
 * Input a word from an I/O port
 */
uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}
