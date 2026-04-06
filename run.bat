@echo off
cls
chcp 65001 >nul

set "OUT_DIR=win32"
set "EXE=%OUT_DIR%\zscript.exe"
set "DLL=%OUT_DIR%\sqlite3.dll"
set "MONGOOSE_DLL=%OUT_DIR%\mongoose.dll"
set "LIB_DIR=%OUT_DIR%\lib"

:: Build timestamp comes from GCC __DATE__ / __TIME__ (see main.c); no wmic/PowerShell.

:: ── Ensure output directory structure exists ─────────────────────────────────
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
if not exist "%LIB_DIR%" mkdir "%LIB_DIR%"

:: ── Build sqlite3.dll into win32\ if not already present ─────────────────────
if not exist "%DLL%" (
    echo Building SQLite DLL...
    gcc -shared -O2 -o "%DLL%" sqlite\sqlite3.c -Wl,--out-implib,sqlite\libsqlite3.a
)
if not exist "%DLL%" (
    echo Error: Failed to build sqlite3.dll
    exit /b 1
)

:: ── Build mongoose.dll into win32\ if not already present ────────────────────
if not exist "%MONGOOSE_DLL%" (
    echo Building Mongoose DLL...
    gcc -shared -O2 -o "%MONGOOSE_DLL%" mongoose\mongoose.c -Wl,--out-implib,mongoose\libmongoose.a -lws2_32
)
if not exist "%MONGOOSE_DLL%" (
    echo Error: Failed to build mongoose.dll
    exit /b 1
)

:: ── Kill any running copy ─────────────────────────────────────────────────────
if exist "%EXE%" (
    taskkill /f /im zscript.exe 2>nul
    timeout /t 1 >nul
    del /f /q "%EXE%" 2>nul
)

:: ── Compile ───────────────────────────────────────────────────────────────────
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
    gcc -O3 -DNDEBUG -flto=%NUMBER_OF_PROCESSORS% -fno-semantic-interposition -ffunction-sections -fdata-sections -fno-ident -Wno-pointer-sign ^
        main.c src\core\*.c src\*.c utf\*.c utf\utf8proc\*.c libbf\*.c "%OUT_DIR%\zscript-icon.o" ^
        -o "%EXE%" -lm -Lsqlite -lsqlite3 -Lmongoose -lmongoose -Wl,--gc-sections -Wl,-s
) else (
    echo Building in debug mode...
    gcc -g -O0 -Wno-pointer-sign ^
        main.c src\core\*.c src\*.c utf\*.c utf\utf8proc\*.c libbf\*.c ^
        -o "%EXE%" -lm -Lsqlite -lsqlite3 -Lmongoose -lmongoose
)

if not exist "%EXE%" (
    echo Error: Compilation failed.
    pause
    exit /b 1
)

:: ── Sync lib\ folder into win32\lib\ ─────────────────────────────────────────
xcopy /e /i /y lib "%LIB_DIR%" >nul

echo Build successful ^> %EXE%

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
