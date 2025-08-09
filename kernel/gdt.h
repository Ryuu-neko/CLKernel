/*
 * =============================================================================
 * CLKernel - GDT Header
 * =============================================================================
 * File: gdt.h
 * Purpose: GDT structures and function declarations
 * =============================================================================
 */

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT entry structure
typedef struct {
    uint16_t limit_low;      // Lower 16 bits of limit
    uint16_t base_low;       // Lower 16 bits of base
    uint8_t  base_middle;    // Next 8 bits of base
    uint8_t  access;         // Access byte
    uint8_t  granularity;    // Upper 4 bits of limit + flags
    uint8_t  base_high;      // Upper 8 bits of base
} __attribute__((packed)) gdt_entry_t;

// GDT pointer structure
typedef struct {
    uint16_t limit;          // Upper 16 bits of all selector limits
    uint32_t base;           // Address of the first gdt_entry_t struct
} __attribute__((packed)) gdt_ptr_t;

// Function prototypes
void gdt_init(void);
void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void gdt_flush(uint32_t gdt_ptr_addr);

#endif // GDT_H
