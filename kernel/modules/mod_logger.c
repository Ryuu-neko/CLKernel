/*
 * =============================================================================
 * CLKernel - Logger Module (mod_logger.ko)
 * =============================================================================
 * File: mod_logger.c
 * Purpose: Hot-swappable logging module for actor activity monitoring
 *
 * This module provides:
 * - Actor activity logging with AI-enhanced pattern detection
 * - System event logging with priority levels
 * - Log rotation and management
 * - Real-time log analysis for anomaly detection
 * =============================================================================
 */

#include "../modules.h"
#include "../scheduler.h"
#include "../kernel.h"
#include "../vga.h"

// Module metadata
MODULE_DEFINE("mod_logger", 1, MODULE_TYPE_MISC, MODULE_FLAG_HOT_SWAP | MODULE_FLAG_AI_MONITOR);

// =============================================================================
// Logger Constants
// =============================================================================

#define MAX_LOG_ENTRIES         1000    // Maximum log entries to store
#define MAX_LOG_MESSAGE_SIZE    256     // Maximum log message size
#define LOG_ROTATION_SIZE       800     // Rotate when 80% full

// Log levels
#define LOG_LEVEL_DEBUG         0
#define LOG_LEVEL_INFO          1
#define LOG_LEVEL_WARNING       2
#define LOG_LEVEL_ERROR         3
#define LOG_LEVEL_CRITICAL      4

// Log categories
#define LOG_CAT_KERNEL          0x01
#define LOG_CAT_ACTOR           0x02
#define LOG_CAT_MODULE          0x04
#define LOG_CAT_MEMORY          0x08
#define LOG_CAT_NETWORK         0x10
#define LOG_CAT_FILESYSTEM      0x20
#define LOG_CAT_SECURITY        0x40
#define LOG_CAT_AI              0x80

// =============================================================================
// Logger Data Structures
// =============================================================================

typedef struct log_entry {
    uint32_t    entry_id;               // Unique entry ID
    uint64_t    timestamp;              // When log was created
    uint8_t     level;                  // Log level
    uint8_t     category;               // Log category
    uint32_t    actor_id;               // Associated actor (0 = kernel)
    uint32_t    module_id;              // Associated module (0 = kernel)
    char        message[MAX_LOG_MESSAGE_SIZE]; // Log message
    
    // AI analysis data
    uint32_t    pattern_score;          // AI-computed pattern score
    bool        anomaly_detected;       // Whether anomaly was detected
    uint32_t    correlation_id;         // Correlated events
    
} log_entry_t;

typedef struct logger_stats {
    uint32_t    total_entries;          // Total log entries created
    uint32_t    current_entries;        // Current entries in buffer
    uint32_t    rotations;              // Number of log rotations
    uint32_t    entries_by_level[5];    // Entries per level
    uint32_t    entries_by_category[8]; // Entries per category
    uint32_t    anomalies_detected;     // AI-detected anomalies
    uint32_t    pattern_matches;        // Pattern matches found
    uint64_t    last_rotation;          // Last rotation timestamp
    
} logger_stats_t;

typedef struct logger_module_state {
    log_entry_t entries[MAX_LOG_ENTRIES]; // Log entry buffer
    uint32_t    entry_count;            // Current number of entries
    uint32_t    next_entry_id;          // Next entry ID
    uint32_t    write_index;            // Write pointer (circular buffer)
    
    // Configuration
    uint8_t     min_log_level;          // Minimum log level to record
    uint8_t     enabled_categories;     // Enabled log categories
    bool        ai_analysis_enabled;    // AI analysis enabled
    bool        real_time_display;      // Real-time log display
    
    // Statistics
    logger_stats_t statistics;          // Logger statistics
    
    // AI Pattern Detection
    uint32_t    recent_patterns[10];    // Recent pattern IDs
    uint32_t    pattern_count;          // Number of patterns detected
    
} logger_module_state_t;

static logger_module_state_t logger_state;
static bool logger_module_active = false;

// =============================================================================
// Module Interface Functions
// =============================================================================

/*
 * Module initialization
 */
int module_init(void)
{
    kprintf("[LOGGER-MODULE] Initializing logger module v1.0...\n");
    
    // Initialize logger state
    logger_state.entry_count = 0;
    logger_state.next_entry_id = 1;
    logger_state.write_index = 0;
    
    // Configuration defaults
    logger_state.min_log_level = LOG_LEVEL_INFO;
    logger_state.enabled_categories = 0xFF; // All categories enabled
    logger_state.ai_analysis_enabled = true;
    logger_state.real_time_display = false; // Don't spam console initially
    
    // Clear statistics
    logger_state.statistics.total_entries = 0;
    logger_state.statistics.current_entries = 0;
    logger_state.statistics.rotations = 0;
    logger_state.statistics.anomalies_detected = 0;
    logger_state.statistics.pattern_matches = 0;
    logger_state.statistics.last_rotation = 0;
    
    for (int i = 0; i < 5; i++) {
        logger_state.statistics.entries_by_level[i] = 0;
    }
    for (int i = 0; i < 8; i++) {
        logger_state.statistics.entries_by_category[i] = 0;
    }
    
    // Clear pattern detection
    logger_state.pattern_count = 0;
    for (int i = 0; i < 10; i++) {
        logger_state.recent_patterns[i] = 0;
    }
    
    logger_module_active = true;
    
    // Log the module initialization
    logger_log(LOG_LEVEL_INFO, LOG_CAT_MODULE, 0, 0, "Logger module initialized successfully");
    
    kprintf("[LOGGER-MODULE] Logger module initialized\n");
    kprintf("[LOGGER-MODULE] Buffer size: %d entries\n", MAX_LOG_ENTRIES);
    kprintf("[LOGGER-MODULE] Min log level: %d\n", logger_state.min_log_level);
    kprintf("[LOGGER-MODULE] AI analysis: %s\n",
            logger_state.ai_analysis_enabled ? "ENABLED" : "DISABLED");
    
    return 0; // Success
}

/*
 * Module cleanup
 */
void module_exit(void)
{
    if (!logger_module_active) return;
    
    kprintf("[LOGGER-MODULE] Shutting down logger module...\n");
    
    // Log shutdown event
    logger_log(LOG_LEVEL_INFO, LOG_CAT_MODULE, 0, 0, "Logger module shutting down");
    
    // Print final statistics
    kprintf("[LOGGER-MODULE] Final statistics:\n");
    kprintf("[LOGGER-MODULE]   Total entries: %d\n", logger_state.statistics.total_entries);
    kprintf("[LOGGER-MODULE]   Current entries: %d\n", logger_state.statistics.current_entries);
    kprintf("[LOGGER-MODULE]   Log rotations: %d\n", logger_state.statistics.rotations);
    kprintf("[LOGGER-MODULE]   Anomalies detected: %d\n", logger_state.statistics.anomalies_detected);
    
    logger_module_active = false;
    
    kprintf("[LOGGER-MODULE] Logger module stopped\n");
}

/*
 * Module control interface
 */
int module_ioctl(uint32_t command, void* argument)
{
    if (!logger_module_active) {
        return -1; // Module not active
    }
    
    switch (command) {
        case 0: // Set minimum log level
            if (argument) {
                uint8_t level = *(uint8_t*)argument;
                if (level <= LOG_LEVEL_CRITICAL) {
                    logger_state.min_log_level = level;
                    kprintf("[LOGGER-MODULE] Min log level set to %d\n", level);
                    return 0;
                }
            }
            break;
            
        case 1: // Enable/disable categories
            if (argument) {
                logger_state.enabled_categories = *(uint8_t*)argument;
                kprintf("[LOGGER-MODULE] Enabled categories: 0x%x\n", 
                        logger_state.enabled_categories);
                return 0;
            }
            break;
            
        case 2: // Enable/disable AI analysis
            if (argument) {
                logger_state.ai_analysis_enabled = *(bool*)argument;
                kprintf("[LOGGER-MODULE] AI analysis %s\n",
                        logger_state.ai_analysis_enabled ? "ENABLED" : "DISABLED");
                return 0;
            }
            break;
            
        case 3: // Enable/disable real-time display
            if (argument) {
                logger_state.real_time_display = *(bool*)argument;
                kprintf("[LOGGER-MODULE] Real-time display %s\n",
                        logger_state.real_time_display ? "ENABLED" : "DISABLED");
                return 0;
            }
            break;
            
        case 4: // Get statistics
            if (argument) {
                *(logger_stats_t*)argument = logger_state.statistics;
                return 0;
            }
            break;
            
        case 5: // Force log rotation
            logger_rotate_logs();
            return 0;
            
        case 6: // Dump recent logs
            logger_dump_recent_logs(*(uint32_t*)argument);
            return 0;
            
        default:
            return -2; // Unknown command
    }
    
    return -3; // Invalid argument
}

// =============================================================================
// Core Logging Functions
// =============================================================================

/*
 * Log a message
 */
void logger_log(uint8_t level, uint8_t category, uint32_t actor_id, 
               uint32_t module_id, const char* message)
{
    if (!logger_module_active) return;
    
    // Check if we should log this level/category
    if (level < logger_state.min_log_level) return;
    if (!(category & logger_state.enabled_categories)) return;
    
    // Check if we need to rotate logs
    if (logger_state.entry_count >= LOG_ROTATION_SIZE) {
        logger_rotate_logs();
    }
    
    // Get log entry slot
    log_entry_t* entry = &logger_state.entries[logger_state.write_index];
    
    // Fill entry data
    entry->entry_id = logger_state.next_entry_id++;
    entry->timestamp = 0; // TODO: Get real timestamp
    entry->level = level;
    entry->category = category;
    entry->actor_id = actor_id;
    entry->module_id = module_id;
    
    // Copy message
    uint32_t msg_len = 0;
    while (msg_len < MAX_LOG_MESSAGE_SIZE - 1 && message[msg_len] != '\0') {
        entry->message[msg_len] = message[msg_len];
        msg_len++;
    }
    entry->message[msg_len] = '\0';
    
    // AI analysis
    entry->pattern_score = 0;
    entry->anomaly_detected = false;
    entry->correlation_id = 0;
    
    if (logger_state.ai_analysis_enabled) {
        logger_ai_analyze_entry(entry);
    }
    
    // Update counters
    logger_state.write_index = (logger_state.write_index + 1) % MAX_LOG_ENTRIES;
    if (logger_state.entry_count < MAX_LOG_ENTRIES) {
        logger_state.entry_count++;
    }
    
    // Update statistics
    logger_state.statistics.total_entries++;
    logger_state.statistics.current_entries = logger_state.entry_count;
    if (level < 5) {
        logger_state.statistics.entries_by_level[level]++;
    }
    
    // Category statistics (find which bit is set)
    for (int i = 0; i < 8; i++) {
        if (category & (1 << i)) {
            logger_state.statistics.entries_by_category[i]++;
            break;
        }
    }
    
    // Real-time display
    if (logger_state.real_time_display) {
        logger_display_entry(entry);
    }
}

/*
 * Log actor activity
 */
void logger_log_actor_activity(uint32_t actor_id, const char* activity)
{
    if (!logger_module_active) return;
    
    char message[MAX_LOG_MESSAGE_SIZE];
    
    // Simple string formatting
    const char* prefix = "Actor activity: ";
    uint32_t i = 0;
    
    // Copy prefix
    while (prefix[i] != '\0' && i < MAX_LOG_MESSAGE_SIZE - 50) {
        message[i] = prefix[i];
        i++;
    }
    
    // Copy activity
    uint32_t j = 0;
    while (activity[j] != '\0' && i < MAX_LOG_MESSAGE_SIZE - 1) {
        message[i++] = activity[j++];
    }
    message[i] = '\0';
    
    logger_log(LOG_LEVEL_DEBUG, LOG_CAT_ACTOR, actor_id, 0, message);
}

/*
 * Log module event
 */
void logger_log_module_event(uint32_t module_id, const char* event)
{
    if (!logger_module_active) return;
    
    char message[MAX_LOG_MESSAGE_SIZE];
    
    // Simple string formatting
    const char* prefix = "Module event: ";
    uint32_t i = 0;
    
    // Copy prefix
    while (prefix[i] != '\0' && i < MAX_LOG_MESSAGE_SIZE - 50) {
        message[i] = prefix[i];
        i++;
    }
    
    // Copy event
    uint32_t j = 0;
    while (event[j] != '\0' && i < MAX_LOG_MESSAGE_SIZE - 1) {
        message[i++] = event[j++];
    }
    message[i] = '\0';
    
    logger_log(LOG_LEVEL_INFO, LOG_CAT_MODULE, 0, module_id, message);
}

/*
 * Log system error
 */
void logger_log_error(const char* subsystem, const char* error_message)
{
    if (!logger_module_active) return;
    
    char message[MAX_LOG_MESSAGE_SIZE];
    
    // Format: "ERROR in <subsystem>: <error_message>"
    uint32_t i = 0;
    const char* prefix = "ERROR in ";
    
    // Copy prefix
    while (prefix[i] != '\0' && i < MAX_LOG_MESSAGE_SIZE - 100) {
        message[i] = prefix[i];
        i++;
    }
    
    // Copy subsystem
    uint32_t j = 0;
    while (subsystem[j] != '\0' && i < MAX_LOG_MESSAGE_SIZE - 50) {
        message[i++] = subsystem[j++];
    }
    
    // Add separator
    message[i++] = ':';
    message[i++] = ' ';
    
    // Copy error message
    j = 0;
    while (error_message[j] != '\0' && i < MAX_LOG_MESSAGE_SIZE - 1) {
        message[i++] = error_message[j++];
    }
    message[i] = '\0';
    
    logger_log(LOG_LEVEL_ERROR, LOG_CAT_KERNEL, 0, 0, message);
}

// =============================================================================
// Log Management Functions
// =============================================================================

/*
 * Rotate logs when buffer is full
 */
void logger_rotate_logs(void)
{
    if (!logger_module_active) return;
    
    kprintf("[LOGGER-MODULE] Rotating logs (buffer %d%% full)\n",
            (logger_state.entry_count * 100) / MAX_LOG_ENTRIES);
    
    // Simple rotation: just reset to beginning
    // In a real implementation, we'd save old logs to storage
    logger_state.write_index = 0;
    logger_state.entry_count = 0;
    logger_state.statistics.rotations++;
    logger_state.statistics.last_rotation = 0; // TODO: Get timestamp
    
    // Log the rotation event
    logger_log(LOG_LEVEL_INFO, LOG_CAT_KERNEL, 0, 0, "Log rotation completed");
}

/*
 * Display log entry
 */
void logger_display_entry(log_entry_t* entry)
{
    if (!entry) return;
    
    const char* levels[] = {"DEBUG", "INFO", "WARN", "ERROR", "CRIT"};
    const char* level_name = (entry->level < 5) ? levels[entry->level] : "UNK";
    
    kprintf("[%s] ", level_name);
    
    if (entry->actor_id != 0) {
        kprintf("Actor%d: ", entry->actor_id);
    } else if (entry->module_id != 0) {
        kprintf("Mod%d: ", entry->module_id);
    }
    
    kprintf("%s", entry->message);
    
    if (entry->anomaly_detected) {
        kprintf(" [ANOMALY]");
    }
    
    kprintf("\n");
}

/*
 * Dump recent log entries
 */
void logger_dump_recent_logs(uint32_t count)
{
    if (!logger_module_active) {
        kprintf("[LOGGER-MODULE] Logger module not active\n");
        return;
    }
    
    if (count > logger_state.entry_count) {
        count = logger_state.entry_count;
    }
    
    kprintf("[LOGGER-MODULE] Dumping %d recent log entries:\n", count);
    
    // Calculate starting index (going backwards from current position)
    uint32_t start_index;
    if (logger_state.write_index >= count) {
        start_index = logger_state.write_index - count;
    } else {
        start_index = MAX_LOG_ENTRIES - (count - logger_state.write_index);
    }
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t index = (start_index + i) % MAX_LOG_ENTRIES;
        log_entry_t* entry = &logger_state.entries[index];
        
        kprintf("  [%d] ", entry->entry_id);
        logger_display_entry(entry);
    }
}

// =============================================================================
// AI Analysis Functions
// =============================================================================

/*
 * AI analysis of log entry
 */
void logger_ai_analyze_entry(log_entry_t* entry)
{
    if (!entry || !logger_state.ai_analysis_enabled) return;
    
    // Simple pattern detection based on keywords
    uint32_t pattern_score = 0;
    bool anomaly = false;
    
    // Check for error keywords
    const char* error_keywords[] = {"error", "fail", "crash", "panic", "corrupt"};
    for (int i = 0; i < 5; i++) {
        if (logger_contains_keyword(entry->message, error_keywords[i])) {
            pattern_score += 20;
            if (entry->level >= LOG_LEVEL_ERROR) {
                anomaly = true;
            }
        }
    }
    
    // Check for suspicious patterns
    const char* suspicious_keywords[] = {"suspicious", "anomaly", "leak", "spike"};
    for (int i = 0; i < 4; i++) {
        if (logger_contains_keyword(entry->message, suspicious_keywords[i])) {
            pattern_score += 30;
            anomaly = true;
        }
    }
    
    // High frequency of logs from same actor might indicate problems
    if (entry->actor_id != 0) {
        uint32_t actor_log_count = logger_count_recent_actor_logs(entry->actor_id);
        if (actor_log_count > 10) { // More than 10 logs recently
            pattern_score += 25;
            if (actor_log_count > 20) {
                anomaly = true;
            }
        }
    }
    
    entry->pattern_score = (pattern_score > 100) ? 100 : pattern_score;
    entry->anomaly_detected = anomaly;
    
    if (anomaly) {
        logger_state.statistics.anomalies_detected++;
        kprintf("[LOGGER-MODULE] ANOMALY detected in log entry %d (score: %d)\n",
                entry->entry_id, entry->pattern_score);
    }
    
    if (pattern_score > 50) {
        logger_state.statistics.pattern_matches++;
        
        // Track recent patterns
        logger_state.recent_patterns[logger_state.pattern_count % 10] = entry->entry_id;
        logger_state.pattern_count++;
    }
}

/*
 * Check if message contains keyword
 */
bool logger_contains_keyword(const char* message, const char* keyword)
{
    if (!message || !keyword) return false;
    
    uint32_t msg_len = 0, key_len = 0;
    
    // Get lengths
    while (message[msg_len] != '\0') msg_len++;
    while (keyword[key_len] != '\0') key_len++;
    
    if (key_len > msg_len) return false;
    
    // Simple substring search (case-insensitive)
    for (uint32_t i = 0; i <= msg_len - key_len; i++) {
        bool match = true;
        for (uint32_t j = 0; j < key_len; j++) {
            char msg_char = message[i + j];
            char key_char = keyword[j];
            
            // Convert to lowercase for comparison
            if (msg_char >= 'A' && msg_char <= 'Z') {
                msg_char += 32;
            }
            if (key_char >= 'A' && key_char <= 'Z') {
                key_char += 32;
            }
            
            if (msg_char != key_char) {
                match = false;
                break;
            }
        }
        
        if (match) return true;
    }
    
    return false;
}

/*
 * Count recent logs from specific actor
 */
uint32_t logger_count_recent_actor_logs(uint32_t actor_id)
{
    if (!logger_module_active) return 0;
    
    uint32_t count = 0;
    uint32_t entries_to_check = (logger_state.entry_count < 50) ? 
                               logger_state.entry_count : 50;
    
    for (uint32_t i = 0; i < entries_to_check; i++) {
        uint32_t index = (logger_state.write_index - 1 - i + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
        if (logger_state.entries[index].actor_id == actor_id) {
            count++;
        }
    }
    
    return count;
}

// =============================================================================
// Status and Diagnostic Functions
// =============================================================================

/*
 * Print logger status
 */
void logger_print_status(void)
{
    if (!logger_module_active) {
        kprintf("[LOGGER-MODULE] Logger module is not active\n");
        return;
    }
    
    kprintf("[LOGGER-MODULE] Logger Module Status:\n");
    kprintf("[LOGGER-MODULE]   Active: YES\n");
    kprintf("[LOGGER-MODULE]   Current entries: %d/%d (%d%%)\n",
            logger_state.entry_count, MAX_LOG_ENTRIES,
            (logger_state.entry_count * 100) / MAX_LOG_ENTRIES);
    kprintf("[LOGGER-MODULE]   Total entries: %d\n", logger_state.statistics.total_entries);
    kprintf("[LOGGER-MODULE]   Rotations: %d\n", logger_state.statistics.rotations);
    kprintf("[LOGGER-MODULE]   Min log level: %d\n", logger_state.min_log_level);
    kprintf("[LOGGER-MODULE]   Enabled categories: 0x%x\n", logger_state.enabled_categories);
    kprintf("[LOGGER-MODULE]   AI analysis: %s\n",
            logger_state.ai_analysis_enabled ? "ENABLED" : "DISABLED");
    kprintf("[LOGGER-MODULE]   Anomalies detected: %d\n", logger_state.statistics.anomalies_detected);
    kprintf("[LOGGER-MODULE]   Pattern matches: %d\n", logger_state.statistics.pattern_matches);
}

// =============================================================================
// Module Export Table
// =============================================================================

MODULE_EXPORT(logger_log);
MODULE_EXPORT(logger_log_actor_activity);
MODULE_EXPORT(logger_log_module_event);
MODULE_EXPORT(logger_log_error);
MODULE_EXPORT(logger_print_status);
MODULE_EXPORT(logger_dump_recent_logs);
