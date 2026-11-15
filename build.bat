@echo off
gcc -std=c11 -Wall -Wextra main.c kt/duQuBMP.c kt/jiSuanHash.c kt/hanMingJuLi.c kt/tpruku.c -o pingpang.exe
if errorlevel 1 (echo BUILD FAILED - pingpang.exe & pause & exit /b 1)

gcc -std=c11 -Wall -Wextra -mwindows -finput-charset=GBK -fexec-charset=GBK gui.c kt/duQuBMP.c kt/jiSuanHash.c kt/hanMingJuLi.c kt/tpruku.c -o gui.exe
if errorlevel 1 (echo BUILD FAILED - gui.exe & pause & exit /b 1)

echo BUILD OK
pause
