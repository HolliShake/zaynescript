@echo off
cls
chcp 65001 >nul

set "OUT_DIR=win32"
set "EXE=%OUT_DIR%\zscript.exe"
set "DLL=%OUT_DIR%\sqlite3.dll"
set "MONGOOSE_DLL=%OUT_DIR%\mongoose.dll"
set "LIB_DIR=%OUT_DIR%\lib"

for /f "tokens=2 delims==." %%I in ('wmic os get localdatetime /value') do set "DT=%%I"
set "BUILD_DATE=%DT:~0,4%-%DT:~4,2%-%DT:~6,2% %DT:~8,2%:%DT:~10,2%:%DT:~12,2%"

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
    gcc -shared -O2 -o "%MONGOOSE_DLL%" mongoose\mongoose.c -Wl,--out-implib,mongoose\libmongoose.a
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
    gcc -O3 -DNDEBUG -DBUILD_DATE="\"%BUILD_DATE%\"" -Wno-pointer-sign ^
        main.c src\core\*.c src\*.c utf\*.c utf\utf8proc\*.c libbf\*.c ^
        -o "%EXE%" -lm -Lsqlite -lsqlite3 -Lmongoose -lmongoose
) else (
    echo Building in debug mode...
    gcc -g -O0 -DBUILD_DATE="\"%BUILD_DATE%\"" -Wno-pointer-sign ^
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
