@echo off
REM =============================================================================
REM CLKernel Windows Build Script
REM ========================================================echo [LD] Linking kernel...
%GCC% -nostdlib -ffreestanding -O2 -m32 ^
    -T kernel_simple.ld ^
    -o build\kernel.elf ^
    build\kernel_entry.o build\kernel_main.o build\vga.o build\gdt.o build\idt.o build\pic.o build\interrupt.o build\io.o build\stubs.o================
REM Purpose: Build script for Windows systems without make
REM =============================================================================

echo ===================================================================
echo CLKernel Build System for Windows
echo ===================================================================

REM Check if build directory exists
if not exist "build" (
    echo [SETUP] Creating build directories...
    mkdir build
    mkdir build\iso
    mkdir build\iso\boot
    mkdir build\iso\boot\grub
)

REM Set compiler variables (adjust paths as needed)
set NASM=nasm
set GCC=gcc
set LD=ld
set OBJCOPY=objcopy

REM Check for required tools
echo [CHECK] Verifying build tools...
%NASM% -v >nul 2>&1
if errorlevel 1 (
    echo ERROR: NASM not found. Please install NASM assembler.
    echo Download from: https://www.nasm.us/
    pause
    exit /b 1
)

%GCC% --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: GCC not found. Please install MinGW or similar.
    echo Download from: https://www.mingw-w64.org/
    pause
    exit /b 1
)

echo [CHECK] Build tools verified!

REM Parse command line argument
if "%1"=="help" goto :help
if "%1"=="clean" goto :clean
if "%1"=="bootloader" goto :bootloader
if "%1"=="kernel" goto :kernel
if "%1"=="all" goto :all
if "%1"=="run" goto :run
if "%1"=="" goto :all

:all
echo [BUILD] Building bootloader...
call :bootloader
if errorlevel 1 goto :error

echo [BUILD] Building kernel...
call :kernel
if errorlevel 1 goto :error

echo [BUILD] Creating disk image...
call :create_image
if errorlevel 1 goto :error

echo ===================================================================
echo CLKernel build complete!
echo ===================================================================
echo Bootloader: build\bootloader.bin
echo Kernel:     build\kernel.bin  
echo Image:      build\clkernel.img
echo ===================================================================
echo Run 'build.bat run' to test in QEMU
echo ===================================================================
goto :end

:bootloader
echo [ASM] Building bootloader...
%NASM% -f bin boot\boot.asm -o build\bootloader.bin
if errorlevel 1 (
    echo ERROR: Bootloader build failed!
    exit /b 1
)
echo [ASM] Bootloader built successfully
goto :eof

:kernel
echo [ASM] Building kernel entry point...
%NASM% -f elf32 kernel\core\kernel_entry.asm -o build\kernel_entry.o
if errorlevel 1 (
    echo ERROR: Kernel entry build failed!
    exit /b 1
)

echo [CC] Compiling kernel sources...
%GCC% -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c ^
    -nostdlib -nostartfiles -nodefaultlibs ^
    -fno-builtin -fno-stack-protector ^
    -m32 -march=i686 ^
    -I kernel -I kernel\core ^
    -DKERNEL_BUILD -DCLKERNEL_VERSION=\"0.1.0\" ^
    kernel\core\kernel_main.c -o build\kernel_main.o
if errorlevel 1 (
    echo ERROR: kernel_main.c compilation failed!
    exit /b 1
)

%GCC% -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c ^
    -nostdlib -nostartfiles -nodefaultlibs ^
    -fno-builtin -fno-stack-protector ^
    -m32 -march=i686 ^
    -I kernel -I kernel\core ^
    -DKERNEL_BUILD -DCLKERNEL_VERSION=\"0.1.0\" ^
    kernel\core\vga.c -o build\vga.o
if errorlevel 1 (
    echo ERROR: vga.c compilation failed!
    exit /b 1
)

%GCC% -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c ^
    -nostdlib -nostartfiles -nodefaultlibs ^
    -fno-builtin -fno-stack-protector ^
    -m32 -march=i686 ^
    -I kernel -I kernel\core ^
    -DKERNEL_BUILD -DCLKERNEL_VERSION=\"0.1.0\" ^
    kernel\core\gdt.c -o build\gdt.o
if errorlevel 1 (
    echo ERROR: gdt.c compilation failed!
    exit /b 1
)

%GCC% -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c ^
    -nostdlib -nostartfiles -nodefaultlibs ^
    -fno-builtin -fno-stack-protector ^
    -m32 -march=i686 ^
    -I kernel -I kernel\core ^
    -DKERNEL_BUILD -DCLKERNEL_VERSION=\"0.1.0\" ^
    kernel\core\idt.c -o build\idt.o
if errorlevel 1 (
    echo ERROR: idt.c compilation failed!
    exit /b 1
)

%GCC% -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c ^
    -nostdlib -nostartfiles -nodefaultlibs ^
    -fno-builtin -fno-stack-protector ^
    -m32 -march=i686 ^
    -I kernel -I kernel\core ^
    -DKERNEL_BUILD -DCLKERNEL_VERSION=\"0.1.0\" ^
    kernel\core\pic.c -o build\pic.o
if errorlevel 1 (
    echo ERROR: pic.c compilation failed!
    exit /b 1
)

%GCC% -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c ^
    -nostdlib -nostartfiles -nodefaultlibs ^
    -fno-builtin -fno-stack-protector ^
    -m32 -march=i686 ^
    -I kernel -I kernel\core ^
    -DKERNEL_BUILD -DCLKERNEL_VERSION=\"0.1.0\" ^
    kernel\core\stubs.c -o build\stubs.o
if errorlevel 1 (
    echo ERROR: stubs.c compilation failed!
    exit /b 1
)

%GCC% -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c ^
    -nostdlib -nostartfiles -nodefaultlibs ^
    -fno-builtin -fno-stack-protector ^
    -m32 -march=i686 ^
    -I kernel -I kernel\core ^
    -DKERNEL_BUILD -DCLKERNEL_VERSION=\"0.1.0\" ^
    kernel\core\io.c -o build\io.o
if errorlevel 1 (
    echo ERROR: io.c compilation failed!
    exit /b 1
)

echo [ASM] Building interrupt handlers...
%NASM% -f elf32 kernel\core\interrupt.asm -o build\interrupt.o
if errorlevel 1 (
    echo ERROR: interrupt.asm assembly failed!
    exit /b 1
)

echo [LD] Linking kernel...
%GCC% -nostdlib -ffreestanding -O2 -m32 ^
    -T kernel.ld ^
    -o build\kernel.elf ^
    build\kernel_entry.o build\kernel_main.o build\vga.o build\gdt.o build\idt.o build\pic.o build\interrupt.o build\io.o build\stubs.o
if errorlevel 1 (
    echo ERROR: Kernel linking failed!
    exit /b 1
)

echo [OBJCOPY] Creating kernel binary...
%OBJCOPY% -O binary build\kernel.elf build\kernel.bin
if errorlevel 1 (
    echo ERROR: Binary creation failed!
    exit /b 1
)

echo [LD] Kernel built successfully
goto :eof

:create_image
echo [IMG] Creating disk image...
REM Create 2.88MB floppy image (5760 sectors of 512 bytes)
fsutil file createnew build\clkernel.img 2949120 >nul 2>&1
if errorlevel 1 (
    echo WARNING: Could not create disk image with fsutil
    echo You may need to manually create the image
    goto :eof
)

REM Note: On Windows, we need a tool like dd for proper disk image creation
REM For now, we'll create a simple concatenated file
copy /b build\bootloader.bin + build\kernel.bin build\clkernel.img >nul
echo [IMG] Basic image created (bootloader + kernel)
echo [IMG] Note: For proper disk image, install dd or similar tool
goto :eof

:run
if not exist "build\clkernel.img" (
    echo ERROR: clkernel.img not found. Run 'build.bat all' first.
    exit /b 1
)

echo [QEMU] Starting CLKernel in QEMU...
echo Press Ctrl+C to exit QEMU
qemu-system-i386 -drive file=build\clkernel.img,format=raw,index=0,if=floppy -m 32M
if errorlevel 1 (
    echo ERROR: QEMU failed to start. Is QEMU installed?
    echo Download from: https://www.qemu.org/download/#windows
)
goto :end

:clean
echo [CLEAN] Removing build files...
if exist "build" rmdir /s /q build
echo [CLEAN] Clean complete
goto :end

:help
echo CLKernel Build System Help
echo ==========================
echo Commands:
echo   all       - Build everything (default)
echo   bootloader- Build just the bootloader
echo   kernel    - Build just the kernel  
echo   run       - Run kernel in QEMU
echo   clean     - Remove all build files
echo   help      - Show this help
echo.
echo Prerequisites:
echo   - NASM assembler
echo   - GCC compiler (MinGW recommended)
echo   - QEMU emulator (for testing)
echo.
echo Development workflow:
echo   1. build.bat all      # Build everything
echo   2. build.bat run      # Test in QEMU
echo   3. build.bat clean    # Clean when needed
goto :end

:error
echo.
echo BUILD FAILED!
echo Check the error messages above.
pause
exit /b 1

:end
echo.
pause
