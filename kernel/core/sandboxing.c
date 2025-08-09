/*
 * =============================================================================
 * CLKernel - Module Sandboxing Engine Implementation
 * =============================================================================
 * File: sandboxing.c
 * Purpose: Module isolation and security enforcement implementation
 *
 * This system provides:
 * - Capability-based module security
 * - WASM-like execution environment for modules
 * - Resource access control and limits
 * - Module isolation and containment
 * =============================================================================
 */

#include "sandboxing.h"
#include "kernel.h"
#include "modules.h"
#include "memory.h"
#include "heap.h"
#include "vga.h"

// =============================================================================
// Global State
// =============================================================================

static sandboxing_system_state_t sandbox_system;
static bool sandboxing_initialized = false;

// =============================================================================
// Initialization and Shutdown
// =============================================================================

/*
 * Initialize sandboxing system
 */
int sandboxing_init(void)
{
    kprintf("[SANDBOX] Initializing module sandboxing system...\n");
    
    // Clear system state
    sandbox_system.sandbox_count = 0;
    sandbox_system.next_sandbox_id = 1;
    sandbox_system.strict_enforcement = true;
    sandbox_system.logging_enabled = true;
    
    // Clear sandbox contexts
    for (int i = 0; i < MAX_SANDBOXES; i++) {
        sandbox_system.sandboxes[i].active = false;
    }
    
    // Clear violation log
    sandbox_system.violation_count = 0;
    sandbox_system.violation_index = 0;
    sandbox_system.next_violation_id = 1;
    
    // Clear statistics
    sandbox_system.total_capability_checks = 0;
    sandbox_system.total_violations = 0;
    sandbox_system.total_enforcements = 0;
    sandbox_system.quarantined_modules = 0;
    
    // Initialize default security policies
    sandboxing_init_default_policies();
    
    sandboxing_initialized = true;
    
    kprintf("[SANDBOX] Sandboxing system initialized\n");
    kprintf("[SANDBOX] Max sandboxes: %d\n", MAX_SANDBOXES);
    kprintf("[SANDBOX] Max capabilities: %d\n", MAX_CAPABILITIES);
    kprintf("[SANDBOX] Strict enforcement: %s\n", 
            sandbox_system.strict_enforcement ? "ENABLED" : "DISABLED");
    
    return 0;
}

/*
 * Shutdown sandboxing system
 */
void sandboxing_shutdown(void)
{
    if (!sandboxing_initialized) return;
    
    kprintf("[SANDBOX] Shutting down sandboxing system...\n");
    
    // Destroy all active sandboxes
    for (int i = 0; i < MAX_SANDBOXES; i++) {
        if (sandbox_system.sandboxes[i].active) {
            sandboxing_destroy_sandbox(sandbox_system.sandboxes[i].sandbox_id);
        }
    }
    
    // Print final statistics
    kprintf("[SANDBOX] Final statistics:\n");
    kprintf("[SANDBOX]   Total capability checks: %llu\n", 
            sandbox_system.total_capability_checks);
    kprintf("[SANDBOX]   Total violations: %llu\n", 
            sandbox_system.total_violations);
    kprintf("[SANDBOX]   Total enforcements: %llu\n", 
            sandbox_system.total_enforcements);
    kprintf("[SANDBOX]   Quarantined modules: %d\n", 
            sandbox_system.quarantined_modules);
    
    sandboxing_initialized = false;
    
    kprintf("[SANDBOX] Sandboxing system shut down\n");
}

/*
 * Initialize default security policies
 */
int sandboxing_init_default_policies(void)
{
    // Unrestricted (kernel modules)
    sandbox_system.default_capabilities[SANDBOX_LEVEL_UNRESTRICTED] = 0xFFFFFFFF; // All capabilities
    
    // Trusted (built-in modules)
    sandbox_system.default_capabilities[SANDBOX_LEVEL_TRUSTED] = 
        CAP_MEMORY_ALLOC | CAP_MEMORY_FREE | CAP_SCHEDULER_CREATE | CAP_SCHEDULER_SIGNAL |
        CAP_MODULE_QUERY | CAP_VGA_WRITE | CAP_VGA_CLEAR | CAP_TIMER_ACCESS |
        CAP_AI_QUERY | CAP_DEBUG_ACCESS;
    
    // User (default for user modules)
    sandbox_system.default_capabilities[SANDBOX_LEVEL_USER] = 
        CAP_MEMORY_ALLOC | CAP_MEMORY_FREE | CAP_SCHEDULER_SIGNAL |
        CAP_MODULE_QUERY | CAP_VGA_WRITE | CAP_TIMER_ACCESS |
        CAP_AI_QUERY;
    
    // Untrusted (strict limitations)
    sandbox_system.default_capabilities[SANDBOX_LEVEL_UNTRUSTED] = 
        CAP_MEMORY_ALLOC | CAP_MEMORY_FREE | CAP_MODULE_QUERY;
    
    // Quarantine (read-only access)
    sandbox_system.default_capabilities[SANDBOX_LEVEL_QUARANTINE] = 
        CAP_MODULE_QUERY;
    
    return 0;
}

// =============================================================================
// Sandbox Management
// =============================================================================

/*
 * Create a new sandbox for a module
 */
int sandboxing_create_sandbox(uint32_t module_id, uint8_t security_level)
{
    if (!sandboxing_initialized) return -1;
    
    if (sandbox_system.sandbox_count >= MAX_SANDBOXES) {
        kprintf("[SANDBOX] Cannot create sandbox: maximum limit reached\n");
        return -2;
    }
    
    if (security_level > SANDBOX_LEVEL_QUARANTINE) {
        kprintf("[SANDBOX] Invalid security level: %d\n", security_level);
        return -3;
    }
    
    // Check if module already has a sandbox
    if (sandboxing_find_sandbox_by_module(module_id) != NULL) {
        kprintf("[SANDBOX] Module %d already has a sandbox\n", module_id);
        return -4;
    }
    
    // Find free sandbox slot
    sandbox_context_t* sandbox = NULL;
    for (int i = 0; i < MAX_SANDBOXES; i++) {
        if (!sandbox_system.sandboxes[i].active) {
            sandbox = &sandbox_system.sandboxes[i];
            break;
        }
    }
    
    if (sandbox == NULL) {
        kprintf("[SANDBOX] No free sandbox slots available\n");
        return -5;
    }
    
    // Initialize sandbox context
    sandbox->sandbox_id = sandbox_system.next_sandbox_id++;
    sandbox->module_id = module_id;
    sandbox->security_level = security_level;
    sandbox->active = true;
    
    // Set default capabilities based on security level
    sandbox->capabilities = sandbox_system.default_capabilities[security_level];
    sandbox->denied_capabilities = 0;
    
    // Initialize resource limits
    sandbox->limit_count = 0;
    
    // Set default resource limits based on security level
    switch (security_level) {
        case SANDBOX_LEVEL_UNRESTRICTED:
            // No limits for kernel modules
            break;
            
        case SANDBOX_LEVEL_TRUSTED:
            sandboxing_set_resource_limit(module_id, RESOURCE_MEMORY, 4 * 1024 * 1024); // 4MB
            sandboxing_set_resource_limit(module_id, RESOURCE_CHILD_ACTORS, 10);
            sandboxing_set_resource_limit(module_id, RESOURCE_HEAP_ALLOCS, 1000);
            break;
            
        case SANDBOX_LEVEL_USER:
            sandboxing_set_resource_limit(module_id, RESOURCE_MEMORY, 2 * 1024 * 1024); // 2MB
            sandboxing_set_resource_limit(module_id, RESOURCE_CHILD_ACTORS, 5);
            sandboxing_set_resource_limit(module_id, RESOURCE_HEAP_ALLOCS, 500);
            sandboxing_set_resource_limit(module_id, RESOURCE_MODULE_CALLS, 1000);
            break;
            
        case SANDBOX_LEVEL_UNTRUSTED:
            sandboxing_set_resource_limit(module_id, RESOURCE_MEMORY, 1 * 1024 * 1024); // 1MB
            sandboxing_set_resource_limit(module_id, RESOURCE_CHILD_ACTORS, 2);
            sandboxing_set_resource_limit(module_id, RESOURCE_HEAP_ALLOCS, 100);
            sandboxing_set_resource_limit(module_id, RESOURCE_MODULE_CALLS, 500);
            sandboxing_set_resource_limit(module_id, RESOURCE_AI_QUERIES, 10);
            break;
            
        case SANDBOX_LEVEL_QUARANTINE:
            sandboxing_set_resource_limit(module_id, RESOURCE_MEMORY, 512 * 1024); // 512KB
            sandboxing_set_resource_limit(module_id, RESOURCE_CHILD_ACTORS, 0);
            sandboxing_set_resource_limit(module_id, RESOURCE_HEAP_ALLOCS, 10);
            sandboxing_set_resource_limit(module_id, RESOURCE_MODULE_CALLS, 100);
            break;
    }
    
    // Initialize execution context (simplified)
    sandbox->memory_base = NULL; // Would allocate isolated memory region
    sandbox->memory_size = 0;
    sandbox->stack_base = NULL;
    sandbox->stack_size = 0;
    
    // Clear statistics
    sandbox->function_calls = 0;
    sandbox->memory_allocations = 0;
    sandbox->capability_checks = 0;
    sandbox->violations = 0;
    sandbox->last_violation_id = 0;
    
    // VM initialization
    sandbox->vm_enabled = false;
    sandbox->vm_context = NULL;
    sandbox->vm_instruction_count = 0;
    sandbox->vm_instruction_limit = 1000000; // Default limit
    
    sandbox_system.sandbox_count++;
    
    kprintf("[SANDBOX] Created sandbox %d for module %d (level %d)\n",
            sandbox->sandbox_id, module_id, security_level);
    kprintf("[SANDBOX] Default capabilities: 0x%x\n", sandbox->capabilities);
    
    return sandbox->sandbox_id;
}

/*
 * Destroy a sandbox
 */
int sandboxing_destroy_sandbox(uint32_t sandbox_id)
{
    if (!sandboxing_initialized) return -1;
    
    sandbox_context_t* sandbox = NULL;
    for (int i = 0; i < MAX_SANDBOXES; i++) {
        if (sandbox_system.sandboxes[i].active && 
            sandbox_system.sandboxes[i].sandbox_id == sandbox_id) {
            sandbox = &sandbox_system.sandboxes[i];
            break;
        }
    }
    
    if (sandbox == NULL) {
        kprintf("[SANDBOX] Sandbox %d not found\n", sandbox_id);
        return -2;
    }
    
    kprintf("[SANDBOX] Destroying sandbox %d (module %d)\n",
            sandbox_id, sandbox->module_id);
    
    // Cleanup VM context if enabled
    if (sandbox->vm_enabled) {
        sandboxing_disable_vm(sandbox->module_id);
    }
    
    // Free allocated memory (if any)
    if (sandbox->memory_base != NULL) {
        // heap_free(sandbox->memory_base); // Would free isolated memory
    }
    
    // Mark as inactive
    sandbox->active = false;
    sandbox_system.sandbox_count--;
    
    return 0;
}

/*
 * Get sandbox context for a module
 */
sandbox_context_t* sandboxing_get_sandbox(uint32_t module_id)
{
    return sandboxing_find_sandbox_by_module(module_id);
}

/*
 * Find sandbox by module ID
 */
sandbox_context_t* sandboxing_find_sandbox_by_module(uint32_t module_id)
{
    if (!sandboxing_initialized) return NULL;
    
    for (int i = 0; i < MAX_SANDBOXES; i++) {
        if (sandbox_system.sandboxes[i].active && 
            sandbox_system.sandboxes[i].module_id == module_id) {
            return &sandbox_system.sandboxes[i];
        }
    }
    
    return NULL;
}

// =============================================================================
// Capability Management
// =============================================================================

/*
 * Check if module has capability
 */
bool sandboxing_has_capability(uint32_t module_id, uint32_t capability)
{
    if (!sandboxing_initialized) return true; // Fail-open if not initialized
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) {
        // No sandbox means unrestricted access (kernel modules)
        return true;
    }
    
    sandbox->capability_checks++;
    sandbox_system.total_capability_checks++;
    
    // Check if explicitly denied
    if (sandbox->denied_capabilities & capability) {
        sandboxing_log_violation(module_id, VIOLATION_CAPABILITY, capability,
                                "Capability explicitly denied");
        return false;
    }
    
    // Check if capability is granted
    bool has_cap = (sandbox->capabilities & capability) != 0;
    
    if (!has_cap) {
        sandboxing_log_violation(module_id, VIOLATION_CAPABILITY, capability,
                                "Capability not granted");
    }
    
    return has_cap;
}

/*
 * Grant capability to module
 */
int sandboxing_grant_capability(uint32_t module_id, uint32_t capability)
{
    if (!sandboxing_initialized) return -1;
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) {
        kprintf("[SANDBOX] Cannot grant capability: module %d has no sandbox\n", module_id);
        return -2;
    }
    
    // Remove from denied capabilities
    sandbox->denied_capabilities &= ~capability;
    
    // Add to granted capabilities
    sandbox->capabilities |= capability;
    
    kprintf("[SANDBOX] Granted capability 0x%x to module %d\n", capability, module_id);
    return 0;
}

/*
 * Revoke capability from module
 */
int sandboxing_revoke_capability(uint32_t module_id, uint32_t capability)
{
    if (!sandboxing_initialized) return -1;
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) {
        kprintf("[SANDBOX] Cannot revoke capability: module %d has no sandbox\n", module_id);
        return -2;
    }
    
    // Remove from granted capabilities
    sandbox->capabilities &= ~capability;
    
    // Add to denied capabilities
    sandbox->denied_capabilities |= capability;
    
    kprintf("[SANDBOX] Revoked capability 0x%x from module %d\n", capability, module_id);
    return 0;
}

/*
 * Set all capabilities for module
 */
int sandboxing_set_capabilities(uint32_t module_id, uint32_t capabilities)
{
    if (!sandboxing_initialized) return -1;
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) {
        kprintf("[SANDBOX] Cannot set capabilities: module %d has no sandbox\n", module_id);
        return -2;
    }
    
    sandbox->capabilities = capabilities;
    sandbox->denied_capabilities = 0; // Clear denied capabilities
    
    kprintf("[SANDBOX] Set capabilities 0x%x for module %d\n", capabilities, module_id);
    return 0;
}

// =============================================================================
// Resource Management
// =============================================================================

/*
 * Set resource limit for module
 */
int sandboxing_set_resource_limit(uint32_t module_id, uint8_t resource_type, uint32_t limit)
{
    if (!sandboxing_initialized) return -1;
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) {
        kprintf("[SANDBOX] Cannot set resource limit: module %d has no sandbox\n", module_id);
        return -2;
    }
    
    if (resource_type >= MAX_RESOURCE_LIMITS) {
        kprintf("[SANDBOX] Invalid resource type: %d\n", resource_type);
        return -3;
    }
    
    // Find existing limit or create new one
    resource_limit_t* res_limit = NULL;
    for (int i = 0; i < sandbox->limit_count; i++) {
        if (sandbox->limits[i].resource_type == resource_type) {
            res_limit = &sandbox->limits[i];
            break;
        }
    }
    
    if (res_limit == NULL && sandbox->limit_count < MAX_RESOURCE_LIMITS) {
        res_limit = &sandbox->limits[sandbox->limit_count++];
        res_limit->resource_type = resource_type;
        res_limit->current_usage = 0;
        res_limit->peak_usage = 0;
    }
    
    if (res_limit == NULL) {
        kprintf("[SANDBOX] Cannot add resource limit: maximum limits reached\n");
        return -4;
    }
    
    res_limit->limit = limit;
    res_limit->enforce = true;
    
    kprintf("[SANDBOX] Set resource limit %d = %d for module %d\n", 
            resource_type, limit, module_id);
    return 0;
}

/*
 * Get current resource usage
 */
uint32_t sandboxing_get_resource_usage(uint32_t module_id, uint8_t resource_type)
{
    if (!sandboxing_initialized) return 0;
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) return 0;
    
    for (int i = 0; i < sandbox->limit_count; i++) {
        if (sandbox->limits[i].resource_type == resource_type) {
            return sandbox->limits[i].current_usage;
        }
    }
    
    return 0;
}

/*
 * Check if resource allocation would exceed limit
 */
bool sandboxing_check_resource_limit(uint32_t module_id, uint8_t resource_type, uint32_t requested)
{
    if (!sandboxing_initialized) return true; // Allow if not initialized
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) return true; // Allow if no sandbox
    
    for (int i = 0; i < sandbox->limit_count; i++) {
        if (sandbox->limits[i].resource_type == resource_type) {
            resource_limit_t* limit = &sandbox->limits[i];
            
            if (!limit->enforce) return true;
            
            if (limit->current_usage + requested > limit->limit) {
                sandboxing_log_violation(module_id, VIOLATION_RESOURCE, resource_type,
                                        "Resource limit exceeded");
                return false;
            }
            
            return true;
        }
    }
    
    return true; // No limit set
}

/*
 * Update resource usage
 */
int sandboxing_update_resource_usage(uint32_t module_id, uint8_t resource_type, int32_t delta)
{
    if (!sandboxing_initialized) return 0;
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) return 0;
    
    for (int i = 0; i < sandbox->limit_count; i++) {
        if (sandbox->limits[i].resource_type == resource_type) {
            resource_limit_t* limit = &sandbox->limits[i];
            
            if (delta < 0 && (uint32_t)(-delta) > limit->current_usage) {
                limit->current_usage = 0;
            } else {
                limit->current_usage += delta;
            }
            
            if (limit->current_usage > limit->peak_usage) {
                limit->peak_usage = limit->current_usage;
            }
            
            return 0;
        }
    }
    
    return -1; // Resource type not found
}

// =============================================================================
// Security Enforcement
// =============================================================================

/*
 * Check memory access permissions
 */
bool sandboxing_check_memory_access(uint32_t module_id, void* address, uint32_t size, bool write)
{
    if (!sandboxing_initialized) return true;
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) return true; // No sandbox = no restrictions
    
    // Check basic memory capabilities
    uint32_t required_cap = write ? CAP_MEMORY_ALLOC : CAP_MEMORY_ALLOC;
    if (!sandboxing_has_capability(module_id, required_cap)) {
        return false;
    }
    
    // TODO: Check if address is within module's allocated memory region
    // For now, just basic validation
    if (address == NULL || size == 0) {
        sandboxing_log_violation(module_id, VIOLATION_MEMORY, 0,
                                "Invalid memory access parameters");
        return false;
    }
    
    return true;
}

/*
 * Check function call permissions
 */
bool sandboxing_check_function_call(uint32_t module_id, const char* function_name)
{
    if (!sandboxing_initialized) return true;
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) return true;
    
    sandbox->function_calls++;
    
    // Check module call limits
    sandboxing_update_resource_usage(module_id, RESOURCE_MODULE_CALLS, 1);
    if (!sandboxing_check_resource_limit(module_id, RESOURCE_MODULE_CALLS, 0)) {
        return false;
    }
    
    // Simple function name-based restrictions
    if (function_name != NULL) {
        // Check for dangerous function calls
        const char* restricted_funcs[] = {
            "system", "exec", "fork", "kill", "reboot", "shutdown"
        };
        
        for (int i = 0; i < 6; i++) {
            const char* restricted = restricted_funcs[i];
            bool match = true;
            
            for (int j = 0; function_name[j] != '\0' && restricted[j] != '\0'; j++) {
                if (function_name[j] != restricted[j]) {
                    match = false;
                    break;
                }
            }
            
            if (match) {
                sandboxing_log_violation(module_id, VIOLATION_EXECUTION, 0,
                                        "Restricted function call");
                return false;
            }
        }
    }
    
    return true;
}

/*
 * Handle security violation
 */
int sandboxing_handle_violation(uint32_t module_id, uint8_t violation_type, const char* description)
{
    if (!sandboxing_initialized) return -1;
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox != NULL) {
        sandbox->violations++;
    }
    
    sandboxing_log_violation(module_id, violation_type, 0, description);
    
    // Take enforcement action based on severity and policy
    if (sandbox_system.strict_enforcement) {
        if (violation_type == VIOLATION_CAPABILITY || violation_type == VIOLATION_EXECUTION) {
            // Severe violations might quarantine the module
            if (sandbox != NULL && sandbox->violations > 5) {
                kprintf("[SANDBOX] Module %d quarantined due to repeated violations\n", module_id);
                sandboxing_quarantine_module(module_id);
            }
        }
    }
    
    return 0;
}

/*
 * Log security violation
 */
void sandboxing_log_violation(uint32_t module_id, uint8_t violation_type, 
                             uint32_t capability, const char* description)
{
    if (!sandbox_system.logging_enabled) return;
    
    security_violation_t* violation = &sandbox_system.violations[sandbox_system.violation_index];
    
    violation->violation_id = sandbox_system.next_violation_id++;
    violation->timestamp = 0; // TODO: Get real timestamp
    violation->module_id = module_id;
    violation->violation_type = violation_type;
    violation->attempted_capability = capability;
    violation->attempted_resource = 0;
    violation->action_taken = false;
    
    // Copy description
    if (description != NULL) {
        for (int i = 0; i < 127 && description[i] != '\0'; i++) {
            violation->description[i] = description[i];
        }
        violation->description[127] = '\0';
    } else {
        violation->description[0] = '\0';
    }
    
    // Update indices and counts
    sandbox_system.violation_index = (sandbox_system.violation_index + 1) % MAX_VIOLATION_LOG;
    if (sandbox_system.violation_count < MAX_VIOLATION_LOG) {
        sandbox_system.violation_count++;
    }
    
    sandbox_system.total_violations++;
    
    kprintf("[SANDBOX] VIOLATION: Module %d, Type %d, %s\n",
            module_id, violation_type, description ? description : "Unknown");
}

// =============================================================================
// Policy Management
// =============================================================================

/*
 * Quarantine a module
 */
int sandboxing_quarantine_module(uint32_t module_id)
{
    if (!sandboxing_initialized) return -1;
    
    sandbox_context_t* sandbox = sandboxing_find_sandbox_by_module(module_id);
    if (sandbox == NULL) {
        kprintf("[SANDBOX] Cannot quarantine: module %d has no sandbox\n", module_id);
        return -2;
    }
    
    kprintf("[SANDBOX] Quarantining module %d\n", module_id);
    
    // Set quarantine security level
    sandbox->security_level = SANDBOX_LEVEL_QUARANTINE;
    sandbox->capabilities = sandbox_system.default_capabilities[SANDBOX_LEVEL_QUARANTINE];
    
    // Set strict resource limits
    sandboxing_set_resource_limit(module_id, RESOURCE_MEMORY, 256 * 1024); // 256KB
    sandboxing_set_resource_limit(module_id, RESOURCE_CHILD_ACTORS, 0);
    sandboxing_set_resource_limit(module_id, RESOURCE_HEAP_ALLOCS, 1);
    sandboxing_set_resource_limit(module_id, RESOURCE_MODULE_CALLS, 10);
    
    sandbox_system.quarantined_modules++;
    
    return 0;
}

/*
 * Enable/disable strict enforcement mode
 */
int sandboxing_enable_strict_mode(bool enabled)
{
    if (!sandboxing_initialized) return -1;
    
    sandbox_system.strict_enforcement = enabled;
    
    kprintf("[SANDBOX] Strict enforcement mode %s\n", enabled ? "ENABLED" : "DISABLED");
    return 0;
}

// =============================================================================
// Monitoring and Reporting
// =============================================================================

/*
 * Print sandboxing system status
 */
void sandboxing_print_status(void)
{
    if (!sandboxing_initialized) {
        kprintf("[SANDBOX] Sandboxing system not initialized\n");
        return;
    }
    
    kprintf("\n[SANDBOX] ========== SANDBOXING STATUS ==========\n");
    
    // System status
    kprintf("[SANDBOX] System Status:\n");
    kprintf("[SANDBOX]   Initialized: YES\n");
    kprintf("[SANDBOX]   Active sandboxes: %d/%d\n", 
            sandbox_system.sandbox_count, MAX_SANDBOXES);
    kprintf("[SANDBOX]   Strict enforcement: %s\n",
            sandbox_system.strict_enforcement ? "ENABLED" : "DISABLED");
    kprintf("[SANDBOX]   Security logging: %s\n",
            sandbox_system.logging_enabled ? "ENABLED" : "DISABLED");
    
    // Statistics
    kprintf("[SANDBOX] Statistics:\n");
    kprintf("[SANDBOX]   Total capability checks: %llu\n", 
            sandbox_system.total_capability_checks);
    kprintf("[SANDBOX]   Total violations: %llu\n", 
            sandbox_system.total_violations);
    kprintf("[SANDBOX]   Total enforcements: %llu\n", 
            sandbox_system.total_enforcements);
    kprintf("[SANDBOX]   Quarantined modules: %d\n", 
            sandbox_system.quarantined_modules);
    
    // Active sandboxes
    kprintf("[SANDBOX] Active Sandboxes:\n");
    for (int i = 0; i < MAX_SANDBOXES; i++) {
        sandbox_context_t* sandbox = &sandbox_system.sandboxes[i];
        if (sandbox->active) {
            kprintf("[SANDBOX]   Sandbox %d: Module %d, Level %d, Caps 0x%x\n",
                    sandbox->sandbox_id, sandbox->module_id, 
                    sandbox->security_level, sandbox->capabilities);
        }
    }
    
    kprintf("[SANDBOX] ==========================================\n\n");
}

/*
 * Print sandbox information
 */
void sandboxing_print_sandbox_info(uint32_t sandbox_id)
{
    if (!sandboxing_initialized) {
        kprintf("[SANDBOX] Sandboxing system not initialized\n");
        return;
    }
    
    sandbox_context_t* sandbox = NULL;
    for (int i = 0; i < MAX_SANDBOXES; i++) {
        if (sandbox_system.sandboxes[i].active && 
            sandbox_system.sandboxes[i].sandbox_id == sandbox_id) {
            sandbox = &sandbox_system.sandboxes[i];
            break;
        }
    }
    
    if (sandbox == NULL) {
        kprintf("[SANDBOX] Sandbox %d not found\n", sandbox_id);
        return;
    }
    
    kprintf("[SANDBOX] Sandbox %d Information:\n", sandbox_id);
    kprintf("[SANDBOX]   Module ID: %d\n", sandbox->module_id);
    kprintf("[SANDBOX]   Security level: %d\n", sandbox->security_level);
    kprintf("[SANDBOX]   Capabilities: 0x%x\n", sandbox->capabilities);
    kprintf("[SANDBOX]   Denied capabilities: 0x%x\n", sandbox->denied_capabilities);
    kprintf("[SANDBOX]   Function calls: %llu\n", sandbox->function_calls);
    kprintf("[SANDBOX]   Memory allocations: %llu\n", sandbox->memory_allocations);
    kprintf("[SANDBOX]   Capability checks: %llu\n", sandbox->capability_checks);
    kprintf("[SANDBOX]   Violations: %llu\n", sandbox->violations);
    kprintf("[SANDBOX]   VM enabled: %s\n", sandbox->vm_enabled ? "YES" : "NO");
    
    // Resource limits
    kprintf("[SANDBOX]   Resource limits:\n");
    for (int i = 0; i < sandbox->limit_count; i++) {
        resource_limit_t* limit = &sandbox->limits[i];
        kprintf("[SANDBOX]     Type %d: %d/%d (peak %d)\n",
                limit->resource_type, limit->current_usage, limit->limit, limit->peak_usage);
    }
}

/*
 * Print recent violations
 */
void sandboxing_print_violations(uint32_t count)
{
    if (!sandboxing_initialized) {
        kprintf("[SANDBOX] Sandboxing system not initialized\n");
        return;
    }
    
    if (count > sandbox_system.violation_count) {
        count = sandbox_system.violation_count;
    }
    
    kprintf("[SANDBOX] Recent violations (%d):\n", count);
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t index = (sandbox_system.violation_index - 1 - i + MAX_VIOLATION_LOG) % MAX_VIOLATION_LOG;
        security_violation_t* violation = &sandbox_system.violations[index];
        
        const char* types[] = {"CAPABILITY", "RESOURCE", "MEMORY", "EXECUTION", "POLICY"};
        const char* type_name = (violation->violation_type < 5) ? types[violation->violation_type] : "UNKNOWN";
        
        kprintf("[SANDBOX]   [%d] Module %d, %s: %s\n",
                violation->violation_id, violation->module_id, type_name, violation->description);
    }
}
