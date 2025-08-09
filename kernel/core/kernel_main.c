/*
 * =============================================================================
 * CLKernel - Next-Generation Operating System Kernel
 * =============================================================================
 * File: kernel_main.c
 * Purpose: Main kernel entry point and core initialization
 * 
 * Architecture Overview:
 * - Hybrid kernel with microkernel modularity + monolithic performance
 * - Async-first design with actor-based IPC
 * - Hot-swappable modular subsystems
 * - AI-augmented fault detection and recovery
 * =============================================================================
 */

#include "kernel.h"
#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "memory.h"
#include "paging.h"
#include "heap.h"
#include "scheduler.h"
#include "modules.h"
#include "ai_supervisor.h"

// Kernel version and build info
#define KERNEL_VERSION_MAJOR 0
#define KERNEL_VERSION_MINOR 1
#define KERNEL_VERSION_PATCH 0
#define KERNEL_BUILD_DATE __DATE__
#define KERNEL_BUILD_TIME __TIME__

// Global kernel state
kernel_state_t kernel_state;

/*
 * Kernel Entry Point
 * Called from bootloader after switching to protected mode
 */
void kernel_main(void)
{
    // Initialize kernel state
    kernel_state.boot_time = 0; // TODO: Get from RTC
    kernel_state.uptime = 0;
    kernel_state.status = KERNEL_BOOTING;
    
    // Clear screen and display kernel banner
    vga_clear_screen();
    display_kernel_banner();
    
    // Initialize core kernel subsystems
    kprintf("[BOOT] Initializing CLKernel v%d.%d.%d\n", 
            KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_VERSION_PATCH);
    
    // Step 1: Setup GDT (Global Descriptor Table)
    kprintf("[BOOT] Setting up GDT... ");
    gdt_init();
    kprintf("OK\n");
    
    // Step 2: Setup IDT (Interrupt Descriptor Table)
    kprintf("[BOOT] Setting up IDT... ");
    idt_init();
    kprintf("OK\n");
    
    // Step 3: Initialize memory management
    kprintf("[BOOT] Initializing memory management... ");
    memory_init();
    paging_init();
    heap_init();
    kprintf("OK\n");
    
    // Step 4: Initialize async scheduler (core of our async-first design)
    kprintf("[BOOT] Initializing async scheduler... ");
    scheduler_init();
    kprintf("OK\n");
    
    // Step 5: Initialize module system (for hot-swappable components)
    kprintf("[BOOT] Initializing module system... ");
    modules_init();
    kprintf("OK\n");
    
    // Step 6: Load core modules
    kprintf("[BOOT] Loading core modules...\n");
    load_core_modules();
    
    // Step 7: Initialize AI supervisor (stubbed for now)
    kprintf("[BOOT] Initializing AI supervisor... ");
    ai_supervisor_init();
    kprintf("OK\n");
    
    // Mark kernel as ready
    kernel_state.status = KERNEL_READY;
    kprintf("\n[BOOT] CLKernel initialization complete!\n");
    kprintf("[BOOT] Kernel is running in hybrid mode with async actors\n");
    kprintf("[BOOT] AI supervisor is monitoring system health\n\n");
    
    // Start the main kernel loop (async event processing)
    kernel_main_loop();
}

/*
 * Display kernel banner with system information
 */
void display_kernel_banner(void)
{
    vga_set_color(VGA_COLOR_LIGHT_CYAN);
    kprintf("================================================================================\n");
    kprintf("  _____ _      _  __                      _ \n");
    kprintf(" / ____| |    | |/ /                     | |\n");
    kprintf("| |    | |    | ' / ___ _ __ _ __   ___  | |\n");
    kprintf("| |    | |    |  < / _ \\ '__| '_ \\ / _ \\ | |\n");
    kprintf("| |____| |____| . \\  __/ |  | | | |  __/ | |\n");
    kprintf(" \\_____|______|_|\\_\\___|_|  |_| |_|\\___| |_|\n");
    kprintf("\n");
    kprintf("CLKernel v%d.%d.%d - Next-Generation Operating System\n", 
            KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_VERSION_PATCH);
    kprintf("Built: %s %s\n", KERNEL_BUILD_DATE, KERNEL_BUILD_TIME);
    kprintf("Architecture: Hybrid Kernel with Async Actors\n");
    kprintf("Target: x86_64 (with future ARM64 support)\n");
    vga_set_color(VGA_COLOR_WHITE);
    kprintf("================================================================================\n\n");
}

/*
 * Load core kernel modules
 * These provide the foundational services for the hybrid kernel
 */
void load_core_modules(void)
{
    // VFS (Virtual File System) module
    kprintf("  -> Loading VFS module... ");
    if (load_module("vfs")) {
        kprintf("OK\n");
    } else {
        kprintf("FAILED\n");
    }
    
    // Device manager module
    kprintf("  -> Loading device manager... ");
    if (load_module("devmgr")) {
        kprintf("OK\n");
    } else {
        kprintf("FAILED\n");
    }
    
    // Network stack module (async-ready)
    kprintf("  -> Loading network stack... ");
    if (load_module("netstack")) {
        kprintf("OK\n");
    } else {
        kprintf("FAILED\n");
    }
    
    // Actor IPC system
    kprintf("  -> Loading actor IPC system... ");
    if (load_module("actor_ipc")) {
        kprintf("OK\n");
    } else {
        kprintf("FAILED\n");
    }
}

/*
 * Main kernel event loop
 * This is where the async magic happens - all kernel operations
 * are processed as async events through the actor system
 */
void kernel_main_loop(void)
{
    kprintf("[KERNEL] Entering main event loop...\n");
    kprintf("[KERNEL] Ready for async actor messages\n");
    
    // Enable interrupts - we're ready for business
    asm volatile("sti");
    
    uint32_t loop_counter = 0;
    
    while (1) {
        // Process pending async tasks
        scheduler_process_pending();
        
        // Handle hardware interrupts
        handle_pending_interrupts();
        
        // AI supervisor periodic check
        if (loop_counter % 10000 == 0) {
            ai_supervisor_check();
        }
        
        // Module system periodic maintenance
        if (loop_counter % 5000 == 0) {
            modules_periodic_check();
        }
        
        // AI Supervisor analysis
        if (loop_counter % 1000 == 0) {
            ai_supervisor_analyze();
        }
        
        // Simple heartbeat (temporary - will be replaced with proper logging)
        if (loop_counter % 100000 == 0) {
            kprintf("[HEARTBEAT] Kernel alive - uptime: %d seconds\n", 
                    kernel_state.uptime);
            
            // Show interrupt statistics every 10 heartbeats
            if (loop_counter % 1000000 == 0) {
                idt_print_stats();
            }
        }
        
        loop_counter++;
        
        // Yield CPU (power management)
        cpu_yield();
    }
}

/*
 * Kernel panic handler
 * Enhanced with AI-assisted recovery attempts
 */
void kernel_panic(const char* message, const char* file, int line)
{
    // Disable interrupts immediately
    asm volatile("cli");
    
    // Set panic mode
    kernel_state.status = KERNEL_PANIC;
    
    // Display panic information
    vga_set_color(VGA_COLOR_RED);
    kprintf("\n\n*** KERNEL PANIC ***\n");
    kprintf("Message: %s\n", message);
    kprintf("File: %s, Line: %d\n", file, line);
    kprintf("Uptime: %d seconds\n", kernel_state.uptime);
    
    // Attempt AI-assisted recovery (if available)
    vga_set_color(VGA_COLOR_YELLOW);
    kprintf("\n[AI] Attempting intelligent recovery...\n");
    
    if (ai_supervisor_attempt_recovery(message, file, line)) {
        vga_set_color(VGA_COLOR_GREEN);
        kprintf("[AI] Recovery successful! Resuming normal operation.\n");
        kernel_state.status = KERNEL_READY;
        asm volatile("sti");
        return;
    }
    
    // Recovery failed - halt system
    vga_set_color(VGA_COLOR_RED);
    kprintf("[AI] Recovery failed. System halted.\n");
    kprintf("\nPress Ctrl+Alt+Del to restart\n");
    
    while (1) {
        asm volatile("hlt");
    }
}

/*
 * CPU yield function for power management
 */
void cpu_yield(void)
{
    // Use HLT instruction to save power when idle
    asm volatile("hlt");
}
