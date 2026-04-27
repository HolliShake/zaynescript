@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem ============================================================
rem  build.bat for zscript (Windows mirror of Makefile)
rem ============================================================

set "DIST_DIR=dist"
set "TARGET=%DIST_DIR%\zscript.exe"
set "SQLITE_LIB=%DIST_DIR%\libsqlite3.dll"
set "MARIADB_DLL=%DIST_DIR%\mariadb-connector-c.dll"
set "THIRDPARTY_DIR=thirdparty"
set "MARIADB_SRC=%THIRDPARTY_DIR%\mariadb-connector-c"
set "MARIADB_BUILD=%MARIADB_SRC%\build-win"
set "GOAL=%~1"

if "%CC%"=="" (
    where clang >nul 2>nul && set "CC=clang" || set "CC=gcc"
)

for /f "usebackq delims=" %%I in (`powershell -NoProfile -Command "(Get-Date).ToString('yyyy-MM-ddTHH:mm:ss')"`) do set "BUILD_DATE=%%I"

set "CFLAGS_BASE=-Wno-pointer-sign -DBUILD_DATE=\"%BUILD_DATE%\""
set "LDFLAGS=-lm -ldl -lpthread -lws2_32"
set "CFLAGS_DBG=-g3 -O0 -fno-omit-frame-pointer -fno-optimize-sibling-calls"
set "CFLAGS_REL=-O3 -march=native -mtune=native -fomit-frame-pointer -funroll-loops -fno-plt -ffunction-sections -fdata-sections -fmerge-all-constants -fno-semantic-interposition -fno-math-errno -fno-trapping-math -fstrict-aliasing -pipe -DNDEBUG -DMG_ENABLE_LOG=0"
set "LDFLAGS_REL=-Wl,--gc-sections -Wl,-O2 -Wl,--strip-all"
if /i "%CC%"=="clang" set "LDFLAGS_REL=-fuse-ld=lld -Wl,--gc-sections -Wl,-O2 -Wl,--strip-all"

if /i "%GOAL%"=="clean" goto :clean
if /i "%GOAL%"=="run" goto :run
if /i "%GOAL%"=="release" goto :release
if /i "%GOAL%"=="debug" goto :debug
if /i "%GOAL%"=="all" goto :debug
if "%GOAL%"=="" goto :debug

echo Unknown target "%GOAL%".
echo Usage: build.bat [debug^|release^|run^|clean^|all]
exit /b 1

:prepare
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"
xcopy /e /i /y lib "%DIST_DIR%\lib" >nul 2>nul
xcopy /e /i /y tests "%DIST_DIR%\tests" >nul 2>nul
goto :eof

:sqlite
call :prepare
echo Building SQLite shared library...
"%CC%" -fPIC -shared -O2 -o "%SQLITE_LIB%" "%THIRDPARTY_DIR%\sqlite\sqlite3.c"
if errorlevel 1 (
    echo Error: Failed to build %SQLITE_LIB%
    exit /b 1
)
goto :eof

:mariadb
call :prepare
echo Building MariaDB Connector/C shared library...
if not exist "%MARIADB_SRC%\CMakeLists.txt" (
    echo Error: MariaDB source not found at %MARIADB_SRC%
    exit /b 1
)

if not exist "%MARIADB_BUILD%" mkdir "%MARIADB_BUILD%"

cmake -S "%MARIADB_SRC%" -B "%MARIADB_BUILD%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DWITH_UNIT_TESTS=OFF -DCMAKE_SH="CMAKE_SH-NOTFOUND"
if errorlevel 1 (
    echo Error: Failed to configure MariaDB Connector/C
    exit /b 1
)

mingw32-make -C "%MARIADB_BUILD%"
if errorlevel 1 (
    echo Error: Failed to build MariaDB Connector/C
    exit /b 1
)

set "MARIADB_BUILT_DLL="
for %%F in (
    "%MARIADB_BUILD%\libmariadb\libmariadb.dll"
    "%MARIADB_BUILD%\libmariadb.dll"
    "%MARIADB_BUILD%\libmariadb\Release\libmariadb.dll"
) do (
    if exist "%%~F" (
        set "MARIADB_BUILT_DLL=%%~F"
        goto :_mariadb_copy
    )
)

echo Error: Could not locate built libmariadb.dll
exit /b 1

:_mariadb_copy
copy /y "%MARIADB_BUILT_DLL%" "%MARIADB_DLL%" >nul
if errorlevel 1 (
    echo Error: Failed to copy MariaDB DLL to %MARIADB_DLL%
    exit /b 1
)
echo MariaDB DLL ready -^> %MARIADB_DLL%
goto :eof

:collect_sources
set "SRCS=main.c"
for %%F in (src\*.c) do set "SRCS=!SRCS! %%F"
for %%F in (src\core\*.c) do set "SRCS=!SRCS! %%F"
for %%F in (utf\*.c) do set "SRCS=!SRCS! %%F"
for %%F in (utf\utf8proc\*.c) do set "SRCS=!SRCS! %%F"
for %%F in (%THIRDPARTY_DIR%\libbf\*.c) do (
    if /i not "%%~nxF"=="cutils.c" set "SRCS=!SRCS! %%F"
)
for %%F in (%THIRDPARTY_DIR%\libregex\*.c) do set "SRCS=!SRCS! %%F"
for %%F in (%THIRDPARTY_DIR%\mongoose\*.c) do set "SRCS=!SRCS! %%F"
set "SRCS=!SRCS! %THIRDPARTY_DIR%\crypto\tiny-AES-c\aes.c"
set "SRCS=!SRCS! %THIRDPARTY_DIR%\crypto\RHash\librhash\ripemd-160.c"
set "SRCS=!SRCS! %THIRDPARTY_DIR%\crypto\pqcleanfips202\pqclean_fips202.c"
goto :eof

:debug
call :sqlite || exit /b 1
call :mariadb || exit /b 1
call :collect_sources
echo Building in debug mode (sqlite dynamic)...
"%CC%" %CFLAGS_BASE% %CFLAGS_DBG% !SRCS! -o "%TARGET%" %LDFLAGS% -L"%DIST_DIR%" -lsqlite3
if errorlevel 1 exit /b 1
echo Build successful -^> %TARGET%
exit /b 0

:release
call :sqlite || exit /b 1
call :mariadb || exit /b 1
call :collect_sources
echo Building in release mode (sqlite dynamic)...
"%CC%" %CFLAGS_BASE% %CFLAGS_REL% !SRCS! -o "%TARGET%" %LDFLAGS% %LDFLAGS_REL% -L"%DIST_DIR%" -lsqlite3
if errorlevel 1 exit /b 1
echo Build successful -^> %TARGET%
exit /b 0

:run
call :debug || exit /b 1
set "LC_ALL=en_US.UTF-8"
"%TARGET%"
exit /b %errorlevel%

:clean
if exist "%DIST_DIR%" (
    echo Removing %DIST_DIR%...
    rmdir /s /q "%DIST_DIR%"
)
exit /b 0
