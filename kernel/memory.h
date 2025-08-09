/*
 * =============================================================================
 * CLKernel - Memory Management Header
 * =============================================================================
 * File: memory.h
 * Purpose: Physical memory detection, management, and allocation structures
 * 
 * CLKernel Innovation: Memory management designed for async actors
 * - Actor-safe memory allocation
 * - AI supervisor memory monitoring
 * - Hot-swappable module memory isolation
 * - Performance-optimized for message passing
 * =============================================================================
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Memory Layout Constants
// =============================================================================

#define PAGE_SIZE               0x1000      // 4KB pages
#define PAGE_SHIFT              12          // log2(PAGE_SIZE)
#define PAGE_MASK               0xFFF       // Page offset mask

// Kernel memory layout (from linker script)
#define KERNEL_START            0x100000    // 1MB
#define KERNEL_END              0x200000    // 2MB
#define KERNEL_HEAP_START       0x200000    // 2MB
#define KERNEL_HEAP_END         0x400000    // 4MB
#define MODULE_AREA_START       0x400000    // 4MB
#define MODULE_AREA_END         0x800000    // 8MB
#define USER_SPACE_START        0x800000    // 8MB+

// Memory limits
#define MAX_MEMORY_REGIONS      64          // Maximum BIOS memory map entries
#define MIN_MEMORY_SIZE         0x1000000   // Minimum 16MB required
#define MAX_ACTOR_MEMORY        0x100000    // 1MB per actor maximum
#define AI_SUPERVISOR_MEMORY    0x200000    // 2MB for AI supervisor

// =============================================================================
// Memory Types and Flags
// =============================================================================

typedef enum {
    MEMORY_TYPE_AVAILABLE = 1,      // Usable RAM
    MEMORY_TYPE_RESERVED = 2,       // Reserved by BIOS
    MEMORY_TYPE_ACPI_RECLAIM = 3,   // ACPI reclaimable
    MEMORY_TYPE_ACPI_NVS = 4,       // ACPI non-volatile
    MEMORY_TYPE_BAD = 5             // Bad memory
} memory_type_t;

typedef enum {
    PAGE_FLAG_PRESENT   = 0x01,     // Page is present in memory
    PAGE_FLAG_WRITABLE  = 0x02,     // Page is writable
    PAGE_FLAG_USER      = 0x04,     // Page accessible from user mode
    PAGE_FLAG_ACCESSED  = 0x20,     // Page has been accessed
    PAGE_FLAG_DIRTY     = 0x40,     // Page has been written to
    PAGE_FLAG_ACTOR     = 0x100,    // Page belongs to an actor (custom)
    PAGE_FLAG_MODULE    = 0x200,    // Page belongs to a module (custom)
    PAGE_FLAG_AI        = 0x400     // Page belongs to AI supervisor (custom)
} page_flags_t;

// =============================================================================
// Memory Region Structures
// =============================================================================

// BIOS memory map entry
typedef struct {
    uint64_t base_address;          // Physical start address
    uint64_t length;                // Region length in bytes
    uint32_t type;                  // Memory type (see memory_type_t)
    uint32_t extended_attributes;   // Extended attributes
} __attribute__((packed)) bios_memory_region_t;

// Physical memory region for kernel management
typedef struct {
    uint64_t start;                 // Start address (page-aligned)
    uint64_t end;                   // End address (page-aligned)
    memory_type_t type;             // Memory type
    bool available;                 // Available for allocation
    uint32_t ref_count;             // Reference count for shared pages
} physical_region_t;

// Physical page frame descriptor
typedef struct page_frame {
    uint32_t flags;                 // Page flags and status
    uint32_t ref_count;             // Reference counter
    uint32_t owner_actor_id;        // Which actor owns this page
    struct page_frame* next;        // Next in free list
    struct page_frame* prev;        // Previous in free list
    void* virtual_address;          // Mapped virtual address (if mapped)
} page_frame_t;

// =============================================================================
// Memory Statistics and Monitoring (for AI Supervisor)
// =============================================================================

typedef struct {
    // Global memory statistics
    uint64_t total_memory;          // Total physical memory
    uint64_t available_memory;      // Available physical memory
    uint64_t used_memory;           // Currently used memory
    uint64_t kernel_memory;         // Memory used by kernel
    uint64_t actor_memory;          // Memory used by actors
    uint64_t module_memory;         // Memory used by modules
    
    // Allocation statistics
    uint64_t total_allocations;     // Total allocation count
    uint64_t failed_allocations;    // Failed allocation count
    uint64_t fragmentation_level;   // Memory fragmentation percentage
    
    // Performance metrics
    uint64_t allocation_time_avg;   // Average allocation time (microseconds)
    uint64_t deallocation_time_avg; // Average deallocation time
    
    // AI supervisor metrics
    uint32_t memory_pressure_level; // 0-100 memory pressure indicator
    uint32_t predicted_oom_time;    // Predicted out-of-memory time (seconds)
    bool ai_monitoring_enabled;     // AI memory monitoring status
} memory_stats_t;

// =============================================================================
// Buddy Allocator System
// =============================================================================

#define MAX_ORDER               10          // Maximum buddy order (2^10 = 1024 pages = 4MB)
#define MIN_ORDER               0           // Minimum buddy order (1 page = 4KB)

typedef struct {
    page_frame_t* free_list[MAX_ORDER + 1]; // Free lists for each order
    uint32_t free_count[MAX_ORDER + 1];     // Count of free blocks per order
    uint32_t total_pages;                   // Total pages managed
    uint32_t free_pages;                    // Currently free pages
    uint32_t allocated_pages;               // Currently allocated pages
} buddy_allocator_t;

// =============================================================================
// Actor Memory Context (for isolation)
// =============================================================================

typedef struct {
    uint32_t actor_id;              // Actor identifier
    uint64_t memory_limit;          // Memory limit for this actor
    uint64_t memory_used;           // Currently used memory
    uint32_t page_count;            // Number of allocated pages
    page_frame_t** pages;           // Array of allocated pages
    bool memory_protected;          // Memory protection enabled
    uint32_t last_allocation_time;  // Last allocation timestamp
} actor_memory_context_t;

// =============================================================================
// Function Prototypes - Memory Detection
// =============================================================================

void memory_init(void);
void memory_detect_regions(void);
void memory_setup_allocator(void);
void memory_enable_paging(void);

// Memory region management
bool memory_add_region(uint64_t start, uint64_t length, memory_type_t type);
physical_region_t* memory_find_region(uint64_t address);
void memory_print_map(void);

// =============================================================================
// Function Prototypes - Page Frame Allocation
// =============================================================================

// Buddy allocator
void buddy_init(uint64_t start_address, uint64_t end_address);
page_frame_t* buddy_alloc_pages(uint32_t order);
void buddy_free_pages(page_frame_t* page, uint32_t order);
uint32_t buddy_get_order(uint32_t pages);

// Page frame management
page_frame_t* alloc_page(void);
page_frame_t* alloc_pages(uint32_t count);
void free_page(page_frame_t* page);
void free_pages(page_frame_t* page, uint32_t count);

// Physical address conversion
uint64_t page_frame_to_physical(page_frame_t* page);
page_frame_t* physical_to_page_frame(uint64_t physical);

// =============================================================================
// Function Prototypes - Actor Memory Management
// =============================================================================

actor_memory_context_t* actor_memory_create_context(uint32_t actor_id, uint64_t memory_limit);
void actor_memory_destroy_context(actor_memory_context_t* context);
void* actor_alloc_memory(uint32_t actor_id, size_t size);
void actor_free_memory(uint32_t actor_id, void* ptr);
bool actor_check_memory_limit(uint32_t actor_id, size_t size);

// =============================================================================
// Function Prototypes - Memory Monitoring (AI Integration)
// =============================================================================

void memory_start_monitoring(void);
void memory_update_stats(void);
memory_stats_t* memory_get_stats(void);
void memory_check_pressure(void);
void memory_predict_oom(void);
void memory_ai_analysis(void);

// =============================================================================
// Function Prototypes - Debug and Diagnostics
// =============================================================================

void memory_dump_regions(void);
void memory_dump_buddy_state(void);
void memory_dump_actor_usage(void);
void memory_check_integrity(void);
void memory_benchmark_allocator(void);

// Utility macros
#define PAGE_ALIGN_DOWN(addr)   ((addr) & ~PAGE_MASK)
#define PAGE_ALIGN_UP(addr)     (((addr) + PAGE_SIZE - 1) & ~PAGE_MASK)
#define BYTES_TO_PAGES(bytes)   (((bytes) + PAGE_SIZE - 1) / PAGE_SIZE)
#define PAGES_TO_BYTES(pages)   ((pages) * PAGE_SIZE)

// Error codes
#define MEMORY_ERROR_SUCCESS        0
#define MEMORY_ERROR_OUT_OF_MEMORY  -1
#define MEMORY_ERROR_INVALID_SIZE   -2
#define MEMORY_ERROR_ACTOR_LIMIT    -3
#define MEMORY_ERROR_FRAGMENTATION  -4
#define MEMORY_ERROR_AI_PREDICTION  -5

// Global memory management state
extern buddy_allocator_t kernel_buddy_allocator;
extern physical_region_t memory_regions[MAX_MEMORY_REGIONS];
extern uint32_t memory_region_count;
extern memory_stats_t memory_statistics;
extern actor_memory_context_t* actor_contexts[256]; // Max 256 actors

#endif // MEMORY_H
