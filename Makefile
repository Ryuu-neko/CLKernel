# =============================================================================
# CLKernel Makefile - Next-Generation OS Kernel Build System
# =============================================================================
# Purpose: Build bootloader, kernel, modules, and create bootable ISO
# Target: x86_64 with GCC toolchain and NASM assembler
# =============================================================================

# Build configuration
TARGET = i686-elf
CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy
GRUB_MKRESCUE = grub-mkrescue
QEMU = qemu-system-i386

# Project directories
BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel
MODULES_DIR = kernel/modules
AI_DIR = kernel/ai
TOOLS_DIR = tools
ISO_DIR = $(BUILD_DIR)/iso

# Compiler flags
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Wpedantic \
         -nostdlib -nostartfiles -nodefaultlibs \
         -fno-builtin -fno-stack-protector \
         -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
         -m32 -march=i686 \
         -I$(KERNEL_DIR) -I$(KERNEL_DIR)/core \
         -DKERNEL_BUILD -DCLKERNEL_VERSION=\"0.1.0\"

# Assembler flags
ASFLAGS = -f bin

# Linker flags
LDFLAGS = -T kernel.ld -nostdlib -m elf_i386

# Source files
BOOT_ASM = $(BOOT_DIR)/boot.asm
KERNEL_ENTRY_ASM = $(KERNEL_DIR)/core/kernel_entry.asm
KERNEL_SOURCES = $(shell find $(KERNEL_DIR) -name "*.c" | grep -v modules | grep -v ai)
MODULE_SOURCES = $(shell find $(MODULES_DIR) -name "*.c" 2>/dev/null || echo "")
AI_SOURCES = $(shell find $(AI_DIR) -name "*.c" 2>/dev/null || echo "")

# Object files
KERNEL_OBJECTS = $(KERNEL_SOURCES:.c=.o)
MODULE_OBJECTS = $(MODULE_SOURCES:.c=.o)
AI_OBJECTS = $(AI_SOURCES:.c=.o)

# Output files
BOOTLOADER = $(BUILD_DIR)/bootloader.bin
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
KERNEL_IMG = $(BUILD_DIR)/clkernel.img
ISO_FILE = $(BUILD_DIR)/clkernel.iso

# Build targets
.PHONY: all clean bootloader kernel modules iso run run-headless debug help setup test size

# Default target
all: setup bootloader kernel iso
	@echo "==================================================================="
	@echo "CLKernel build complete!"
	@echo "==================================================================="
	@echo "Bootloader: $(BOOTLOADER)"
	@echo "Kernel:     $(KERNEL_BIN)"
	@echo "Image:      $(KERNEL_IMG)"
	@echo "ISO:        $(ISO_FILE)"
	@echo "==================================================================="
	@echo "Run 'make run' to test in QEMU"
	@echo "Run 'make debug' to launch with debugging"
	@echo "==================================================================="

# Setup build directories
setup:
	@echo "[SETUP] Creating build directories..."
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(ISO_DIR)
	@mkdir -p $(ISO_DIR)/boot/grub
	@echo "[SETUP] Build environment ready"

# Build bootloader
bootloader: $(BOOTLOADER)

$(BOOTLOADER): $(BOOT_ASM) | setup
	@echo "[ASM] Building bootloader..."
	$(AS) $(ASFLAGS) $(BOOT_ASM) -o $@
	@echo "[ASM] Bootloader built successfully ($$(wc -c < $@) bytes)"

# Build kernel entry point
$(BUILD_DIR)/kernel_entry.o: $(KERNEL_ENTRY_ASM) | setup
	@echo "[ASM] Building kernel entry point..."
	$(AS) -f elf32 $(KERNEL_ENTRY_ASM) -o $(BUILD_DIR)/kernel_entry.o

# Build kernel
kernel: $(KERNEL_BIN)

$(KERNEL_BIN): $(BUILD_DIR)/kernel_entry.o $(KERNEL_OBJECTS) | setup
	@echo "[LD] Linking kernel..."
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(BUILD_DIR)/kernel_entry.o $(KERNEL_OBJECTS)
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)
	@echo "[LD] Kernel built successfully ($(shell wc -c < $(KERNEL_BIN)) bytes)"

# Compile C source files
%.o: %.c
	@echo "[CC] Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Build modules (placeholder for now)
modules:
	@echo "[MODULES] Building kernel modules..."
	@if [ -n "$(MODULE_SOURCES)" ]; then \
		for module in $(MODULE_SOURCES); do \
			echo "[CC] Compiling module $$module"; \
			$(CC) $(CFLAGS) -DMODULE_BUILD -c $$module -o $${module%.c}.o; \
		done; \
	else \
		echo "[MODULES] No modules to build yet"; \
	fi

# Create disk image
$(KERNEL_IMG): $(BOOTLOADER) $(KERNEL_BIN)
	@echo "[IMG] Creating disk image..."
	@# Create 2.88MB floppy image
	@dd if=/dev/zero of=$(KERNEL_IMG) bs=512 count=5760 2>/dev/null
	@# Write bootloader to first sector
	@dd if=$(BOOTLOADER) of=$(KERNEL_IMG) bs=512 count=1 conv=notrunc 2>/dev/null
	@# Write kernel starting at sector 2
	@dd if=$(KERNEL_BIN) of=$(KERNEL_IMG) bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "[IMG] Disk image created successfully"

# Create ISO (future GRUB support)
iso: $(KERNEL_IMG)
	@echo "[ISO] Creating bootable ISO..."
	@# Create GRUB configuration
	@echo 'menuentry "CLKernel" {' > $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    multiboot /boot/kernel.bin' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	@# Copy kernel to ISO directory
	@cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.bin
	@# Create ISO (if grub-mkrescue is available)
	@if command -v $(GRUB_MKRESCUE) >/dev/null 2>&1; then \
		$(GRUB_MKRESCUE) -o $(ISO_FILE) $(ISO_DIR); \
		echo "[ISO] ISO created successfully"; \
	else \
		echo "[ISO] grub-mkrescue not found - using disk image instead"; \
		cp $(KERNEL_IMG) $(ISO_FILE); \
	fi

# Run in QEMU
run: $(KERNEL_IMG)
	@echo "[QEMU] Starting CLKernel in QEMU..."
	@echo "Press Ctrl+C to exit QEMU"
	$(QEMU) -drive file=$(KERNEL_IMG),format=raw,index=0,if=floppy \
		-m 32M \
		-display curses \
		-serial stdio

# Run with debugging support
debug: $(KERNEL_IMG)
	@echo "[QEMU] Starting CLKernel in QEMU with debugging..."
	@echo "GDB server will be available on localhost:1234"
	@echo "Press Ctrl+C to exit QEMU"
	$(QEMU) -drive file=$(KERNEL_IMG),format=raw,index=0,if=floppy \
		-m 32M \
		-s -S \
		-display curses \
		-serial stdio &
	@echo "Run 'gdb $(KERNEL_ELF)' and 'target remote :1234' to connect"

# Run headless (for CI/CD)
run-headless: $(KERNEL_IMG)
	@echo "[QEMU] Starting CLKernel in headless mode..."
	$(QEMU) -drive file=$(KERNEL_IMG),format=raw,index=0,if=floppy \
	        -m 32M \
	        -nographic \
	        -serial stdio

# Run tests (placeholder)
test:
	@echo "[TEST] No tests implemented yet"

# Development utilities
objdump: $(KERNEL_ELF)
	@echo "[DEBUG] Kernel disassembly:"
	objdump -d $(KERNEL_ELF) | less

hexdump: $(KERNEL_BIN)
	@echo "[DEBUG] Kernel binary hex dump:"
	hexdump -C $(KERNEL_BIN) | head -20

size: $(KERNEL_BIN) $(BOOTLOADER) $(KERNEL_IMG)
	@echo "[INFO] Build sizes:"
	@echo -n "Bootloader: "
	@wc -c < $(BOOTLOADER)
	@echo -n "Kernel:     "
	@wc -c < $(KERNEL_BIN)
	@echo -n "Image:      "
	@wc -c < $(KERNEL_IMG)

# Clean build files
clean:
	@echo "[CLEAN] Removing build files..."
	@rm -rf $(BUILD_DIR)
	@find . -name "*.o" -delete
	@echo "[CLEAN] Clean complete"

# Show help
help:
	@echo "CLKernel Build System Help"
	@echo "=========================="
	@echo "Targets:"
	@echo "  all       - Build everything (default)"
	@echo "  bootloader- Build just the bootloader"
	@echo "  kernel    - Build just the kernel"
	@echo "  modules   - Build kernel modules"
	@echo "  iso       - Create bootable ISO"
	@echo "  run       - Run kernel in QEMU"
	@echo "  debug     - Run kernel in QEMU with GDB support"
	@echo "  objdump   - Show kernel disassembly"
	@echo "  hexdump   - Show kernel binary hex dump"
	@echo "  size      - Show build sizes"
	@echo "  clean     - Remove all build files"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Development workflow:"
	@echo "  1. make all      # Build everything"
	@echo "  2. make run      # Test in QEMU"
	@echo "  3. make clean    # Clean when needed"

# Make sure intermediate files are kept
.PRECIOUS: %.o $(BUILD_DIR)/kernel_entry.o
