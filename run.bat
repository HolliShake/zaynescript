@echo off
cls


if exist main.exe del main.exe

gcc -g -O3 -Wno-pointer-sign main.c src/*.c utf/*.c utf/utf8proc/*.c -o main.exe -static -lm -ldl -lpthread
chcp 65001 >nul
.\main.exe
pause