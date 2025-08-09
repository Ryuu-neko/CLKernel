/*
 * =============================================================================
 * CLKernel - PIC (Programmable Interrupt Controller) Header
 * =============================================================================
 * File: pic.h
 * Purpose: 8259 PIC management for IRQ handling
 * =============================================================================
 */

#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// PIC ports
#define PIC1            0x20    // Master PIC command port
#define PIC1_DATA       0x21    // Master PIC data port
#define PIC2            0xA0    // Slave PIC command port  
#define PIC2_DATA       0xA1    // Slave PIC data port

// PIC commands
#define PIC_EOI         0x20    // End of Interrupt
#define ICW1_ICW4       0x01    // ICW4 (not) needed
#define ICW1_SINGLE     0x02    // Single (cascade) mode
#define ICW1_INTERVAL4  0x04    // Call address interval 4 (8)
#define ICW1_LEVEL      0x08    // Level triggered (edge) mode
#define ICW1_INIT       0x10    // Initialization - required!

#define ICW4_8086       0x01    // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO       0x02    // Auto (normal) EOI
#define ICW4_BUF_SLAVE  0x08    // Buffered mode/slave
#define ICW4_BUF_MASTER 0x0C    // Buffered mode/master
#define ICW4_SFNM       0x10    // Special fully nested (not)

// IRQ base offsets
#define IRQ_BASE        32      // IRQs mapped to interrupts 32-47

// Function prototypes
void pic_init(void);
void pic_send_eoi(uint8_t irq);
void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);
void pic_mask_all(void);
uint16_t pic_get_irr(void);
uint16_t pic_get_isr(void);

#endif // PIC_H
