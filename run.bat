@echo off
cls


if exist main.exe del main.exe

gcc -o main main.c src/*.c utf/*.c utf/utf8proc/*.c -g
chcp 65001 >nul
.\main.exe
pause