@echo off
rem Build and test minimal bootloader

echo Building minimal bootloader test...

if not exist build mkdir build

rem Build minimal bootloader
nasm -f bin boot\boot_test.asm -o build\bootloader_test.bin
if errorlevel 1 goto error

rem Create 1.44MB floppy image 
fsutil file createnew build\test.img 1474560 >nul

rem Write bootloader to first sector
copy /b build\bootloader_test.bin build\test.img >nul

echo SUCCESS! Bootloader ready.
echo Testing: 
& "C:\Program Files\qemu\qemu-system-i386.exe" -drive format=raw,file=build\test.img -m 16M

goto end

:error
echo BUILD FAILED!
pause

:end
