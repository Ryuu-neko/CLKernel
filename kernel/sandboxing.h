/*
 * =============================================================================
 * CLKernel - Module Sandboxing Engine
 * =============================================================================
 * File: sandboxing.h
 * Purpose: Module isolation and security enforcement
 *
 * This system provides:
 * - Capability-based module security
 * - WASM-like execution environment for modules
 * - Resource access control and limits
 * - Module isolation and containment
 * =============================================================================
 */

#ifndef SANDBOXING_H
#define SANDBOXING_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Sandboxing Constants
// =============================================================================

#define MAX_SANDBOXES           32      // Maximum number of sandboxes
#define MAX_CAPABILITIES        64      // Maximum capabilities per sandbox
#define MAX_RESOURCE_LIMITS     16      // Maximum resource limits
#define MAX_VIOLATION_LOG       100     // Maximum security violations logged

// Security levels
#define SANDBOX_LEVEL_UNRESTRICTED  0   // No restrictions (kernel modules)
#define SANDBOX_LEVEL_TRUSTED       1   // Trusted modules (built-in)
#define SANDBOX_LEVEL_USER          2   // User modules (default)
#define SANDBOX_LEVEL_UNTRUSTED     3   // Untrusted modules (strict)
#define SANDBOX_LEVEL_QUARANTINE    4   // Quarantined modules (read-only)

// Capability flags
#define CAP_MEMORY_ALLOC        0x00000001  // Allocate memory
#define CAP_MEMORY_FREE         0x00000002  // Free memory
#define CAP_MEMORY_MAP          0x00000004  // Map memory pages
#define CAP_MEMORY_UNMAP        0x00000008  // Unmap memory pages
#define CAP_MEMORY_PROTECT      0x00000010  // Change memory protection

#define CAP_SCHEDULER_CREATE    0x00000020  // Create actors
#define CAP_SCHEDULER_DESTROY   0x00000040  // Destroy actors
#define CAP_SCHEDULER_MODIFY    0x00000080  // Modify actor properties
#define CAP_SCHEDULER_SIGNAL    0x00000100  // Send signals to actors

#define CAP_MODULE_LOAD         0x00000200  // Load other modules
#define CAP_MODULE_UNLOAD       0x00000400  // Unload modules
#define CAP_MODULE_QUERY        0x00000800  // Query module information

#define CAP_VGA_WRITE           0x00001000  // Write to VGA buffer
#define CAP_VGA_CLEAR           0x00002000  // Clear VGA screen
#define CAP_VGA_CURSOR          0x00004000  // Control cursor

#define CAP_HARDWARE_IO         0x00008000  // Hardware I/O operations
#define CAP_INTERRUPT_HANDLE    0x00010000  // Handle interrupts
#define CAP_TIMER_ACCESS        0x00020000  // Access timer hardware

#define CAP_FILESYSTEM_READ     0x00040000  // Read files
#define CAP_FILESYSTEM_WRITE    0x00080000  // Write files
#define CAP_FILESYSTEM_CREATE   0x00100000  // Create files/directories
#define CAP_FILESYSTEM_DELETE   0x00200000  // Delete files/directories

#define CAP_NETWORK_SEND        0x00400000  // Send network packets
#define CAP_NETWORK_RECV        0x00800000  // Receive network packets
#define CAP_NETWORK_SOCKET      0x01000000  // Create network sockets

#define CAP_AI_QUERY            0x02000000  // Query AI supervisor
#define CAP_AI_CONFIGURE        0x04000000  // Configure AI parameters

#define CAP_DEBUG_ACCESS        0x08000000  // Access debug functions
#define CAP_SYSTEM_SHUTDOWN     0x10000000  // System shutdown/restart
#define CAP_SECURITY_OVERRIDE   0x20000000  // Override security policies

// Resource types
#define RESOURCE_MEMORY         0   // Memory usage limit
#define RESOURCE_CPU_TIME       1   // CPU time limit
#define RESOURCE_FILE_HANDLES   2   // File handle limit
#define RESOURCE_NETWORK_CONN   3   // Network connection limit
#define RESOURCE_CHILD_ACTORS   4   // Child actor limit
#define RESOURCE_HEAP_ALLOCS    5   // Heap allocation limit
#define RESOURCE_MODULE_CALLS   6   // Module function call limit
#define RESOURCE_AI_QUERIES     7   // AI supervisor query limit

// Violation types
#define VIOLATION_CAPABILITY    0   // Capability violation
#define VIOLATION_RESOURCE      1   // Resource limit exceeded
#define VIOLATION_MEMORY        2   // Memory access violation
#define VIOLATION_EXECUTION     3   // Code execution violation
#define VIOLATION_POLICY        4   // Policy violation

// =============================================================================
// Sandboxing Data Structures
// =============================================================================

typedef struct resource_limit {
    uint8_t     resource_type;          // Resource type
    uint32_t    limit;                  // Resource limit
    uint32_t    current_usage;          // Current usage
    uint32_t    peak_usage;             // Peak usage recorded
    bool        enforce;                // Whether to enforce limit
    
} resource_limit_t;

typedef struct security_violation {
    uint32_t    violation_id;           // Unique violation ID
    uint64_t    timestamp;              // When violation occurred
    uint32_t    module_id;              // Module that violated policy
    uint8_t     violation_type;         // Type of violation
    uint32_t    attempted_capability;   // Capability that was denied
    uint32_t    attempted_resource;     // Resource that was exceeded
    char        description[128];       // Human-readable description
    bool        action_taken;           // Whether corrective action was taken
    
} security_violation_t;

typedef struct sandbox_context {
    uint32_t    sandbox_id;             // Unique sandbox ID
    uint32_t    module_id;              // Associated module ID
    uint8_t     security_level;         // Security level
    bool        active;                 // Whether sandbox is active
    
    // Capabilities
    uint32_t    capabilities;           // Allowed capabilities bitmask
    uint32_t    denied_capabilities;    // Explicitly denied capabilities
    
    // Resource limits
    resource_limit_t limits[MAX_RESOURCE_LIMITS]; // Resource limits
    uint32_t    limit_count;            // Number of active limits
    
    // Execution context
    void*       memory_base;            // Base address of module memory
    uint32_t    memory_size;            // Size of allocated memory
    void*       stack_base;             // Base of execution stack
    uint32_t    stack_size;             // Size of execution stack
    
    // Statistics
    uint64_t    function_calls;         // Total function calls made
    uint64_t    memory_allocations;     // Total memory allocations
    uint64_t    capability_checks;      // Total capability checks
    uint64_t    violations;             // Total violations
    uint32_t    last_violation_id;      // ID of last violation
    
    // WASM-like VM state
    bool        vm_enabled;             // Whether VM is enabled
    void*       vm_context;             // VM execution context
    uint32_t    vm_instruction_count;   // Instructions executed
    uint32_t    vm_instruction_limit;   // Instruction limit
    
} sandbox_context_t;

typedef struct sandboxing_system_state {
    sandbox_context_t sandboxes[MAX_SANDBOXES]; // Active sandboxes
    uint32_t    sandbox_count;          // Number of active sandboxes
    uint32_t    next_sandbox_id;        // Next sandbox ID to assign
    
    // Security policy
    uint32_t    default_capabilities[5]; // Default capabilities by security level
    bool        strict_enforcement;     // Strict enforcement mode
    bool        logging_enabled;        // Security logging enabled
    
    // Violation tracking
    security_violation_t violations[MAX_VIOLATION_LOG]; // Violation log
    uint32_t    violation_count;        // Number of violations logged
    uint32_t    violation_index;        // Current violation index
    uint32_t    next_violation_id;      // Next violation ID
    
    // Statistics
    uint64_t    total_capability_checks; // Total capability checks
    uint64_t    total_violations;       // Total violations
    uint64_t    total_enforcements;     // Total enforcement actions
    uint32_t    quarantined_modules;    // Number of quarantined modules
    
} sandboxing_system_state_t;

// =============================================================================
// Function Declarations
// =============================================================================

// Sandbox management
int sandboxing_init(void);
void sandboxing_shutdown(void);
int sandboxing_create_sandbox(uint32_t module_id, uint8_t security_level);
int sandboxing_destroy_sandbox(uint32_t sandbox_id);
sandbox_context_t* sandboxing_get_sandbox(uint32_t module_id);

// Capability management
bool sandboxing_has_capability(uint32_t module_id, uint32_t capability);
int sandboxing_grant_capability(uint32_t module_id, uint32_t capability);
int sandboxing_revoke_capability(uint32_t module_id, uint32_t capability);
int sandboxing_set_capabilities(uint32_t module_id, uint32_t capabilities);

// Resource management
int sandboxing_set_resource_limit(uint32_t module_id, uint8_t resource_type, uint32_t limit);
uint32_t sandboxing_get_resource_usage(uint32_t module_id, uint8_t resource_type);
bool sandboxing_check_resource_limit(uint32_t module_id, uint8_t resource_type, uint32_t requested);
int sandboxing_update_resource_usage(uint32_t module_id, uint8_t resource_type, int32_t delta);

// Security enforcement
bool sandboxing_check_memory_access(uint32_t module_id, void* address, uint32_t size, bool write);
bool sandboxing_check_function_call(uint32_t module_id, const char* function_name);
int sandboxing_handle_violation(uint32_t module_id, uint8_t violation_type, const char* description);

// WASM-like VM
int sandboxing_enable_vm(uint32_t module_id);
int sandboxing_disable_vm(uint32_t module_id);
bool sandboxing_vm_execute_instruction(uint32_t module_id, void* instruction);
int sandboxing_vm_set_instruction_limit(uint32_t module_id, uint32_t limit);

// Policy management
int sandboxing_set_security_policy(uint8_t security_level, uint32_t capabilities);
int sandboxing_enable_strict_mode(bool enabled);
int sandboxing_quarantine_module(uint32_t module_id);

// Monitoring and reporting
void sandboxing_print_status(void);
void sandboxing_print_sandbox_info(uint32_t sandbox_id);
void sandboxing_print_violations(uint32_t count);
int sandboxing_export_violation_log(void* buffer, uint32_t buffer_size);

// Internal functions
sandbox_context_t* sandboxing_find_sandbox_by_module(uint32_t module_id);
int sandboxing_init_default_policies(void);
void sandboxing_log_violation(uint32_t module_id, uint8_t violation_type, 
                             uint32_t capability, const char* description);
bool sandboxing_enforce_violation(sandbox_context_t* sandbox, security_violation_t* violation);

#endif // SANDBOXING_H
