/*
 * =============================================================================
 * CLKernel - Dynamic Module System
 * =============================================================================
 * File: modules.h
 * Purpose: Hot-swappable module system for revolutionary kernel architecture
 *
 * This module system implements:
 * - Runtime module loading and unloading
 * - Hot-swapping without kernel restart
 * - Dependency management and symbol resolution
 * - AI-supervised module health monitoring
 * =============================================================================
 */

#ifndef MODULES_H
#define MODULES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// =============================================================================
// Constants and Configuration
// =============================================================================

#define MAX_MODULES             64      // Maximum loaded modules
#define MAX_MODULE_NAME         64      // Maximum module name length
#define MAX_MODULE_SIZE         (1024*1024) // Maximum module size (1MB)
#define MAX_DEPENDENCIES        16      // Maximum dependencies per module
#define MAX_EXPORTED_SYMBOLS    256     // Maximum exported symbols per module
#define MODULE_MAGIC            0x4D4F44 // "MOD" magic number
#define MODULE_VERSION          1       // Module format version

// Module states
#define MODULE_STATE_UNLOADED   0       // Not loaded
#define MODULE_STATE_LOADING    1       // Currently being loaded
#define MODULE_STATE_LOADED     2       // Loaded and ready
#define MODULE_STATE_RUNNING    3       // Active and running
#define MODULE_STATE_UNLOADING  4       // Being unloaded
#define MODULE_STATE_ERROR      5       // Error state
#define MODULE_STATE_SUSPENDED  6       // Suspended by system

// Module types
#define MODULE_TYPE_DRIVER      0       // Device driver
#define MODULE_TYPE_FILESYSTEM  1       // File system driver
#define MODULE_TYPE_NETWORK     2       // Network stack component
#define MODULE_TYPE_SCHEDULER   3       // Scheduler extension
#define MODULE_TYPE_MEMORY      4       // Memory management extension
#define MODULE_TYPE_SECURITY    5       // Security module
#define MODULE_TYPE_AI          6       // AI/ML component
#define MODULE_TYPE_USER        7       // User-space service
#define MODULE_TYPE_MISC        8       // Miscellaneous module

// Module flags
#define MODULE_FLAG_CORE        0x01    // Core system module (can't unload)
#define MODULE_FLAG_AUTO_START  0x02    // Auto-start on load
#define MODULE_FLAG_HOT_SWAP    0x04    // Supports hot-swapping
#define MODULE_FLAG_AI_MONITOR  0x08    // Enable AI monitoring
#define MODULE_FLAG_PRIVILEGED  0x10    // Privileged module
#define MODULE_FLAG_PERSISTENT  0x20    // Persistent across reboots

// =============================================================================
// Data Structures
// =============================================================================

/*
 * Module symbol export/import
 */
typedef struct module_symbol {
    char            name[64];           // Symbol name
    void*           address;            // Symbol address
    uint32_t        size;               // Symbol size
    uint8_t         type;               // Symbol type (function, data, etc.)
    uint8_t         visibility;         // Public, private, etc.
} module_symbol_t;

/*
 * Module dependency information
 */
typedef struct module_dependency {
    char            module_name[MAX_MODULE_NAME]; // Required module name
    uint32_t        min_version;        // Minimum required version
    uint32_t        max_version;        // Maximum supported version
    bool            optional;           // Whether dependency is optional
    bool            satisfied;          // Whether dependency is satisfied
} module_dependency_t;

/*
 * Module metadata structure
 */
typedef struct module_header {
    uint32_t        magic;              // Module magic number
    uint32_t        version;            // Module format version
    uint32_t        module_version;     // Module code version
    char            name[MAX_MODULE_NAME]; // Module name
    char            description[256];   // Module description
    char            author[128];        // Module author
    char            license[64];        // License information
    
    uint8_t         type;               // Module type
    uint8_t         priority;           // Load priority
    uint16_t        flags;              // Module flags
    
    uint32_t        code_size;          // Size of code section
    uint32_t        data_size;          // Size of data section
    uint32_t        bss_size;           // Size of BSS section
    uint32_t        entry_point;        // Entry point offset
    uint32_t        exit_point;         // Exit point offset
    
    uint32_t        symbol_count;       // Number of exported symbols
    uint32_t        symbol_table_offset; // Offset to symbol table
    uint32_t        dependency_count;   // Number of dependencies
    uint32_t        dependency_table_offset; // Offset to dependency table
    
    uint32_t        checksum;           // Module checksum
    uint32_t        signature;          // Digital signature (future)
} module_header_t;

/*
 * Runtime module information
 */
typedef struct module {
    uint32_t        module_id;          // Unique module ID
    char            name[MAX_MODULE_NAME]; // Module name
    uint8_t         state;              // Current module state
    uint8_t         type;               // Module type
    uint16_t        flags;              // Module flags
    
    // Memory layout
    void*           base_address;       // Module base address
    void*           code_address;       // Code section address
    void*           data_address;       // Data section address
    size_t          total_size;         // Total memory size
    
    // Module functions
    int             (*init_func)(void); // Module initialization
    void            (*exit_func)(void); // Module cleanup
    int             (*ioctl_func)(uint32_t cmd, void* arg); // Module control
    
    // Symbol management
    module_symbol_t* exported_symbols; // Exported symbols
    uint32_t        symbol_count;       // Number of exported symbols
    
    // Dependency tracking
    module_dependency_t* dependencies; // Module dependencies
    uint32_t        dependency_count;   // Number of dependencies
    struct module*  dependents[MAX_MODULES]; // Modules depending on this one
    uint32_t        dependent_count;    // Number of dependent modules
    
    // Statistics and monitoring
    uint64_t        load_time;          // When module was loaded
    uint64_t        cpu_time;           // CPU time consumed by module
    uint64_t        memory_allocated;   // Memory allocated by module
    uint32_t        function_calls;     // Number of function calls
    uint32_t        error_count;        // Number of errors
    
    // AI supervision data
    uint32_t        behavior_score;     // AI-computed behavior score
    uint32_t        anomaly_count;      // Anomalies detected
    bool            ai_monitored;       // Whether AI monitors this module
    
    // Reference counting
    uint32_t        ref_count;          // Reference count
    
    // Linked list for module management
    struct module*  next;               // Next module in list
    struct module*  prev;               // Previous module in list
} module_t;

/*
 * Module system statistics
 */
typedef struct module_stats {
    uint32_t        modules_loaded;     // Total modules loaded
    uint32_t        modules_unloaded;   // Total modules unloaded
    uint32_t        hot_swaps;          // Hot-swap operations performed
    uint32_t        load_errors;        // Module load errors
    uint32_t        dependency_failures; // Dependency resolution failures
    uint32_t        symbol_lookups;     // Symbol lookup operations
    uint64_t        total_memory_used;  // Total memory used by modules
    uint32_t        ai_interventions;   // AI interventions performed
} module_stats_t;

/*
 * Module system context
 */
typedef struct module_system {
    module_t*       loaded_modules;     // List of loaded modules
    uint32_t        module_count;       // Number of loaded modules
    uint32_t        next_module_id;     // Next available module ID
    
    // Module pools
    module_t        module_pool[MAX_MODULES]; // Module structures pool
    bool            module_pool_used[MAX_MODULES]; // Pool usage tracking
    
    // Symbol table for global lookups
    module_symbol_t* global_symbols;   // Global symbol table
    uint32_t        global_symbol_count; // Global symbol count
    
    // Module system state
    bool            system_enabled;     // Whether module system is active
    bool            hot_swap_enabled;   // Whether hot-swapping is enabled
    bool            ai_supervision;     // Whether AI supervision is enabled
    
    // Statistics
    module_stats_t  statistics;         // Module system statistics
    
    // Security and validation
    bool            signature_checking; // Whether to check signatures
    bool            sandboxing_enabled; // Whether modules are sandboxed
    
} module_system_t;

// =============================================================================
// Core Module System Functions
// =============================================================================

/*
 * Initialize the module system
 */
void modules_init(void);

/*
 * Load a module from memory
 */
uint32_t module_load(void* module_data, size_t size);

/*
 * Load a module by name (from storage)
 */
uint32_t module_load_by_name(const char* module_name);

/*
 * Unload a module
 */
bool module_unload(uint32_t module_id);

/*
 * Hot-swap a module (replace with new version)
 */
bool module_hot_swap(uint32_t module_id, void* new_module_data, size_t size);

/*
 * Get module by ID
 */
module_t* module_get(uint32_t module_id);

/*
 * Find module by name
 */
module_t* module_find_by_name(const char* name);

// =============================================================================
// Symbol Management Functions
// =============================================================================

/*
 * Resolve a symbol by name
 */
void* module_resolve_symbol(const char* symbol_name);

/*
 * Export a symbol from a module
 */
bool module_export_symbol(uint32_t module_id, const char* name, void* address, uint32_t size);

/*
 * Import a symbol into a module
 */
void* module_import_symbol(uint32_t module_id, const char* symbol_name);

/*
 * List all exported symbols from a module
 */
module_symbol_t* module_list_symbols(uint32_t module_id, uint32_t* count);

// =============================================================================
// Dependency Management
// =============================================================================

/*
 * Check module dependencies
 */
bool module_check_dependencies(module_t* module);

/*
 * Resolve all dependencies for a module
 */
bool module_resolve_dependencies(module_t* module);

/*
 * Add a dependency relationship
 */
bool module_add_dependency(uint32_t module_id, uint32_t dependency_id);

/*
 * Remove a dependency relationship
 */
bool module_remove_dependency(uint32_t module_id, uint32_t dependency_id);

/*
 * Get modules that depend on the given module
 */
module_t** module_get_dependents(uint32_t module_id, uint32_t* count);

// =============================================================================
// Module Control and Communication
// =============================================================================

/*
 * Send control command to module
 */
int module_ioctl(uint32_t module_id, uint32_t command, void* argument);

/*
 * Start a module (call its init function)
 */
bool module_start(uint32_t module_id);

/*
 * Stop a module (call its exit function)
 */
bool module_stop(uint32_t module_id);

/*
 * Suspend a module
 */
bool module_suspend(uint32_t module_id);

/*
 * Resume a module
 */
bool module_resume(uint32_t module_id);

// =============================================================================
// Statistics and Monitoring
// =============================================================================

/*
 * Get module system statistics
 */
module_stats_t* module_get_statistics(void);

/*
 * Print module system status
 */
void module_print_status(void);

/*
 * Print information about all loaded modules
 */
void module_print_modules(void);

/*
 * Get module performance statistics
 */
void module_get_performance_stats(uint32_t module_id, 
                                  uint64_t* cpu_time, 
                                  uint64_t* memory_used);

// =============================================================================
// AI Integration Functions
// =============================================================================

/*
 * AI-based module health monitoring
 */
void module_ai_health_check(void);

/*
 * AI-assisted dependency resolution
 */
bool module_ai_resolve_conflicts(uint32_t* conflicting_modules, uint32_t count);

/*
 * AI-optimized module loading order
 */
uint32_t* module_ai_optimize_load_order(uint32_t* modules, uint32_t count);

/*
 * AI behavior analysis for modules
 */
void module_ai_analyze_behavior(uint32_t module_id);

// =============================================================================
// Security and Validation Functions
// =============================================================================

/*
 * Validate module format and integrity
 */
bool module_validate(void* module_data, size_t size);

/*
 * Check module digital signature
 */
bool module_verify_signature(module_header_t* header);

/*
 * Sandbox a module (restrict its capabilities)
 */
bool module_enable_sandbox(uint32_t module_id);

/*
 * Check module permissions for operation
 */
bool module_check_permission(uint32_t module_id, uint32_t operation);

// =============================================================================
// Debug and Diagnostic Functions
// =============================================================================

/*
 * Dump module system state
 */
void module_dump_state(void);

/*
 * Validate module system integrity
 */
bool module_validate_system(void);

/*
 * Test module system functionality
 */
void module_test_functionality(void);

/*
 * Benchmark module loading performance
 */
void module_benchmark_loading(void);

// =============================================================================
// Utility Functions and Macros
// =============================================================================

/*
 * Get module state name
 */
static inline const char* module_state_name(uint8_t state)
{
    const char* states[] = {
        "UNLOADED", "LOADING", "LOADED", "RUNNING", 
        "UNLOADING", "ERROR", "SUSPENDED"
    };
    return (state < 7) ? states[state] : "UNKNOWN";
}

/*
 * Get module type name
 */
static inline const char* module_type_name(uint8_t type)
{
    const char* types[] = {
        "DRIVER", "FILESYSTEM", "NETWORK", "SCHEDULER", 
        "MEMORY", "SECURITY", "AI", "USER", "MISC"
    };
    return (type < 9) ? types[type] : "UNKNOWN";
}

/*
 * Check if module supports hot-swapping
 */
static inline bool module_supports_hot_swap(module_t* module)
{
    return module && (module->flags & MODULE_FLAG_HOT_SWAP);
}

/*
 * Check if module is core system module
 */
static inline bool module_is_core(module_t* module)
{
    return module && (module->flags & MODULE_FLAG_CORE);
}

// =============================================================================
// Module API Macros for Module Developers
// =============================================================================

#define MODULE_DEFINE(name, version, type, flags) \
    static module_header_t __module_header __attribute__((section(".module_header"))) = { \
        .magic = MODULE_MAGIC, \
        .version = MODULE_VERSION, \
        .module_version = version, \
        .name = name, \
        .type = type, \
        .flags = flags \
    }

#define MODULE_EXPORT(symbol) \
    __attribute__((visibility("default"))) symbol

#define MODULE_IMPORT(symbol) \
    extern symbol

#define MODULE_INIT(func) \
    int module_init(void) { return func(); }

#define MODULE_EXIT(func) \
    void module_exit(void) { func(); }

#endif // MODULES_H
