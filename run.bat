@echo off
cls

if exist main.exe (
    taskkill /f /im main.exe 2>nul
    del /f /q main.exe 2>nul
    if exist main.exe (
        timeout /t 1 >nul
        taskkill /f /im main.exe 2>nul
        del /f /q main.exe 2>nul
    )
)

gcc -g -O3 -Wno-pointer-sign main.c src\core\*.c src\*.c utf\*.c utf\utf8proc\*.c -o main.exe -static -lm -ldl -lpthread

chcp 65001 >nul

main.exe
pause
