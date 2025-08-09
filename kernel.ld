/*
 * =============================================================================
 * CLKernel Linker Script
 * =============================================================================
 * Purpose: Define memory layout and section organization for the kernel
 * Target: x86_64 (32-bit protected mode initially)
 * 
 * Memory Layout:
 * 0x00000000 - 0x000FFFFF: Low memory (1MB) - Reserved for BIOS/bootloader
 * 0x00100000 - 0x00200000: Kernel code and data (1MB)
 * 0x00200000 - 0x00400000: Kernel heap (2MB)
 * 0x00400000 - 0x00800000: Module loading area (4MB)
 * 0x00800000+            : Available for applications and data
 * =============================================================================
 */

ENTRY(_start)

/* Define memory regions */
MEMORY
{
    /* Kernel is loaded at 1MB (0x100000) for safety */
    KERNEL (rwx) : ORIGIN = 0x100000, LENGTH = 1M
    
    /* Module loading area */
    MODULES (rwx) : ORIGIN = 0x400000, LENGTH = 4M
}

SECTIONS
{
    /* Kernel entry point - must be first */
    .text ALIGN(4K) : AT(ADDR(.text) - 0x100000)
    {
        _kernel_start = .;
        *(.multiboot)       /* Multiboot header (future GRUB support) */
        *(.text.entry)      /* Kernel entry point */
        *(.text)            /* Code sections */
        *(.text.*)
        *(.rodata)          /* Read-only data */
        *(.rodata.*)
        . = ALIGN(4K);
        _kernel_text_end = .;
    } > KERNEL

    /* Initialized data section */
    .data ALIGN(4K) : AT(ADDR(.data) - 0x100000)
    {
        _kernel_data_start = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4K);
        _kernel_data_end = .;
    } > KERNEL

    /* Uninitialized data section */
    .bss ALIGN(4K) : AT(ADDR(.bss) - 0x100000)
    {
        _kernel_bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        . = ALIGN(4K);
        _kernel_bss_end = .;
    } > KERNEL

    /* Stack section */
    .stack ALIGN(4K) : AT(ADDR(.stack) - 0x100000)
    {
        _kernel_stack_bottom = .;
        . = . + 0x4000;     /* 16KB stack */
        _kernel_stack_top = .;
    } > KERNEL

    /* Heap section */
    .heap ALIGN(4K) : AT(ADDR(.heap) - 0x100000)
    {
        _kernel_heap_start = .;
        . = . + 0x100000;   /* 1MB initial heap */
        _kernel_heap_end = .;
    } > KERNEL

    /* Module information table */
    .modules ALIGN(4K) : AT(ADDR(.modules) - 0x100000)
    {
        _modules_table_start = .;
        *(.modules)
        *(.modules.*)
        . = ALIGN(4K);
        _modules_table_end = .;
    } > KERNEL

    /* AI subsystem section (for future use) */
    .ai_data ALIGN(4K) : AT(ADDR(.ai_data) - 0x100000)
    {
        _ai_data_start = .;
        *(.ai_data)
        *(.ai_data.*)
        . = ALIGN(4K);
        _ai_data_end = .;
    } > KERNEL

    _kernel_end = .;
    
    /* Module loading area */
    .module_area : AT(0x400000)
    {
        _module_area_start = .;
        . = . + 4M;         /* 4MB for loadable modules */
        _module_area_end = .;
    } > MODULES

    /* Discard debug information for now */
    /DISCARD/ :
    {
        *(.comment)
        *(.note)
        *(.note.*)
        *(.eh_frame)
        *(.debug*)
    }
}

/* Export important addresses for kernel use */
PROVIDE(_kernel_physical_start = 0x100000);
PROVIDE(_kernel_virtual_start = 0x100000);
PROVIDE(_kernel_size = _kernel_end - _kernel_start);
