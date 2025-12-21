@echo off
cls


if exist main.exe del main.exe

clang -w -fno-diagnostics-show-note-include-stack -o main main.c src/*.c utf/*.c utf/utf8proc/*.c -lm -g
chcp 65001 >nul
.\main.exe
pause