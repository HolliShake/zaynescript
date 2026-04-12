@echo off
cls
chcp 65001 >nul

set "OUT_DIR=dist"
set "EXE=%OUT_DIR%\zscript.exe"
set "SQLITE_DLL=%OUT_DIR%\sqlite3.dll"

:: Build timestamp comes from GCC __DATE__ / __TIME__ (see main.c); no wmic/PowerShell.

:: ── Ensure output directory structure exists ─────────────────────────────────
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

:: ── Sync assets directly into dist\ ──────────────────────────────────────────
xcopy /e /i /y lib "%OUT_DIR%\lib" >nul
xcopy /e /i /y tests "%OUT_DIR%\tests" >nul

:: ── Kill any running copy ────────────────────────────────────────────────────
if exist "%EXE%" (
    taskkill /f /im zscript.exe 2>nul
    timeout /t 1 >nul
    del /f /q "%EXE%" 2>nul
)

:: ── Build SQLite Shared Library (Dynamic) ────────────────────────────────────
echo Building SQLite shared library...
gcc -O2 -shared sqlite\sqlite3.c -o "%SQLITE_DLL%"
if errorlevel 1 (
    echo Error: Failed to build sqlite3.dll
    pause
    exit /b 1
)

:: ── Compile EXEs ─────────────────────────────────────────────────────────────
if "%1"=="--release" (
    echo Building in release mode...
    echo Compiling icon resource ^(docs\zs.ico^)...
    windres -O coff -i zscript.rc -o "%OUT_DIR%\zscript-icon.o"
    if errorlevel 1 (
        echo Error: windres failed ^(is MinGW windres on PATH?^)
        exit /b 1
    )
    rem LTO + --gc-sections: smaller/faster exe; -Wl,-s strips symbols (MinGW).
    rem NUMBER_OF_PROCESSORS is a system env var (safe to expand inside this block).
    gcc -O3 -DNDEBUG -DMG_ENABLE_LOG=0 -flto=%NUMBER_OF_PROCESSORS% -fno-semantic-interposition -ffunction-sections -fdata-sections -fno-ident -Wno-pointer-sign ^
        main.c src\core\*.c src\*.c utf\*.c utf\utf8proc\*.c libbf\*.c mongoose\*.c "%OUT_DIR%\zscript-icon.o" ^
        -o "%EXE%" "%SQLITE_DLL%" -lm -Wl,--gc-sections -Wl,-s
) else (
    echo Building in debug mode...
    gcc -g -O0 -Wno-pointer-sign ^
        main.c src\core\*.c src\*.c utf\*.c utf\utf8proc\*.c libbf\*.c mongoose\*.c ^
        -o "%EXE%" "%SQLITE_DLL%" -lm
)

if not exist "%EXE%" (
    echo Error: Compilation failed.
    pause
    exit /b 1
)

echo Build successful -^> %EXE%

:: ── Dispatch on flag ─────────────────────────────────────────────────────────
if "%1"=="--compile" (
    rem build-only, nothing more to do
) else if "%1"=="--release" (
    rem release build-only
) else if "%1"=="--tests" (
    echo Running test suite...
    "%EXE%" --run tests\test_all.zs
    pause
) else if "%1"=="--dbg" (
    gdb -ex run -ex bt --args "%EXE%" --run %2
    pause
) else (
    "%EXE%"
    pause
)