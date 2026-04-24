@echo off
setlocal EnableExtensions
cls
chcp 65001 >nul

:: Mirrors Makefile logic (sources, BUILD_DATE, debug/release flags, run env).
:: Windows uses MinGW gcc and links sqlite as a DLL next to the exe.

set "OUT_DIR=dist"
set "EXE=%OUT_DIR%\zscript.exe"
set "SQLITE_DLL=%OUT_DIR%\libsqlite3.dll"

for /f "usebackq delims=" %%I in (`powershell -NoProfile -Command "(Get-Date).ToString('yyyy-MM-dd HH:mm:ss')"`) do set "BUILD_DATE=%%I"

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

xcopy /e /i /y lib "%OUT_DIR%\lib" >nul
xcopy /e /i /y tests "%OUT_DIR%\tests" >nul

if exist "%EXE%" (
	taskkill /f /im zscript.exe 2>nul
	timeout /t 1 >nul
	del /f /q "%EXE%" 2>nul
)

echo Building SQLite shared library...
gcc -fPIC -shared -O2 sqlite\sqlite3.c -o "%SQLITE_DLL%"
if errorlevel 1 (
	echo Error: Failed to build %SQLITE_DLL%
	pause
	exit /b 1
)

set "CFLAGS_BASE=-Wno-pointer-sign -DBUILD_DATE=\"%BUILD_DATE%\""

if "%1"=="--release" (
	echo Building in release mode...
	echo Compiling icon resource ^(docs\zs.ico^)...
	windres -O coff -i zscript.rc -o "%OUT_DIR%\zscript-icon.o"
	if errorlevel 1 (
		echo Error: windres failed ^(is MinGW windres on PATH?^)
		exit /b 1
	)
	rem Matches Makefile release: CFLAGS_REL + LDFLAGS_REL ^(gcc/MinGW; no lld requirement^)
	rem Sources match Makefile ALL_SRCS minus sqlite/* (dynamic) and minus libbf/cutils.c (cutils from libregex).
	gcc %CFLAGS_BASE% -O3 -march=native -mtune=native -flto=thin -fomit-frame-pointer -funroll-loops -fno-plt -ffunction-sections -fdata-sections -fmerge-all-constants -fno-semantic-interposition -fno-math-errno -fno-trapping-math -fstrict-aliasing -fvectorize -fslp-vectorize -pipe -DNDEBUG -DMG_ENABLE_LOG=0 ^
		main.c src\*.c src\core\*.c utf\*.c utf\utf8proc\*.c libbf\libbf.c libregex\*.c mongoose\*.c "%OUT_DIR%\zscript-icon.o" ^
		-o "%EXE%" -L"%OUT_DIR%" -lsqlite3 -lm -flto=thin -Wl,--gc-sections -Wl,-O2 -Wl,--strip-all
) else (
	echo Building in debug mode...
	rem Matches Makefile debug: CFLAGS_BASE + CFLAGS_DBG; same source set as release ^(no sqlite/*.c in link^).
	gcc %CFLAGS_BASE% -g3 -O0 -fsanitize=address,leak -fno-omit-frame-pointer -fno-optimize-sibling-calls ^
		main.c src\*.c src\core\*.c utf\*.c utf\utf8proc\*.c libbf\libbf.c libregex\*.c mongoose\*.c ^
		-o "%EXE%" -L"%OUT_DIR%" -lsqlite3 -lm
)

if not exist "%EXE%" (
	echo Error: Compilation failed.
	pause
	exit /b 1
)

echo Build successful -^> %EXE%

if "%1"=="--compile" (
	rem build-only
) else if "%1"=="--release" (
	rem release build-only
) else if "%1"=="--tests" (
	echo Running test suite...
	set "LC_ALL=en_US.UTF-8"
	"%EXE%" --run tests\test_all.zs
	pause
) else if "%1"=="--dbg" (
	set "LC_ALL=en_US.UTF-8"
	gdb -ex run -ex bt --args "%EXE%" --run %2
	pause
) else (
	set "ASAN_OPTIONS=fast_unwind_on_malloc=0:malloc_context_size=30"
	set "LC_ALL=en_US.UTF-8"
	"%EXE%"
	pause
)
