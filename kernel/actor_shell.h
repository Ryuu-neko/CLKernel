/*
 * =============================================================================
 * CLKernel - Actor Shell (Interactive Command Line)
 * =============================================================================
 * File: actor_shell.h
 * Purpose: VGA-based interactive command line interface running as an actor
 *
 * This system provides:
 * - Interactive command-line interface for kernel management
 * - Module management commands (load, unload, list, configure)
 * - System diagnostics and monitoring commands
 * - AI supervisor interaction commands
 * - Real-time system status display
 * =============================================================================
 */

#ifndef ACTOR_SHELL_H
#define ACTOR_SHELL_H

#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// Shell Constants
// =============================================================================

#define SHELL_MAX_COMMAND_LENGTH    256     // Maximum command length
#define SHELL_MAX_ARGS              16      // Maximum command arguments
#define SHELL_HISTORY_SIZE          50      // Command history size
#define SHELL_MAX_COMMANDS          64      // Maximum built-in commands
#define SHELL_PROMPT_LENGTH         32      // Maximum prompt length

// Shell colors
#define SHELL_COLOR_NORMAL          0x07    // Light gray on black
#define SHELL_COLOR_PROMPT          0x0F    // White on black
#define SHELL_COLOR_INPUT           0x0B    // Light cyan on black
#define SHELL_COLOR_OUTPUT          0x07    // Light gray on black
#define SHELL_COLOR_ERROR           0x0C    // Light red on black
#define SHELL_COLOR_SUCCESS         0x0A    // Light green on black
#define SHELL_COLOR_WARNING         0x0E    // Yellow on black
#define SHELL_COLOR_INFO            0x09    // Light blue on black

// Command result codes
#define SHELL_RESULT_SUCCESS        0       // Command executed successfully
#define SHELL_RESULT_ERROR          1       // Command execution error
#define SHELL_RESULT_UNKNOWN        2       // Unknown command
#define SHELL_RESULT_INVALID_ARGS   3       // Invalid arguments
#define SHELL_RESULT_EXIT           4       // Exit shell
#define SHELL_RESULT_HELP           5       // Help requested

// Shell modes
#define SHELL_MODE_INTERACTIVE      0       // Interactive mode
#define SHELL_MODE_BATCH            1       // Batch mode
#define SHELL_MODE_DEBUG            2       // Debug mode

// =============================================================================
// Shell Data Structures
// =============================================================================

typedef struct shell_command {
    char        name[32];               // Command name
    char        description[128];       // Command description
    char        usage[256];             // Usage string
    int         (*handler)(int argc, char* argv[]); // Command handler function
    uint8_t     min_args;               // Minimum arguments required
    uint8_t     max_args;               // Maximum arguments allowed
    bool        privileged;             // Requires elevated privileges
    
} shell_command_t;

typedef struct shell_history_entry {
    char        command[SHELL_MAX_COMMAND_LENGTH]; // Command text
    uint64_t    timestamp;              // When command was executed
    int         result;                 // Command result code
    
} shell_history_entry_t;

typedef struct shell_state {
    // Shell configuration
    bool        active;                 // Whether shell is active
    uint8_t     mode;                   // Shell mode
    bool        echo_enabled;           // Command echo enabled
    bool        colors_enabled;         // Color output enabled
    char        prompt[SHELL_PROMPT_LENGTH]; // Command prompt
    
    // Input state
    char        input_buffer[SHELL_MAX_COMMAND_LENGTH]; // Current input
    uint32_t    input_position;         // Current cursor position in input
    uint32_t    input_length;           // Length of current input
    bool        input_ready;            // Whether input is ready to process
    
    // Command processing
    char*       argv[SHELL_MAX_ARGS];   // Parsed command arguments
    int         argc;                   // Number of arguments
    
    // Command history
    shell_history_entry_t history[SHELL_HISTORY_SIZE]; // Command history
    uint32_t    history_count;          // Number of history entries
    uint32_t    history_index;          // Current history index
    int32_t     history_position;       // Position in history navigation
    
    // Built-in commands
    shell_command_t commands[SHELL_MAX_COMMANDS]; // Built-in commands
    uint32_t    command_count;          // Number of registered commands
    
    // Display state
    uint8_t     current_color;          // Current text color
    uint32_t    lines_printed;          // Lines printed in current command
    bool        more_mode;              // Whether in pager mode
    
    // Statistics
    uint64_t    commands_executed;      // Total commands executed
    uint64_t    characters_typed;       // Total characters typed
    uint32_t    errors_encountered;     // Errors encountered
    uint64_t    session_start_time;     // When shell session started
    
} shell_state_t;

// =============================================================================
// Function Declarations
// =============================================================================

// Shell lifecycle
int shell_init(void);
void shell_shutdown(void);
int shell_start_interactive(void);
void shell_stop(void);

// Input processing
void shell_process_input(void);
void shell_handle_key(uint8_t key);
void shell_handle_special_key(uint8_t scancode);
bool shell_parse_command(const char* command_line);
void shell_execute_command(void);

// Command management
int shell_register_command(const char* name, const char* description, const char* usage,
                          int (*handler)(int argc, char* argv[]), 
                          uint8_t min_args, uint8_t max_args, bool privileged);
shell_command_t* shell_find_command(const char* name);
void shell_list_commands(void);

// History management
void shell_add_to_history(const char* command, int result);
void shell_navigate_history(int direction);
void shell_print_history(uint32_t count);

// Display functions
void shell_print_prompt(void);
void shell_print_colored(const char* text, uint8_t color);
void shell_print_line(const char* text);
void shell_print_error(const char* message);
void shell_print_success(const char* message);
void shell_print_warning(const char* message);
void shell_print_info(const char* message);

// Utility functions
void shell_clear_screen(void);
void shell_set_prompt(const char* new_prompt);
void shell_enable_colors(bool enabled);
void shell_print_banner(void);

// Built-in command handlers
int shell_cmd_help(int argc, char* argv[]);
int shell_cmd_clear(int argc, char* argv[]);
int shell_cmd_exit(int argc, char* argv[]);
int shell_cmd_history(int argc, char* argv[]);
int shell_cmd_echo(int argc, char* argv[]);
int shell_cmd_prompt(int argc, char* argv[]);

// System command handlers
int shell_cmd_status(int argc, char* argv[]);
int shell_cmd_uptime(int argc, char* argv[]);
int shell_cmd_memory(int argc, char* argv[]);
int shell_cmd_actors(int argc, char* argv[]);
int shell_cmd_scheduler(int argc, char* argv[]);

// Module command handlers
int shell_cmd_modules(int argc, char* argv[]);
int shell_cmd_load_module(int argc, char* argv[]);
int shell_cmd_unload_module(int argc, char* argv[]);
int shell_cmd_module_info(int argc, char* argv[]);
int shell_cmd_hot_swap(int argc, char* argv[]);

// Diagnostic command handlers
int shell_cmd_diag(int argc, char* argv[]);
int shell_cmd_test(int argc, char* argv[]);
int shell_cmd_benchmark(int argc, char* argv[]);
int shell_cmd_logs(int argc, char* argv[]);

// AI supervisor command handlers
int shell_cmd_ai(int argc, char* argv[]);
int shell_cmd_ai_status(int argc, char* argv[]);
int shell_cmd_ai_analyze(int argc, char* argv[]);
int shell_cmd_ai_configure(int argc, char* argv[]);

// Sandboxing command handlers
int shell_cmd_sandbox(int argc, char* argv[]);
int shell_cmd_capabilities(int argc, char* argv[]);
int shell_cmd_quarantine(int argc, char* argv[]);

// Advanced command handlers
int shell_cmd_crash_test(int argc, char* argv[]);
int shell_cmd_performance(int argc, char* argv[]);
int shell_cmd_debug(int argc, char* argv[]);

// Internal functions
void shell_register_builtin_commands(void);
void shell_process_backspace(void);
void shell_process_enter(void);
void shell_update_display(void);
bool shell_is_whitespace(char c);
char* shell_trim_whitespace(char* str);

#endif // ACTOR_SHELL_H
