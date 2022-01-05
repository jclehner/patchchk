@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
rmdir /S /Q build
mkdir build
cd build
cmake -G "NMake Makefiles" ..
nmake package
cd ..
