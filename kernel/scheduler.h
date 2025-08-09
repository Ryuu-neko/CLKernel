/*
 * =============================================================================
 * CLKernel - Async-First Scheduler
 * =============================================================================
 * File: scheduler.h  
 * Purpose: Actor-based cooperative scheduler for async-first kernel architecture
 *
 * This scheduler implements:
 * - Actor-based concurrency model (no traditional threads)
 * - Cooperative multitasking with async/await semantics
 * - Message-passing IPC between actors
 * - AI-supervised load balancing and deadlock detection
 * =============================================================================
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// =============================================================================
// Constants and Configuration
// =============================================================================

#define MAX_ACTORS              256     // Maximum number of concurrent actors
#define MAX_MESSAGES            1024    // Maximum messages in system
#define MAX_MESSAGE_SIZE        4096    // Maximum message payload size
#define ACTOR_STACK_SIZE        8192    // Default actor stack size
#define SCHEDULER_TIMESLICE_MS  10      // Time slice in milliseconds

// Actor states
#define ACTOR_STATE_CREATED     0       // Actor created but not started
#define ACTOR_STATE_READY       1       // Ready to run
#define ACTOR_STATE_RUNNING     2       // Currently executing
#define ACTOR_STATE_BLOCKED     3       // Waiting for message/resource
#define ACTOR_STATE_FINISHED    4       // Completed execution
#define ACTOR_STATE_ERROR       5       // Actor encountered error
#define ACTOR_STATE_SUSPENDED   6       // Suspended by system/user

// Actor priorities
#define ACTOR_PRIORITY_CRITICAL 0       // System-critical actors
#define ACTOR_PRIORITY_HIGH     1       // High priority actors
#define ACTOR_PRIORITY_NORMAL   2       // Normal priority actors
#define ACTOR_PRIORITY_LOW      3       // Background actors
#define ACTOR_PRIORITY_IDLE     4       // Idle time actors

// Message types
#define MSG_TYPE_ASYNC          0       // Fire-and-forget message
#define MSG_TYPE_SYNC_REQUEST   1       // Synchronous request (expects reply)
#define MSG_TYPE_SYNC_REPLY     2       // Synchronous reply
#define MSG_TYPE_BROADCAST      3       // Broadcast to multiple actors
#define MSG_TYPE_SYSTEM         4       // System management message

// =============================================================================
// Data Structures
// =============================================================================

/*
 * Actor execution context
 */
typedef struct actor_context {
    uint32_t        actor_id;           // Unique actor identifier
    uint32_t        parent_id;          // Parent actor ID (0 for kernel)
    
    // Actor state
    uint8_t         state;              // Current actor state
    uint8_t         priority;           // Actor priority level
    uint32_t        flags;              // Actor flags and attributes
    
    // Execution context
    void*           stack_base;         // Stack memory base
    void*           stack_current;      // Current stack pointer
    size_t          stack_size;         // Stack size in bytes
    void*           entry_point;        // Actor entry function
    void*           user_data;          // User data pointer
    
    // CPU context (for context switching)
    uint32_t        registers[8];       // General purpose registers
    uint32_t        eip;                // Instruction pointer
    uint32_t        esp;                // Stack pointer
    uint32_t        ebp;                // Base pointer
    uint32_t        eflags;             // Flags register
    
    // Message handling
    struct message* message_queue;      // Incoming message queue
    uint32_t        queue_size;         // Current queue size
    uint32_t        max_queue_size;     // Maximum queue size
    
    // Statistics and monitoring
    uint64_t        cpu_time_used;      // Total CPU time consumed
    uint64_t        messages_sent;      // Messages sent by this actor
    uint64_t        messages_received;  // Messages received by this actor
    uint64_t        creation_time;      // When actor was created
    uint64_t        last_scheduled;     // Last time actor was scheduled
    
    // Memory management
    void*           memory_context;     // Actor memory context
    size_t          memory_limit;       // Memory limit for this actor
    size_t          memory_used;        // Current memory usage
    
    // Error handling
    uint32_t        error_code;         // Last error code
    char*           error_message;      // Error description
    
    // AI supervision data
    uint32_t        behavior_score;     // AI-computed behavior score
    uint32_t        anomaly_count;      // Number of anomalies detected
    bool            ai_monitored;       // Whether AI is monitoring this actor
    
    // Linked list pointers
    struct actor_context* next;        // Next in ready queue
    struct actor_context* prev;        // Previous in ready queue
} actor_t;

/*
 * Inter-actor message
 */
typedef struct message {
    uint32_t        sender_id;          // Sender actor ID
    uint32_t        recipient_id;       // Recipient actor ID (0 = broadcast)
    uint32_t        message_id;         // Unique message ID
    
    uint8_t         type;               // Message type
    uint8_t         priority;           // Message priority
    uint16_t        flags;              // Message flags
    
    size_t          payload_size;       // Size of payload data
    void*           payload;            // Message payload
    
    uint64_t        timestamp;          // When message was created
    uint64_t        deadline;           // Message deadline (0 = no deadline)
    
    // For synchronous messages
    uint32_t        reply_to;           // Actor expecting reply
    bool            requires_reply;     // Whether reply is expected
    
    struct message* next;               // Next message in queue
} message_t;

/*
 * Scheduler statistics
 */
typedef struct scheduler_stats {
    uint64_t        context_switches;   // Total context switches
    uint64_t        actors_created;     // Total actors created
    uint64_t        actors_destroyed;   // Total actors destroyed
    uint64_t        messages_sent;      // Total messages sent
    uint64_t        messages_delivered; // Total messages delivered
    uint64_t        cpu_time_total;     // Total CPU time tracked
    uint32_t        current_actors;     // Currently active actors
    uint32_t        ready_actors;       // Actors in ready queue
    uint32_t        blocked_actors;     // Actors waiting for messages
    uint32_t        average_queue_depth;// Average message queue depth
    uint32_t        scheduler_overhead; // Scheduler overhead percentage
    uint32_t        deadlocks_detected; // AI-detected deadlocks
    uint32_t        load_balance_actions;// Load balancing actions taken
} scheduler_stats_t;

/*
 * Main scheduler context
 */
typedef struct scheduler_context {
    // Actor management
    actor_t*        actors[MAX_ACTORS]; // Actor table
    actor_t*        ready_queue;        // Ready to run actors
    actor_t*        current_actor;      // Currently running actor
    uint32_t        next_actor_id;      // Next available actor ID
    
    // Message system
    message_t*      free_messages;      // Pool of free message structures
    message_t*      message_pool;       // Message memory pool
    uint32_t        message_count;      // Current message count
    
    // Scheduling state
    bool            scheduler_enabled;  // Whether scheduler is running
    uint32_t        tick_count;         // Scheduler tick counter
    uint32_t        current_timeslice;  // Current time slice counter
    
    // Statistics and monitoring
    scheduler_stats_t statistics;       // Scheduler statistics
    bool            ai_supervision;     // AI supervision enabled
    
    // Performance tuning
    uint32_t        context_switch_time;// Average context switch time
    uint32_t        load_average[3];    // Load average (1, 5, 15 min)
    
} scheduler_t;

// =============================================================================
// Core Scheduler Functions
// =============================================================================

/*
 * Initialize the scheduler
 */
void scheduler_init(void);

/*
 * Start the scheduler (begin cooperative multitasking)
 */
void scheduler_start(void);

/*
 * Schedule the next actor to run
 */
void scheduler_schedule(void);

/*
 * Yield CPU to the next actor
 */
void scheduler_yield(void);

/*
 * Scheduler timer interrupt handler
 */
void scheduler_timer_handler(void);

// =============================================================================
// Actor Management Functions
// =============================================================================

/*
 * Create a new actor
 */
uint32_t actor_create(void* entry_point, void* user_data, 
                      uint8_t priority, size_t stack_size);

/*
 * Start an actor (move from CREATED to READY state)
 */
bool actor_start(uint32_t actor_id);

/*
 * Terminate an actor
 */
void actor_terminate(uint32_t actor_id);

/*
 * Suspend an actor
 */
bool actor_suspend(uint32_t actor_id);

/*
 * Resume a suspended actor
 */
bool actor_resume(uint32_t actor_id);

/*
 * Get actor by ID
 */
actor_t* actor_get(uint32_t actor_id);

/*
 * Get current running actor
 */
actor_t* actor_get_current(void);

// =============================================================================
// Message Passing Functions
// =============================================================================

/*
 * Send asynchronous message
 */
bool message_send_async(uint32_t recipient_id, uint8_t type, 
                       void* payload, size_t payload_size);

/*
 * Send synchronous message (blocks until reply)
 */
bool message_send_sync(uint32_t recipient_id, uint8_t type,
                      void* payload, size_t payload_size,
                      void* reply_buffer, size_t reply_buffer_size);

/*
 * Broadcast message to multiple actors
 */
bool message_broadcast(uint32_t* recipient_ids, uint32_t recipient_count,
                      uint8_t type, void* payload, size_t payload_size);

/*
 * Receive message (non-blocking)
 */
message_t* message_receive(void);

/*
 * Wait for message (blocking)
 */
message_t* message_wait(uint32_t timeout_ms);

/*
 * Reply to synchronous message
 */
bool message_reply(message_t* original_message, void* payload, size_t payload_size);

/*
 * Free message after processing
 */
void message_free(message_t* message);

// =============================================================================
// Async/Await Implementation
// =============================================================================

/*
 * Async function return type
 */
typedef struct async_result {
    bool            completed;          // Whether operation is complete
    void*           result;             // Result data
    uint32_t        error_code;         // Error code if failed
    struct async_result* next;          // For chaining operations
} async_result_t;

/*
 * Create an async operation
 */
async_result_t* async_create(void);

/*
 * Await an async operation (yields if not complete)
 */
void* await(async_result_t* operation);

/*
 * Complete an async operation
 */
void async_complete(async_result_t* operation, void* result);

/*
 * Fail an async operation
 */
void async_fail(async_result_t* operation, uint32_t error_code);

// =============================================================================
// Statistics and Monitoring
// =============================================================================

/*
 * Get scheduler statistics
 */
scheduler_stats_t* scheduler_get_statistics(void);

/*
 * Print scheduler status
 */
void scheduler_print_status(void);

/*
 * Print actor information
 */
void scheduler_print_actors(void);

/*
 * Get CPU usage for actor
 */
uint32_t actor_get_cpu_usage(uint32_t actor_id);

// =============================================================================
// AI Integration Functions  
// =============================================================================

/*
 * AI-based load balancing
 */
void scheduler_ai_balance_load(void);

/*
 * AI deadlock detection
 */
bool scheduler_ai_detect_deadlock(void);

/*
 * AI actor behavior analysis
 */
void scheduler_ai_analyze_actors(void);

/*
 * AI-optimized scheduling decisions
 */
uint32_t scheduler_ai_select_next_actor(void);

// =============================================================================
// Debug and Diagnostic Functions
// =============================================================================

/*
 * Dump scheduler state
 */
void scheduler_dump_state(void);

/*
 * Validate scheduler integrity
 */
bool scheduler_validate_state(void);

/*
 * Benchmark scheduler performance
 */
void scheduler_benchmark_performance(void);

// =============================================================================
// Inline Helper Functions
// =============================================================================

/*
 * Check if actor is valid
 */
static inline bool actor_is_valid(uint32_t actor_id)
{
    extern scheduler_t kernel_scheduler;
    return (actor_id < MAX_ACTORS && kernel_scheduler.actors[actor_id] != NULL);
}

/*
 * Get actor state name
 */
static inline const char* actor_state_name(uint8_t state)
{
    const char* states[] = {
        "CREATED", "READY", "RUNNING", "BLOCKED", 
        "FINISHED", "ERROR", "SUSPENDED"
    };
    return (state < 7) ? states[state] : "UNKNOWN";
}

/*
 * Get actor priority name
 */
static inline const char* actor_priority_name(uint8_t priority)
{
    const char* priorities[] = {
        "CRITICAL", "HIGH", "NORMAL", "LOW", "IDLE"
    };
    return (priority < 5) ? priorities[priority] : "UNKNOWN";
}

#endif // SCHEDULER_H
