/*
 * =============================================================================
 * CLKernel - GDT (Global Descriptor Table) Management
 * =============================================================================
 * File: gdt.c
 * Purpose: Setup and manage the Global Descriptor Table
 * =============================================================================
 */

#include "gdt.h"

// GDT entries
static gdt_entry_t gdt_entries[5];
static gdt_ptr_t gdt_ptr;

/*
 * Initialize the Global Descriptor Table
 */
void gdt_init(void)
{
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;
    
    // Null segment (index 0)
    gdt_set_gate(0, 0, 0, 0, 0);
    
    // Code segment (index 1) - 0x08
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    // Data segment (index 2) - 0x10
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    // User mode code segment (index 3) - 0x18
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    // User mode data segment (index 4) - 0x20
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    gdt_flush((uint32_t)&gdt_ptr);
}

/*
 * Set a GDT gate
 */
void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;
    
    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

/*
 * Assembly function to flush the GDT
 */
void gdt_flush(uint32_t gdt_ptr_addr)
{
    asm volatile (
        "lgdt (%0)\n\t"         // Load GDT
        "mov $0x10, %%ax\n\t"   // Data segment selector
        "mov %%ax, %%ds\n\t"    // Load data segments
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        "ljmp $0x08, $1f\n\t"   // Far jump to reload CS
        "1:\n\t"                 // Local label
        :
        : "r" (gdt_ptr_addr)
        : "eax"
    );
}
