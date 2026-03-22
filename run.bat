@echo off
cls

if exist zscript.exe (
    taskkill /f /im zscript.exe 2>nul
    del /f /q zscript.exe 2>nul
    if exist zscript.exe (
        timeout /t 1 >nul
        taskkill /f /im zscript.exe 2>nul
        del /f /q zscript.exe 2>nul
    )
)

if "%1"=="--release" (
    echo Building in release mode...
    gcc -O3 -DNDEBUG -Wno-pointer-sign main.c src\core\*.c src\*.c utf\*.c utf\utf8proc\*.c libbf\*.c -o zscript.exe -lm -ldl -lpthread
) else (
    gcc -g -O3 -Wno-pointer-sign main.c src\core\*.c src\*.c utf\*.c utf\utf8proc\*.c libbf\*.c -o zscript.exe -lm -ldl -lpthread
)

chcp 65001 >nul

if "%1"=="--compile" (
    if exist zscript.exe (
        echo Build successful.
    ) else (
        echo Error: Failed to build zscript.exe
    )
) else if "%1"=="--dbg" (
    if exist zscript.exe (
        gdb -ex run -ex bt --args zscript.exe --run %2
    ) else (
        echo Error: Failed to build zscript.exe
        pause
    )
) else (
    if exist zscript.exe (
        zscript.exe
    ) else (
        echo Error: Failed to build zscript.exe
        pause
    )
)
pause
