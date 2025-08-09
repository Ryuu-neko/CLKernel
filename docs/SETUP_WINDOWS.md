# Windows Development Setup Guide for CLKernel

This guide will help you set up a complete development environment for CLKernel on Windows.

## 🛠️ Required Tools

### 1. NASM Assembler
**Download**: https://www.nasm.us/pub/nasm/releasebuilds/2.16.01/win64/nasm-2.16.01-win64.zip

**Installation**:
1. Extract to `C:\nasm\`
2. Add `C:\nasm` to your PATH environment variable
3. Test: Open Command Prompt and run `nasm -v`

### 2. MinGW-w64 (GCC for Windows)
**Download**: https://www.mingw-w64.org/downloads/

**Recommended**: Use the installer from:
- https://github.com/niXman/mingw-builds-binaries/releases
- Download: `x86_64-13.2.0-release-posix-seh-msvcrt-rt_v11-rev1.7z`

**Installation**:
1. Extract to `C:\mingw64\`
2. Add `C:\mingw64\bin` to your PATH
3. Test: `gcc --version`

### 3. QEMU Emulator
**Download**: https://qemu.weilnetz.de/w64/

**Installation**:
1. Download and run the installer
2. Default installation path: `C:\Program Files\qemu\`
3. Add QEMU to PATH: `C:\Program Files\qemu`
4. Test: `qemu-system-i386 --version`

### 4. Optional: Git for Windows
**Download**: https://git-scm.com/download/win

## 🚀 Quick Setup Script

Create a file called `setup_tools.bat` and run as Administrator:

```batch
@echo off
echo Setting up CLKernel development environment...

REM Create tools directory
if not exist "C:\devtools" mkdir "C:\devtools"

echo.
echo Please download and extract the following tools:
echo 1. NASM to C:\devtools\nasm\
echo 2. MinGW-w64 to C:\devtools\mingw64\
echo 3. QEMU to C:\Program Files\qemu\
echo.
echo After extraction, this script will update your PATH.
pause

REM Add to PATH (requires admin privileges)
setx PATH "%PATH%;C:\devtools\nasm;C:\devtools\mingw64\bin;C:\Program Files\qemu" /M

echo.
echo Setup complete! Please restart your command prompt.
echo Test with: nasm -v && gcc --version && qemu-system-i386 --version
pause
```

## 🔧 Manual PATH Configuration

If the script doesn't work, manually add these to your system PATH:

1. Press `Win + X`, select "System"
2. Click "Advanced system settings"
3. Click "Environment Variables"
4. Under "System Variables", find and select "Path", then click "Edit"
5. Add these paths (adjust if you installed elsewhere):
   - `C:\devtools\nasm`
   - `C:\devtools\mingw64\bin`
   - `C:\Program Files\qemu`
6. Click OK and restart Command Prompt

## 🧪 Testing Your Setup

Open a new Command Prompt and run:

```cmd
REM Test NASM
nasm -v

REM Test GCC
gcc --version

REM Test QEMU
qemu-system-i386 --version

REM Test object tools
ld --version
objcopy --version
```

Expected output:
```
C:\> nasm -v
NASM version 2.16.01 compiled on Dec  7 2022

C:\> gcc --version
gcc.exe (x86_64-posix-seh-rev1, Built by MinGW-Builds project) 13.2.0

C:\> qemu-system-i386 --version
QEMU emulator version 8.0.0
```

## 🏗️ Building CLKernel

Once your tools are set up:

```cmd
cd C:\Users\omerc\Downloads\CLKernel

REM Build everything
build.bat all

REM Run in QEMU
build.bat run

REM Clean build files
build.bat clean
```

## 🐛 Common Issues and Solutions

### Issue: "nasm is not recognized"
**Solution**: NASM is not in PATH. Check installation and PATH configuration.

### Issue: "gcc is not recognized"  
**Solution**: MinGW-w64 is not in PATH. Ensure `mingw64\bin` is added to PATH.

### Issue: "ld: cannot find -lgcc"
**Solution**: Use this GCC command format:
```cmd
gcc -nostdlib -ffreestanding -m32 -march=i686 [files...]
```

### Issue: QEMU fails to start
**Solution**: 
1. Check QEMU installation
2. Try full path: `"C:\Program Files\qemu\qemu-system-i386.exe"`
3. Verify disk image exists: `build\clkernel.img`

### Issue: "Access denied" when creating files
**Solution**: Run Command Prompt as Administrator

## 🎯 Development Workflow

1. **Edit code** in your favorite editor (VS Code recommended)
2. **Build** with `build.bat all`
3. **Test** with `build.bat run`
4. **Debug** - QEMU will show kernel output
5. **Iterate** - modify, build, test

## 🔍 Debugging Tips

### 1. View Bootloader Hex Dump
```cmd
certutil -encodehex build\bootloader.bin bootloader.hex
type bootloader.hex
```

### 2. Check Kernel Size
```cmd
dir build\*.bin
```

### 3. QEMU Monitor Commands
In QEMU, press `Ctrl+Alt+2` to access monitor:
- `info registers` - View CPU registers
- `x/20i 0x7c00` - Disassemble bootloader
- `x/20i 0x8000` - Disassemble kernel

### 4. Enable QEMU Serial Output
```cmd
qemu-system-i386 -drive file=build\clkernel.img,format=raw -serial stdio
```

## 📚 Next Steps

1. **Understand the code** - Read through `boot\boot.asm` and `kernel\core\kernel_main.c`
2. **Modify the kernel** - Add your own features
3. **Expand the build** - Add more modules and components
4. **Learn assembly** - Study the bootloader and entry point code
5. **Implement features** - Work on the async scheduler, AI supervisor, etc.

## 🤔 Alternative Development Options

### Option 1: WSL2 (Windows Subsystem for Linux)
If you prefer Linux tools:
```bash
# In WSL2
sudo apt update
sudo apt install build-essential nasm qemu-system-x86
cd /mnt/c/Users/omerc/Downloads/CLKernel
make all
make run
```

### Option 2: Docker Development Environment
```dockerfile
FROM ubuntu:latest
RUN apt update && apt install -y build-essential nasm qemu-system-x86
WORKDIR /workspace
CMD ["bash"]
```

### Option 3: Virtual Machine
- Install Linux in VirtualBox/VMware
- Use native Linux development tools
- Shared folders for code access

---

Happy kernel hacking! 🚀
