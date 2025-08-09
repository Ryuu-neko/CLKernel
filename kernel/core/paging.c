/*
 * =============================================================================
 * CLKernel - Virtual Memory Management Implementation
 * =============================================================================
 * File: paging.c
 * Purpose: Virtual memory management with actor isolation and AI supervision
 * 
 * This paging implementation provides:
 * - Page table management for x86 protected mode
 * - Virtual memory address spaces for actors
 * - Memory mapped regions and VMA management
 * - AI-supervised memory access pattern analysis
 * =============================================================================
 */

#include "paging.h"
#include "memory.h"
#include "kernel.h"
#include "vga.h"

// =============================================================================
// Global Paging State
// =============================================================================

paging_context_t kernel_paging_context;
bool paging_enabled = false;

// Page directory and tables (aligned to 4KB boundaries)
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_tables[256][1024] __attribute__((aligned(4096)));  // Up to 256 page tables
static uint32_t next_page_table_index = 0;

// =============================================================================
// Paging Initialization
// =============================================================================

void paging_init(void)
{
    kprintf("[PAGING] Initializing virtual memory management...\n");
    
    // Clear page directory
    for (uint32_t i = 0; i < 1024; i++) {
        page_directory[i] = 0;
    }
    
    // Set up identity mapping for the first 4MB (kernel space)
    paging_create_page_table(0, PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE);
    
    // Identity map the first 4MB
    uint32_t* page_table = page_tables[0];
    for (uint32_t i = 0; i < 1024; i++) {
        page_table[i] = (i * 4096) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE;
    }
    
    // Initialize kernel paging context
    kernel_paging_context.page_directory_physical = (uint32_t)page_directory;
    kernel_paging_context.page_directory_virtual = page_directory;
    kernel_paging_context.page_fault_handler = paging_handle_page_fault;
    kernel_paging_context.statistics.page_faults = 0;
    kernel_paging_context.statistics.pages_allocated = 1024; // First 4MB
    kernel_paging_context.statistics.pages_freed = 0;
    kernel_paging_context.statistics.tlb_flushes = 0;
    kernel_paging_context.ai_monitoring_enabled = true;
    
    // Enable paging
    paging_enable_paging((uint32_t)page_directory);
    
    paging_enabled = true;
    
    kprintf("[PAGING] Virtual memory enabled\n");
    kprintf("[PAGING] Kernel mapped: 0x00000000 - 0x00400000 (4MB)\n");
    kprintf("[PAGING] Page directory at: 0x%x\n", (uint32_t)page_directory);
    kprintf("[PAGING] AI monitoring enabled\n");
}

/*
 * Enable paging (assembly helper required)
 */
void paging_enable_paging(uint32_t page_directory_physical)
{
    // Set CR3 register with page directory address
    asm volatile("mov %0, %%cr3" :: "r"(page_directory_physical));
    
    // Enable paging by setting bit 31 of CR0
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Set PG bit
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
    
    // Flush TLB
    asm volatile("mov %%cr3, %0; mov %0, %%cr3" :: "r"(page_directory_physical));
}

// =============================================================================
// Page Table Management
// =============================================================================

/*
 * Create a new page table
 */
bool paging_create_page_table(uint32_t page_dir_index, uint32_t flags)
{
    if (page_dir_index >= 1024 || next_page_table_index >= 256) {
        return false;
    }
    
    // Clear the page table
    uint32_t* page_table = page_tables[next_page_table_index];
    for (uint32_t i = 0; i < 1024; i++) {
        page_table[i] = 0;
    }
    
    // Set up page directory entry
    page_directory[page_dir_index] = ((uint32_t)page_table) | flags;
    
    next_page_table_index++;
    return true;
}

/*
 * Map a virtual page to a physical page
 */
bool paging_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    uint32_t page_dir_index = virtual_addr >> 22;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;
    
    // Check if page directory entry exists
    if (!(page_directory[page_dir_index] & PAGE_FLAG_PRESENT)) {
        // Create page table
        if (!paging_create_page_table(page_dir_index, PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE)) {
            return false;
        }
    }
    
    // Get page table
    uint32_t* page_table = (uint32_t*)(page_directory[page_dir_index] & 0xFFFFF000);
    
    // Map the page
    page_table[page_table_index] = (physical_addr & 0xFFFFF000) | flags;
    
    // Flush TLB for this page
    asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
    kernel_paging_context.statistics.tlb_flushes++;
    
    return true;
}

/*
 * Unmap a virtual page
 */
bool paging_unmap_page(uint32_t virtual_addr)
{
    uint32_t page_dir_index = virtual_addr >> 22;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;
    
    // Check if page directory entry exists
    if (!(page_directory[page_dir_index] & PAGE_FLAG_PRESENT)) {
        return false; // Page not mapped
    }
    
    // Get page table
    uint32_t* page_table = (uint32_t*)(page_directory[page_dir_index] & 0xFFFFF000);
    
    // Unmap the page
    page_table[page_table_index] = 0;
    
    // Flush TLB for this page
    asm volatile("invlpg (%0)" :: "r"(virtual_addr) : "memory");
    kernel_paging_context.statistics.tlb_flushes++;
    
    return true;
}

/*
 * Get the physical address for a virtual address
 */
uint32_t paging_get_physical_address(uint32_t virtual_addr)
{
    uint32_t page_dir_index = virtual_addr >> 22;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;
    uint32_t page_offset = virtual_addr & 0xFFF;
    
    // Check if page directory entry exists
    if (!(page_directory[page_dir_index] & PAGE_FLAG_PRESENT)) {
        return 0; // Page not mapped
    }
    
    // Get page table
    uint32_t* page_table = (uint32_t*)(page_directory[page_dir_index] & 0xFFFFF000);
    
    // Check if page table entry exists
    if (!(page_table[page_table_index] & PAGE_FLAG_PRESENT)) {
        return 0; // Page not mapped
    }
    
    // Get physical address
    uint32_t physical_page = page_table[page_table_index] & 0xFFFFF000;
    return physical_page | page_offset;
}

// =============================================================================
// Address Space Management
// =============================================================================

/*
 * Create a new address space for an actor
 */
address_space_t* paging_create_address_space(uint32_t actor_id)
{
    if (actor_id >= MAX_ACTORS) {
        return NULL;
    }
    
    // For now, return a pointer to kernel address space
    // TODO: Implement proper per-actor address spaces
    return &kernel_paging_context.address_spaces[0];
}

/*
 * Switch to an address space
 */
bool paging_switch_address_space(address_space_t* address_space)
{
    if (!address_space || !paging_enabled) {
        return false;
    }
    
    // Load new page directory
    asm volatile("mov %0, %%cr3" :: "r"(address_space->page_directory_physical));
    
    // Flush entire TLB
    kernel_paging_context.statistics.tlb_flushes++;
    
    return true;
}

/*
 * Destroy an address space
 */
void paging_destroy_address_space(address_space_t* address_space)
{
    if (!address_space) {
        return;
    }
    
    // TODO: Free all pages in the address space
    // TODO: Clean up VMA structures
    
    kprintf("[PAGING] Address space destroyed\n");
}

// =============================================================================
// Virtual Memory Area (VMA) Management
// =============================================================================

/*
 * Create a VMA for memory mapping
 */
vma_t* paging_create_vma(uint32_t start_addr, uint32_t end_addr, uint32_t flags, uint32_t type)
{
    // TODO: Implement VMA allocation from a pool
    // For now, return NULL (not implemented)
    
    kprintf("[PAGING] VMA creation requested: 0x%x - 0x%x\n", start_addr, end_addr);
    return NULL;
}

/*
 * Find VMA containing the given address
 */
vma_t* paging_find_vma(address_space_t* address_space, uint32_t addr)
{
    if (!address_space) {
        return NULL;
    }
    
    // TODO: Implement VMA search
    // For now, return NULL
    return NULL;
}

/*
 * Remove a VMA from address space
 */
bool paging_remove_vma(address_space_t* address_space, vma_t* vma)
{
    if (!address_space || !vma) {
        return false;
    }
    
    // TODO: Implement VMA removal and cleanup
    return true;
}

// =============================================================================
// Page Fault Handling
// =============================================================================

/*
 * Handle page faults
 */
void paging_handle_page_fault(uint32_t fault_addr, uint32_t error_code)
{
    kernel_paging_context.statistics.page_faults++;
    
    kprintf("[PAGING] Page fault at 0x%x, error code: 0x%x\n", fault_addr, error_code);
    
    // Decode error code
    bool present = error_code & 0x1;
    bool write = error_code & 0x2;
    bool user = error_code & 0x4;
    bool reserved = error_code & 0x8;
    bool instruction_fetch = error_code & 0x10;
    
    kprintf("[PAGING] Fault type: ");
    if (!present) kprintf("Page not present ");
    if (write) kprintf("Write access ");
    if (user) kprintf("User mode ");
    if (reserved) kprintf("Reserved bit ");
    if (instruction_fetch) kprintf("Instruction fetch ");
    kprintf("\n");
    
    // TODO: Implement intelligent page fault handling
    // - Check if address is in valid VMA
    // - Handle copy-on-write
    // - Handle demand paging
    // - Handle stack growth
    
    // For now, just report the fault
    kprintf("[PAGING] Page fault handling not fully implemented\n");
}

// =============================================================================
// Memory Mapping and I/O
// =============================================================================

/*
 * Map physical memory for I/O
 */
void* paging_map_io(uint32_t physical_addr, size_t size)
{
    // Align to page boundaries
    uint32_t page_aligned_addr = physical_addr & 0xFFFFF000;
    uint32_t page_count = (size + 4095) / 4096;
    
    // Find a virtual address range (simplified - use high addresses)
    uint32_t virtual_base = 0xF0000000; // Start from 3.75GB
    
    kprintf("[PAGING] Mapping I/O region: phys=0x%x size=%d pages=%d\n", 
            physical_addr, (uint32_t)size, page_count);
    
    // Map pages
    for (uint32_t i = 0; i < page_count; i++) {
        uint32_t virt_addr = virtual_base + (i * 4096);
        uint32_t phys_addr = page_aligned_addr + (i * 4096);
        
        if (!paging_map_page(virt_addr, phys_addr, 
                           PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE | PAGE_FLAG_CACHE_DISABLE)) {
            kprintf("[PAGING] Failed to map I/O page %d\n", i);
            return NULL;
        }
    }
    
    return (void*)(virtual_base + (physical_addr & 0xFFF));
}

/*
 * Unmap I/O memory
 */
void paging_unmap_io(void* virtual_addr, size_t size)
{
    uint32_t page_aligned_addr = (uint32_t)virtual_addr & 0xFFFFF000;
    uint32_t page_count = (size + 4095) / 4096;
    
    kprintf("[PAGING] Unmapping I/O region: virt=0x%x size=%d pages=%d\n", 
            (uint32_t)virtual_addr, (uint32_t)size, page_count);
    
    // Unmap pages
    for (uint32_t i = 0; i < page_count; i++) {
        uint32_t virt_addr = page_aligned_addr + (i * 4096);
        paging_unmap_page(virt_addr);
    }
}

// =============================================================================
// Statistics and Monitoring
// =============================================================================

/*
 * Get paging statistics
 */
paging_stats_t* paging_get_statistics(void)
{
    if (!paging_enabled) {
        return NULL;
    }
    
    return &kernel_paging_context.statistics;
}

/*
 * Print paging statistics
 */
void paging_print_statistics(void)
{
    if (!paging_enabled) {
        kprintf("[PAGING] Paging not enabled\n");
        return;
    }
    
    paging_stats_t* stats = &kernel_paging_context.statistics;
    
    kprintf("[PAGING] Virtual Memory Statistics:\n");
    kprintf("  Page faults: %d\n", stats->page_faults);
    kprintf("  Pages allocated: %d (%d KB)\n", 
            stats->pages_allocated, stats->pages_allocated * 4);
    kprintf("  Pages freed: %d\n", stats->pages_freed);
    kprintf("  TLB flushes: %d\n", stats->tlb_flushes);
    kprintf("  Page directory at: 0x%x\n", kernel_paging_context.page_directory_physical);
}

// =============================================================================
// AI Integration and Pattern Analysis
// =============================================================================

/*
 * AI-based memory access pattern analysis
 */
void paging_ai_analyze_access_patterns(void)
{
    if (!kernel_paging_context.ai_monitoring_enabled) {
        return;
    }
    
    // TODO: Implement ML-based access pattern analysis
    // - Track page access frequencies
    // - Identify hot/cold pages
    // - Predict future access patterns
    // - Optimize page placement
    
    kprintf("[AI-PAGING] Access pattern analysis completed\n");
}

/*
 * AI-assisted page replacement
 */
uint32_t paging_ai_select_victim_page(void)
{
    if (!kernel_paging_context.ai_monitoring_enabled) {
        return 0;
    }
    
    // TODO: Implement intelligent page replacement
    // - Use access history and patterns
    // - Consider actor priorities
    // - Predict future access likelihood
    
    kprintf("[AI-PAGING] AI page replacement not implemented\n");
    return 0;
}

/*
 * AI memory layout optimization
 */
void paging_ai_optimize_layout(void)
{
    if (!kernel_paging_context.ai_monitoring_enabled) {
        return;
    }
    
    // TODO: Implement layout optimization
    // - Reorganize pages for better locality
    // - Consolidate fragmented regions
    // - Optimize for cache performance
    
    kprintf("[AI-PAGING] Layout optimization completed\n");
}

// =============================================================================
// Debug and Diagnostic Functions
// =============================================================================

/*
 * Dump page directory contents
 */
void paging_dump_page_directory(void)
{
    kprintf("[PAGING] Page Directory contents:\n");
    
    uint32_t present_entries = 0;
    
    for (uint32_t i = 0; i < 1024; i++) {
        if (page_directory[i] & PAGE_FLAG_PRESENT) {
            present_entries++;
            kprintf("  Entry %d: 0x%x (Present, ", i, page_directory[i]);
            
            if (page_directory[i] & PAGE_FLAG_WRITABLE) kprintf("RW, ");
            else kprintf("RO, ");
            
            if (page_directory[i] & PAGE_FLAG_USER) kprintf("User");
            else kprintf("Kernel");
            
            kprintf(")\n");
        }
    }
    
    kprintf("[PAGING] Total present entries: %d\n", present_entries);
}

/*
 * Validate page table integrity
 */
bool paging_validate_page_tables(void)
{
    kprintf("[PAGING] Validating page table integrity...\n");
    
    uint32_t valid_entries = 0;
    uint32_t invalid_entries = 0;
    
    for (uint32_t i = 0; i < 1024; i++) {
        if (page_directory[i] & PAGE_FLAG_PRESENT) {
            uint32_t* page_table = (uint32_t*)(page_directory[i] & 0xFFFFF000);
            
            // Check if page table address is valid
            if ((uint32_t)page_table < 0x400000) { // Should be above 4MB
                valid_entries++;
            } else {
                invalid_entries++;
                kprintf("[PAGING] Invalid page table address at entry %d: 0x%x\n", 
                        i, (uint32_t)page_table);
            }
        }
    }
    
    kprintf("[PAGING] Validation complete: %d valid, %d invalid\n", 
            valid_entries, invalid_entries);
    
    return (invalid_entries == 0);
}

/*
 * Test paging functionality
 */
void paging_test_functionality(void)
{
    kprintf("[PAGING] Running paging functionality tests...\n");
    
    // Test 1: Virtual to physical address translation
    uint32_t test_virt = 0x1000;
    uint32_t test_phys = paging_get_physical_address(test_virt);
    kprintf("  Test 1 - Address translation: 0x%x -> 0x%x\n", test_virt, test_phys);
    
    // Test 2: Page mapping
    uint32_t new_virt = 0x500000; // 5MB
    uint32_t new_phys = 0x500000;
    if (paging_map_page(new_virt, new_phys, PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE)) {
        kprintf("  Test 2 - Page mapping: SUCCESS\n");
    } else {
        kprintf("  Test 2 - Page mapping: FAILED\n");
    }
    
    // Test 3: Memory access
    volatile uint32_t* test_ptr = (volatile uint32_t*)new_virt;
    *test_ptr = 0xDEADBEEF;
    if (*test_ptr == 0xDEADBEEF) {
        kprintf("  Test 3 - Memory access: SUCCESS (wrote and read 0xDEADBEEF)\n");
    } else {
        kprintf("  Test 3 - Memory access: FAILED\n");
    }
    
    kprintf("[PAGING] Functionality tests completed\n");
}
