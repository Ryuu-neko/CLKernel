/*
 * =============================================================================
 * CLKernel - Paging System Header
 * =============================================================================
 * File: paging.h
 * Purpose: Virtual memory management, page tables, and memory protection
 * 
 * CLKernel Paging Innovation:
 * - Actor memory isolation through page tables
 * - Hot-swappable module page mapping
 * - AI-supervised page fault handling
 * - Performance-optimized TLB management
 * =============================================================================
 */

#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>
#include "memory.h"

// =============================================================================
// Paging Constants
// =============================================================================

#define PAGE_DIRECTORY_SIZE     1024        // Number of page directory entries
#define PAGE_TABLE_SIZE         1024        // Number of page table entries
#define PAGE_TABLE_ALIGN        0x1000      // Page table alignment (4KB)

// Virtual memory layout
#define KERNEL_VIRTUAL_BASE     0xC0000000  // 3GB (kernel space starts here)
#define USER_VIRTUAL_BASE       0x00000000  // 0GB (user space starts here)
#define USER_VIRTUAL_LIMIT      0xBFFFFFFF  // User space limit (3GB)

// Special virtual addresses
#define RECURSIVE_PD_INDEX      1023        // Recursive page directory mapping
#define RECURSIVE_PD_ADDR       0xFFFFF000  // Recursive PD virtual address
#define TEMP_PAGE_ADDR          0xFFFE0000  // Temporary page mapping address

// =============================================================================
// Page Table Entry Structures
// =============================================================================

// Page Directory Entry (32-bit)
typedef union {
    uint32_t raw;
    struct {
        uint32_t present        : 1;    // Page is present in memory
        uint32_t writable       : 1;    // Page is writable
        uint32_t user           : 1;    // Page accessible from user mode
        uint32_t write_through  : 1;    // Write-through caching
        uint32_t cache_disabled : 1;    // Cache disabled
        uint32_t accessed       : 1;    // Page has been accessed
        uint32_t reserved       : 1;    // Reserved (must be 0)
        uint32_t page_size      : 1;    // 0 = 4KB, 1 = 4MB pages
        uint32_t global         : 1;    // Global page (not flushed on CR3 reload)
        uint32_t available      : 3;    // Available for OS use
        uint32_t address        : 20;   // Physical address >> 12
    };
} __attribute__((packed)) page_directory_entry_t;

// Page Table Entry (32-bit)
typedef union {
    uint32_t raw;
    struct {
        uint32_t present        : 1;    // Page is present in memory
        uint32_t writable       : 1;    // Page is writable
        uint32_t user           : 1;    // Page accessible from user mode
        uint32_t write_through  : 1;    // Write-through caching
        uint32_t cache_disabled : 1;    // Cache disabled
        uint32_t accessed       : 1;    // Page has been accessed
        uint32_t dirty          : 1;    // Page has been written to
        uint32_t attribute      : 1;    // Page attribute table
        uint32_t global         : 1;    // Global page
        uint32_t available      : 3;    // Available for OS use (actor ID, etc.)
        uint32_t address        : 20;   // Physical address >> 12
    };
} __attribute__((packed)) page_table_entry_t;

// Page Directory (1024 entries = 4KB)
typedef struct {
    page_directory_entry_t entries[PAGE_DIRECTORY_SIZE];
} __attribute__((aligned(PAGE_SIZE))) page_directory_t;

// Page Table (1024 entries = 4KB)
typedef struct {
    page_table_entry_t entries[PAGE_TABLE_SIZE];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

// =============================================================================
// Virtual Memory Area (VMA) for Actor Memory Management
// =============================================================================

typedef struct vma {
    uint32_t start_addr;                // Virtual start address
    uint32_t end_addr;                  // Virtual end address
    uint32_t flags;                     // Protection flags
    uint32_t owner_actor_id;            // Which actor owns this VMA
    struct vma* next;                   // Next VMA in list
} virtual_memory_area_t;

// =============================================================================
// Address Space Descriptor (for each actor)
// =============================================================================

typedef struct {
    uint32_t actor_id;                  // Actor identifier
    page_directory_t* page_directory;   // Page directory for this address space
    virtual_memory_area_t* vma_list;    // List of virtual memory areas
    uint32_t total_pages;               // Total pages allocated
    uint32_t code_pages;                // Pages for code
    uint32_t data_pages;                // Pages for data
    uint32_t stack_pages;               // Pages for stack
    bool copy_on_write_enabled;         // COW optimization
} address_space_t;

// =============================================================================
// Page Fault Information (for AI Supervisor)
// =============================================================================

typedef struct {
    uint32_t fault_address;             // Virtual address that caused fault
    uint32_t error_code;                // Page fault error code
    uint32_t actor_id;                  // Actor that caused the fault
    uint64_t timestamp;                 // When the fault occurred
    uint32_t instruction_pointer;       // EIP when fault occurred
    bool resolved;                      // Was the fault resolved?
    uint32_t resolution_time_us;        // Time to resolve (microseconds)
} page_fault_info_t;

// =============================================================================
// Function Prototypes - Paging Initialization
// =============================================================================

void paging_init(void);
void paging_enable(void);
void paging_setup_kernel_mappings(void);
page_directory_t* paging_create_directory(void);
void paging_destroy_directory(page_directory_t* dir);

// =============================================================================
// Function Prototypes - Page Table Management
// =============================================================================

void paging_map_page(page_directory_t* dir, uint32_t virtual_addr, 
                    uint32_t physical_addr, uint32_t flags);
void paging_unmap_page(page_directory_t* dir, uint32_t virtual_addr);
uint32_t paging_get_physical_address(page_directory_t* dir, uint32_t virtual_addr);
bool paging_is_page_present(page_directory_t* dir, uint32_t virtual_addr);

// Bulk operations
void paging_map_range(page_directory_t* dir, uint32_t virtual_start, 
                     uint32_t physical_start, uint32_t size, uint32_t flags);
void paging_unmap_range(page_directory_t* dir, uint32_t virtual_start, uint32_t size);

// =============================================================================
// Function Prototypes - Address Space Management
// =============================================================================

address_space_t* address_space_create(uint32_t actor_id);
void address_space_destroy(address_space_t* space);
void address_space_switch(address_space_t* space);
address_space_t* address_space_get_current(void);

// VMA management
virtual_memory_area_t* vma_create(uint32_t start, uint32_t end, uint32_t flags, uint32_t actor_id);
void vma_destroy(virtual_memory_area_t* vma);
virtual_memory_area_t* vma_find(address_space_t* space, uint32_t address);
bool vma_add(address_space_t* space, virtual_memory_area_t* vma);
void vma_remove(address_space_t* space, virtual_memory_area_t* vma);

// =============================================================================
// Function Prototypes - Page Fault Handling
// =============================================================================

void page_fault_handler_advanced(uint32_t fault_address, uint32_t error_code);
bool page_fault_handle_demand_paging(uint32_t address);
bool page_fault_handle_copy_on_write(uint32_t address);
bool page_fault_handle_stack_expansion(uint32_t address, uint32_t actor_id);
void page_fault_log_for_ai(page_fault_info_t* info);

// =============================================================================
// Function Prototypes - Actor Memory Protection
// =============================================================================

bool paging_actor_map_memory(uint32_t actor_id, uint32_t virtual_addr, 
                             size_t size, uint32_t flags);
void paging_actor_unmap_memory(uint32_t actor_id, uint32_t virtual_addr, size_t size);
bool paging_actor_protect_memory(uint32_t actor_id, uint32_t virtual_addr, 
                                size_t size, uint32_t new_flags);
bool paging_actor_check_access(uint32_t actor_id, uint32_t virtual_addr, uint32_t flags);

// =============================================================================
// Function Prototypes - Module Memory Management
// =============================================================================

bool paging_module_map(const char* module_name, uint32_t virtual_addr, 
                      uint32_t physical_addr, size_t size);
void paging_module_unmap(const char* module_name);
bool paging_module_remap(const char* module_name, uint32_t new_physical_addr);

// =============================================================================
// Function Prototypes - TLB Management
// =============================================================================

void tlb_flush_all(void);
void tlb_flush_page(uint32_t virtual_addr);
void tlb_flush_actor(uint32_t actor_id);
void tlb_invalidate_range(uint32_t start, uint32_t end);

// =============================================================================
// Function Prototypes - AI-Enhanced Features
// =============================================================================

void paging_ai_analyze_patterns(void);
void paging_ai_predict_faults(uint32_t actor_id);
void paging_ai_optimize_layout(address_space_t* space);
void paging_ai_suggest_prefetch(uint32_t actor_id);

// =============================================================================
// Function Prototypes - Debug and Diagnostics
// =============================================================================

void paging_dump_directory(page_directory_t* dir);
void paging_dump_address_space(address_space_t* space);
void paging_dump_vma_list(virtual_memory_area_t* vma);
void paging_check_integrity(void);
void paging_benchmark_performance(void);

// =============================================================================
// Utility Macros and Functions
// =============================================================================

// Address manipulation macros
#define PAGE_DIRECTORY_INDEX(addr)  (((addr) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(addr)      (((addr) >> 12) & 0x3FF)
#define PAGE_FRAME_ADDR(addr)       ((addr) & 0xFFFFF000)
#define PAGE_OFFSET(addr)           ((addr) & 0xFFF)

// Virtual to physical conversion
#define KERNEL_VIRTUAL_TO_PHYSICAL(vaddr) ((vaddr) - KERNEL_VIRTUAL_BASE)
#define KERNEL_PHYSICAL_TO_VIRTUAL(paddr) ((paddr) + KERNEL_VIRTUAL_BASE)

// Page flags (combined with standard flags)
#define PAGE_FLAG_ACTOR_OWNED       0x200   // Page owned by specific actor
#define PAGE_FLAG_MODULE_CODE       0x400   // Module code page
#define PAGE_FLAG_AI_MONITORED      0x800   // AI supervisor monitoring

// Error codes
#define PAGING_SUCCESS              0
#define PAGING_ERROR_OUT_OF_MEMORY  -1
#define PAGING_ERROR_INVALID_ADDR   -2
#define PAGING_ERROR_ACCESS_DENIED  -3
#define PAGING_ERROR_ALREADY_MAPPED -4
#define PAGING_ERROR_NOT_MAPPED     -5

// Global paging state
extern page_directory_t* kernel_page_directory;
extern address_space_t* current_address_space;
extern uint32_t paging_enabled;

#endif // PAGING_H
