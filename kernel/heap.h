/*
 * =============================================================================
 * CLKernel - Kernel Heap Implementation
 * =============================================================================
 * File: heap.h
 * Purpose: Dynamic memory allocation for kernel and actors
 * 
 * CLKernel Heap Features:
 * - Actor-isolated heap regions
 * - AI-monitored allocation patterns
 * - Performance-optimized slab allocator
 * - Memory leak detection and prevention
 * =============================================================================
 */

#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Heap Configuration
// =============================================================================

#define HEAP_MIN_BLOCK_SIZE     16          // Minimum allocation size
#define HEAP_MAX_BLOCK_SIZE     0x100000    // Maximum single allocation (1MB)
#define HEAP_ALIGNMENT          8           // Memory alignment requirement
#define HEAP_MAGIC              0xDEADBEEF  // Magic number for corruption detection

// Slab allocator sizes (power of 2)
#define SLAB_SIZE_COUNT         12
static const uint32_t SLAB_SIZES[SLAB_SIZE_COUNT] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768
};

// =============================================================================
// Heap Block Structure
// =============================================================================

typedef struct heap_block {
    uint32_t magic;                     // Magic number for corruption detection
    uint32_t size;                      // Size of this block (including header)
    bool allocated;                     // Is this block allocated?
    uint32_t owner_actor_id;            // Which actor owns this block
    uint64_t timestamp;                 // When was this block allocated
    struct heap_block* next;            // Next block in list
    struct heap_block* prev;            // Previous block in list
    
    // Debug information (only in debug builds)
    #ifdef DEBUG
    const char* file;                   // Source file of allocation
    int line;                           // Line number of allocation
    const char* function;               // Function name of allocation
    #endif
} heap_block_t;

// =============================================================================
// Slab Allocator Structure
// =============================================================================

typedef struct slab {
    uint32_t size;                      // Size of objects in this slab
    uint32_t free_count;                // Number of free objects
    uint32_t total_count;               // Total objects in slab
    void* free_list;                    // Free object list
    struct slab* next;                  // Next slab of same size
} slab_t;

typedef struct {
    slab_t* slabs[SLAB_SIZE_COUNT];     // Slab lists for each size
    uint32_t total_slabs;               // Total number of slabs
    uint64_t total_allocated;           // Total memory allocated via slabs
    uint64_t total_freed;               // Total memory freed via slabs
} slab_allocator_t;

// =============================================================================
// Heap Statistics (for AI Supervisor)
// =============================================================================

typedef struct {
    uint64_t total_allocations;         // Total allocation count
    uint64_t total_frees;               // Total free count
    uint64_t current_allocations;       // Current active allocations
    uint64_t bytes_allocated;           // Total bytes allocated
    uint64_t bytes_freed;               // Total bytes freed
    uint64_t peak_usage;                // Peak memory usage
    uint32_t fragmentation_level;       // Fragmentation percentage
    uint64_t allocation_time_avg;       // Average allocation time
    uint64_t free_time_avg;             // Average free time
    
    // Memory leak detection
    uint32_t potential_leaks;           // Number of potential memory leaks
    uint64_t leaked_bytes;              // Bytes in potential leaks
    
    // Actor-specific statistics
    uint32_t actor_allocations[256];    // Allocations per actor
    uint64_t actor_memory_used[256];    // Memory used per actor
} heap_stats_t;

// =============================================================================
// Heap Configuration Structure
// =============================================================================

typedef struct {
    void* start_address;                // Start of heap area
    void* end_address;                  // End of heap area
    uint64_t total_size;                // Total heap size
    uint64_t available_size;            // Available heap size
    heap_block_t* free_list;            // Free block list
    slab_allocator_t slab_allocator;    // Slab allocator
    heap_stats_t statistics;            // Heap statistics
    bool corruption_check_enabled;      // Enable corruption checking
    bool leak_detection_enabled;        // Enable leak detection
    bool ai_monitoring_enabled;         // Enable AI monitoring
} heap_t;

// =============================================================================
// Function Prototypes - Heap Initialization
// =============================================================================

void heap_init(void);
bool heap_create(heap_t* heap, void* start, void* end);
void heap_destroy(heap_t* heap);

// =============================================================================
// Function Prototypes - Memory Allocation
// =============================================================================

// Standard allocation functions
void* kmalloc(size_t size);
void* kmalloc_aligned(size_t size, uint32_t alignment);
void* kcalloc(size_t count, size_t size);
void* krealloc(void* ptr, size_t new_size);
void kfree(void* ptr);

// Actor-specific allocation
void* actor_malloc(uint32_t actor_id, size_t size);
void* actor_calloc(uint32_t actor_id, size_t count, size_t size);
void* actor_realloc(uint32_t actor_id, void* ptr, size_t new_size);
void actor_free(uint32_t actor_id, void* ptr);

// Debug allocation functions (with source location tracking)
#ifdef DEBUG
#define kmalloc_debug(size) kmalloc_with_info(size, __FILE__, __LINE__, __FUNCTION__)
#define kfree_debug(ptr) kfree_with_info(ptr, __FILE__, __LINE__, __FUNCTION__)
void* kmalloc_with_info(size_t size, const char* file, int line, const char* function);
void kfree_with_info(void* ptr, const char* file, int line, const char* function);
#endif

// =============================================================================
// Function Prototypes - Slab Allocator
// =============================================================================

void slab_init(slab_allocator_t* allocator);
void* slab_alloc(uint32_t size);
void slab_free(void* ptr);
slab_t* slab_create(uint32_t size);
void slab_destroy(slab_t* slab);

// =============================================================================
// Function Prototypes - Memory Management
// =============================================================================

heap_block_t* heap_find_free_block(size_t size);
heap_block_t* heap_split_block(heap_block_t* block, size_t size);
void heap_merge_blocks(void);
void heap_coalesce_free_blocks(void);
bool heap_expand(size_t additional_size);

// =============================================================================
// Function Prototypes - Memory Protection and Validation
// =============================================================================

bool heap_validate_pointer(void* ptr);
bool heap_check_corruption(void);
bool heap_check_magic(heap_block_t* block);
void heap_mark_pattern(void* ptr, size_t size, uint8_t pattern);
bool heap_verify_pattern(void* ptr, size_t size, uint8_t pattern);

// =============================================================================
// Function Prototypes - Memory Leak Detection
// =============================================================================

void heap_start_leak_detection(void);
void heap_stop_leak_detection(void);
void heap_check_leaks(void);
void heap_dump_allocations(void);
void heap_dump_actor_allocations(uint32_t actor_id);

// =============================================================================
// Function Prototypes - Statistics and Monitoring
// =============================================================================

heap_stats_t* heap_get_statistics(void);
void heap_update_statistics(void);
void heap_print_statistics(void);
void heap_reset_statistics(void);

// Actor statistics
void heap_print_actor_stats(uint32_t actor_id);
uint64_t heap_get_actor_usage(uint32_t actor_id);
uint32_t heap_get_actor_allocations(uint32_t actor_id);

// =============================================================================
// Function Prototypes - AI Integration
// =============================================================================

void heap_ai_analyze_patterns(void);
void heap_ai_predict_allocations(uint32_t actor_id);
void heap_ai_detect_leaks(void);
void heap_ai_optimize_layout(void);
void heap_ai_suggest_compaction(void);

// =============================================================================
// Function Prototypes - Debug and Diagnostics
// =============================================================================

void heap_dump_blocks(void);
void heap_dump_free_list(void);
void heap_dump_slab_state(void);
void heap_benchmark_performance(void);
void heap_stress_test(void);

// Visualization helpers
void heap_visualize_fragmentation(void);
void heap_visualize_actor_usage(void);

// =============================================================================
// Utility Macros
// =============================================================================

// Alignment macros
#define HEAP_ALIGN_UP(addr, align)     (((addr) + (align) - 1) & ~((align) - 1))
#define HEAP_ALIGN_DOWN(addr, align)   ((addr) & ~((align) - 1))
#define HEAP_IS_ALIGNED(addr, align)   (((addr) & ((align) - 1)) == 0)

// Size calculation macros
#define HEAP_BLOCK_OVERHEAD             sizeof(heap_block_t)
#define HEAP_MIN_ALLOCATION             (HEAP_MIN_BLOCK_SIZE + HEAP_BLOCK_OVERHEAD)
#define HEAP_USABLE_SIZE(block)         ((block)->size - HEAP_BLOCK_OVERHEAD)

// Magic number validation
#define HEAP_VALIDATE_MAGIC(block)      ((block)->magic == HEAP_MAGIC)
#define HEAP_SET_MAGIC(block)           ((block)->magic = HEAP_MAGIC)
#define HEAP_CLEAR_MAGIC(block)         ((block)->magic = 0)

// Actor ownership
#define HEAP_SET_OWNER(block, actor)    ((block)->owner_actor_id = (actor))
#define HEAP_GET_OWNER(block)           ((block)->owner_actor_id)
#define HEAP_IS_OWNED_BY(block, actor)  ((block)->owner_actor_id == (actor))

// Size categories for slab allocator
static inline uint32_t heap_get_slab_index(size_t size)
{
    for (uint32_t i = 0; i < SLAB_SIZE_COUNT; i++) {
        if (size <= SLAB_SIZES[i]) {
            return i;
        }
    }
    return SLAB_SIZE_COUNT; // Too large for slab allocation
}

// Error codes
#define HEAP_SUCCESS                    0
#define HEAP_ERROR_OUT_OF_MEMORY        -1
#define HEAP_ERROR_INVALID_POINTER      -2
#define HEAP_ERROR_CORRUPTION_DETECTED  -3
#define HEAP_ERROR_DOUBLE_FREE          -4
#define HEAP_ERROR_ACTOR_LIMIT_EXCEEDED -5
#define HEAP_ERROR_FRAGMENTATION        -6

// Global heap state
extern heap_t kernel_heap;
extern bool heap_initialized;

#endif // HEAP_H
