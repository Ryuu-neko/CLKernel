/*
 * =============================================================================
 * CLKernel - Physical Memory Management Implementation
 * =============================================================================
 * File: memory.c
 * Purpose: Physical memory detection, buddy allocator, and actor memory isolation
 * 
 * CLKernel's revolutionary approach to memory management:
 * - Actor-isolated memory allocation
 * - AI-supervised memory monitoring
 * - Performance-optimized buddy allocator
 * - Predictive out-of-memory detection
 * =============================================================================
 */

#include "memory.h"
#include "kernel.h"
#include "vga.h"

// =============================================================================
// Global Memory Management State
// =============================================================================

buddy_allocator_t kernel_buddy_allocator;
physical_region_t memory_regions[MAX_MEMORY_REGIONS];
uint32_t memory_region_count = 0;
memory_stats_t memory_statistics;
actor_memory_context_t* actor_contexts[256]; // Max 256 actors

// Page frame array (will be allocated after memory detection)
static page_frame_t* page_frames = NULL;
static uint32_t total_page_frames = 0;

// Memory detection data from bootloader (simple approach for now)
static uint32_t total_memory_kb = 0;

// =============================================================================
// Memory Initialization
// =============================================================================

void memory_init(void)
{
    kprintf("[MEMORY] Initializing memory management system...\n");
    
    // Clear statistics
    memory_statistics.total_memory = 0;
    memory_statistics.available_memory = 0;
    memory_statistics.used_memory = 0;
    memory_statistics.kernel_memory = 0;
    memory_statistics.actor_memory = 0;
    memory_statistics.module_memory = 0;
    memory_statistics.total_allocations = 0;
    memory_statistics.failed_allocations = 0;
    memory_statistics.fragmentation_level = 0;
    memory_statistics.allocation_time_avg = 0;
    memory_statistics.deallocation_time_avg = 0;
    memory_statistics.memory_pressure_level = 0;
    memory_statistics.predicted_oom_time = 0;
    memory_statistics.ai_monitoring_enabled = false;
    
    // Clear actor contexts
    for (int i = 0; i < 256; i++) {
        actor_contexts[i] = NULL;
    }
    
    // Step 1: Detect physical memory regions
    memory_detect_regions();
    
    // Step 2: Setup buddy allocator
    memory_setup_allocator();
    
    // Step 3: Initialize kernel heap area
    // (This will be handled by heap.c)
    
    // Step 4: Start memory monitoring for AI supervisor
    memory_start_monitoring();
    
    kprintf("[MEMORY] Memory management initialized\n");
    kprintf("[MEMORY] Total memory: %d MB\n", (uint32_t)(memory_statistics.total_memory / (1024 * 1024)));
    kprintf("[MEMORY] Available memory: %d MB\n", (uint32_t)(memory_statistics.available_memory / (1024 * 1024)));
    kprintf("[MEMORY] Buddy allocator ready with %d pages\n", kernel_buddy_allocator.total_pages);
}

/*
 * Detect physical memory regions
 * For now, use a simple approach - later we'll add BIOS memory map support
 */
void memory_detect_regions(void)
{
    kprintf("[MEMORY] Detecting physical memory regions...\n");
    
    // Simple memory detection using CMOS (basic approach)
    // In real implementation, we'd use INT 15h, E820h for proper memory map
    
    // Read lower memory (0-640KB) - always present
    memory_add_region(0x0, 0xA0000, MEMORY_TYPE_AVAILABLE);
    
    // Read extended memory (1MB+) - simplified detection
    // TODO: Implement proper E820 memory map reading
    uint32_t extended_memory = 32 * 1024 * 1024; // Assume 32MB for now (QEMU default)
    memory_add_region(KERNEL_END, extended_memory, MEMORY_TYPE_AVAILABLE);
    
    // Reserve kernel area
    memory_add_region(KERNEL_START, KERNEL_END - KERNEL_START, MEMORY_TYPE_RESERVED);
    
    // Reserve VGA memory
    memory_add_region(0xA0000, 0x60000, MEMORY_TYPE_RESERVED); // VGA 640KB-1MB
    
    // Calculate total memory
    memory_statistics.total_memory = extended_memory;
    memory_statistics.available_memory = extended_memory - (KERNEL_END - KERNEL_START);
    
    kprintf("[MEMORY] Detected %d memory regions\n", memory_region_count);
    memory_print_map();
}

/*
 * Add a memory region
 */
bool memory_add_region(uint64_t start, uint64_t length, memory_type_t type)
{
    if (memory_region_count >= MAX_MEMORY_REGIONS) {
        return false;
    }
    
    physical_region_t* region = &memory_regions[memory_region_count];
    region->start = PAGE_ALIGN_DOWN(start);
    region->end = PAGE_ALIGN_UP(start + length);
    region->type = type;
    region->available = (type == MEMORY_TYPE_AVAILABLE);
    region->ref_count = 0;
    
    memory_region_count++;
    return true;
}

/*
 * Print memory map
 */
void memory_print_map(void)
{
    kprintf("[MEMORY] Physical memory map:\n");
    
    for (uint32_t i = 0; i < memory_region_count; i++) {
        physical_region_t* region = &memory_regions[i];
        const char* type_str;
        
        switch (region->type) {
            case MEMORY_TYPE_AVAILABLE: type_str = "Available"; break;
            case MEMORY_TYPE_RESERVED: type_str = "Reserved"; break;
            case MEMORY_TYPE_ACPI_RECLAIM: type_str = "ACPI Reclaim"; break;
            case MEMORY_TYPE_ACPI_NVS: type_str = "ACPI NVS"; break;
            case MEMORY_TYPE_BAD: type_str = "Bad"; break;
            default: type_str = "Unknown"; break;
        }
        
        kprintf("  [%d] 0x%x - 0x%x (%s)\n", 
                i, (uint32_t)region->start, (uint32_t)region->end, type_str);
    }
}

// =============================================================================
// Buddy Allocator Setup
// =============================================================================

void memory_setup_allocator(void)
{
    kprintf("[MEMORY] Setting up buddy allocator...\n");
    
    // Find the largest available region for buddy allocator
    uint64_t allocator_start = 0;
    uint64_t allocator_size = 0;
    
    for (uint32_t i = 0; i < memory_region_count; i++) {
        physical_region_t* region = &memory_regions[i];
        if (region->available && (region->end - region->start) > allocator_size) {
            allocator_start = region->start;
            allocator_size = region->end - region->start;
        }
    }
    
    if (allocator_size == 0) {
        kprintf("[MEMORY] ERROR: No available memory for allocator!\n");
        return;
    }
    
    // Use memory starting after kernel heap area
    if (allocator_start < KERNEL_HEAP_END) {
        allocator_start = KERNEL_HEAP_END;
        allocator_size = allocator_size - (KERNEL_HEAP_END - allocator_start);
    }
    
    buddy_init(allocator_start, allocator_start + allocator_size);
    
    kprintf("[MEMORY] Buddy allocator initialized\n");
    kprintf("[MEMORY] Allocator range: 0x%x - 0x%x\n", 
            (uint32_t)allocator_start, (uint32_t)(allocator_start + allocator_size));
    kprintf("[MEMORY] Managing %d pages (%d KB)\n", 
            kernel_buddy_allocator.total_pages, 
            kernel_buddy_allocator.total_pages * 4);
}

/*
 * Initialize buddy allocator
 */
void buddy_init(uint64_t start_address, uint64_t end_address)
{
    // Calculate number of pages
    uint64_t size = end_address - start_address;
    uint32_t page_count = size / PAGE_SIZE;
    
    // Initialize buddy allocator structure
    for (int i = 0; i <= MAX_ORDER; i++) {
        kernel_buddy_allocator.free_list[i] = NULL;
        kernel_buddy_allocator.free_count[i] = 0;
    }
    
    kernel_buddy_allocator.total_pages = page_count;
    kernel_buddy_allocator.free_pages = page_count;
    kernel_buddy_allocator.allocated_pages = 0;
    
    // For now, create a simple free list (full buddy implementation would be more complex)
    // TODO: Implement full buddy allocator algorithm
    
    kprintf("[BUDDY] Initialized with %d pages\n", page_count);
}

/*
 * Get required order for number of pages
 */
uint32_t buddy_get_order(uint32_t pages)
{
    if (pages <= 1) return 0;
    
    uint32_t order = 0;
    uint32_t size = 1;
    
    while (size < pages) {
        size <<= 1;
        order++;
        if (order > MAX_ORDER) return MAX_ORDER;
    }
    
    return order;
}

// =============================================================================
// Page Allocation (Simplified Implementation)
// =============================================================================

/*
 * Allocate a single page
 */
page_frame_t* alloc_page(void)
{
    return alloc_pages(1);
}

/*
 * Allocate multiple pages
 */
page_frame_t* alloc_pages(uint32_t count)
{
    if (count == 0 || kernel_buddy_allocator.free_pages < count) {
        memory_statistics.failed_allocations++;
        return NULL;
    }
    
    // Simple linear allocator for now (TODO: implement proper buddy)
    static uint64_t next_free_address = 0;
    
    if (next_free_address == 0) {
        // Find first available region
        for (uint32_t i = 0; i < memory_region_count; i++) {
            if (memory_regions[i].available && memory_regions[i].start >= KERNEL_HEAP_END) {
                next_free_address = memory_regions[i].start;
                break;
            }
        }
    }
    
    if (next_free_address == 0) {
        memory_statistics.failed_allocations++;
        return NULL;
    }
    
    // Create a simple page frame descriptor
    static page_frame_t simple_page_frame;
    simple_page_frame.flags = PAGE_FLAG_PRESENT;
    simple_page_frame.ref_count = 1;
    simple_page_frame.owner_actor_id = 0; // Kernel
    simple_page_frame.next = NULL;
    simple_page_frame.prev = NULL;
    simple_page_frame.virtual_address = (void*)next_free_address;
    
    // Update allocator state
    kernel_buddy_allocator.free_pages -= count;
    kernel_buddy_allocator.allocated_pages += count;
    memory_statistics.total_allocations++;
    memory_statistics.used_memory += count * PAGE_SIZE;
    memory_statistics.available_memory -= count * PAGE_SIZE;
    
    next_free_address += count * PAGE_SIZE;
    
    return &simple_page_frame;
}

/*
 * Free pages
 */
void free_pages(page_frame_t* page, uint32_t count)
{
    if (!page || count == 0) return;
    
    // Update allocator state
    kernel_buddy_allocator.free_pages += count;
    kernel_buddy_allocator.allocated_pages -= count;
    memory_statistics.used_memory -= count * PAGE_SIZE;
    memory_statistics.available_memory += count * PAGE_SIZE;
    
    // TODO: Implement proper buddy coalescing
}

/*
 * Free a single page
 */
void free_page(page_frame_t* page)
{
    free_pages(page, 1);
}

// =============================================================================
// Actor Memory Management
// =============================================================================

/*
 * Create memory context for an actor
 */
actor_memory_context_t* actor_memory_create_context(uint32_t actor_id, uint64_t memory_limit)
{
    if (actor_id >= 256 || actor_contexts[actor_id] != NULL) {
        return NULL;
    }
    
    // For now, use a static allocation (TODO: use proper kmalloc when heap is ready)
    static actor_memory_context_t contexts[256];
    static uint32_t context_count = 0;
    
    if (context_count >= 256) {
        return NULL;
    }
    
    actor_memory_context_t* context = &contexts[context_count++];
    context->actor_id = actor_id;
    context->memory_limit = memory_limit;
    context->memory_used = 0;
    context->page_count = 0;
    context->pages = NULL; // TODO: allocate when heap is ready
    context->memory_protected = true;
    context->last_allocation_time = 0; // TODO: get real timestamp
    
    actor_contexts[actor_id] = context;
    
    kprintf("[MEMORY] Created memory context for actor %d (limit: %d KB)\n", 
            actor_id, (uint32_t)(memory_limit / 1024));
    
    return context;
}

/*
 * Check if actor can allocate memory
 */
bool actor_check_memory_limit(uint32_t actor_id, size_t size)
{
    if (actor_id >= 256 || actor_contexts[actor_id] == NULL) {
        return false;
    }
    
    actor_memory_context_t* context = actor_contexts[actor_id];
    return (context->memory_used + size) <= context->memory_limit;
}

// =============================================================================
// Memory Monitoring for AI Supervisor
// =============================================================================

/*
 * Start memory monitoring
 */
void memory_start_monitoring(void)
{
    memory_statistics.ai_monitoring_enabled = true;
    kprintf("[MEMORY] AI memory monitoring enabled\n");
}

/*
 * Update memory statistics
 */
void memory_update_stats(void)
{
    // Calculate fragmentation level
    if (kernel_buddy_allocator.total_pages > 0) {
        memory_statistics.fragmentation_level = 
            (100 * kernel_buddy_allocator.allocated_pages) / kernel_buddy_allocator.total_pages;
    }
    
    // Calculate memory pressure
    if (memory_statistics.total_memory > 0) {
        uint64_t used_percentage = (100 * memory_statistics.used_memory) / memory_statistics.total_memory;
        memory_statistics.memory_pressure_level = (uint32_t)used_percentage;
    }
    
    // Simple OOM prediction (TODO: enhance with AI)
    if (memory_statistics.memory_pressure_level > 90) {
        memory_statistics.predicted_oom_time = 60; // 60 seconds warning
    } else {
        memory_statistics.predicted_oom_time = 0;
    }
}

/*
 * Get memory statistics
 */
memory_stats_t* memory_get_stats(void)
{
    memory_update_stats();
    return &memory_statistics;
}

/*
 * AI-enhanced memory analysis
 */
void memory_ai_analysis(void)
{
    if (!memory_statistics.ai_monitoring_enabled) return;
    
    // TODO: Implement ML-based memory analysis
    // - Pattern recognition in allocation behavior
    // - Predictive OOM detection
    // - Automatic defragmentation recommendations
    // - Actor memory usage anomaly detection
    
    kprintf("[AI-MEMORY] Memory analysis completed\n");
}

// =============================================================================
// Debug and Diagnostics
// =============================================================================

/*
 * Dump memory regions
 */
void memory_dump_regions(void)
{
    memory_print_map();
}

/*
 * Dump buddy allocator state
 */
void memory_dump_buddy_state(void)
{
    kprintf("[BUDDY] Allocator state:\n");
    kprintf("  Total pages: %d\n", kernel_buddy_allocator.total_pages);
    kprintf("  Free pages: %d\n", kernel_buddy_allocator.free_pages);
    kprintf("  Allocated pages: %d\n", kernel_buddy_allocator.allocated_pages);
    kprintf("  Free percentage: %d%%\n", 
            (100 * kernel_buddy_allocator.free_pages) / kernel_buddy_allocator.total_pages);
}

/*
 * Dump actor memory usage
 */
void memory_dump_actor_usage(void)
{
    kprintf("[MEMORY] Actor memory usage:\n");
    
    for (uint32_t i = 0; i < 256; i++) {
        if (actor_contexts[i] != NULL) {
            actor_memory_context_t* ctx = actor_contexts[i];
            kprintf("  Actor %d: %d KB used / %d KB limit\n", 
                    ctx->actor_id, 
                    (uint32_t)(ctx->memory_used / 1024), 
                    (uint32_t)(ctx->memory_limit / 1024));
        }
    }
}
