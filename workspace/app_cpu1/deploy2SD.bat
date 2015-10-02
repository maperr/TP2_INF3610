@echo off
set BUILDCONFIG=%1
set RUNPATH=%~DP0
set XILPATH=C:\Logiciels\Xilinx\14.4\ISE_DS

call %XILPATH%\settings64.bat
cd %RUNPATH%
echo Copying files
copy %BUILDCONFIG%\app_cpu1.elf ..\..\bootgen\
copy ..\amp_fsbl\Debug\amp_fsbl.elf ..\..\bootgen\
copy ..\edk_system_hw_platform\system.bit ..\..\bootgen\
copy ..\softuart\Debug\softuart.elf ..\..\SDCARD\
copy ..\sendpacket\Debug\sendpacket.elf ..\..\SDCARD\
copy ..\rwmem\Debug\rwmem.elf ..\..\SDCARD\
cd ..\..\bootgen\
echo Generating boot.bin...
call createBoot.bat
copy BOOT.BIN ..\SDCARD\
echo SDCARD directory updated
cd %RUNPATH%