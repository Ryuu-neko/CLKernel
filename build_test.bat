@echo off
rem Test build with GCC direct linking approach

echo Building test kernel for CLKernel Revolution...

rem Create build directory
if not exist build mkdir build

rem Build bootloader
echo [1/4] Building bootloader...
nasm -f bin boot\boot.asm -o build\bootloader.bin
if errorlevel 1 goto error

rem Build kernel entry 
echo [2/4] Building kernel entry...
nasm -f elf32 kernel\core\kernel_entry_simple.asm -o build\kernel_entry.o
if errorlevel 1 goto error

rem Compile test kernel
echo [3/4] Compiling test kernel...
gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c ^
    -m32 -march=i686 ^
    -fno-builtin -fno-stack-protector ^
    -nostdlib -nostartfiles -nodefaultlibs ^
    kernel\core\kernel_test.c -o build\kernel_test.o
if errorlevel 1 goto error

rem Link kernel using GCC 
echo [4/4] Linking kernel...
gcc -m32 -ffreestanding -O2 -nostdlib ^
    -Wl,-Ttext=0x100000 ^
    -o build\kernel.bin ^
    build\kernel_entry.o build\kernel_test.o
if errorlevel 1 goto error

rem Create bootable image
echo Creating bootable disk image...
copy /b build\bootloader.bin + build\kernel.bin build\clkernel.img >nul

echo ===================================
echo SUCCESS! Test kernel ready!
echo ===================================
echo File: build\clkernel.img
echo Test: qemu-system-i386 -drive format=raw,file=build\clkernel.img
echo ===================================

goto end

:error
echo ===================================  
echo BUILD FAILED!
echo ===================================
pause

:end
