#!/usr/bin/env bash
set -e

clear

DIST_DIR="dist"
EXE="$DIST_DIR/zscript.exe"
SQLITE_LIB="$DIST_DIR/libsqlite3.so"
BUILD_DATE=$(date '+%Y-%m-%d %H:%M:%S')

# ── Ensure output directory exists and sync assets ────────────
mkdir -p "$DIST_DIR"
cp -rn lib "$DIST_DIR/" 2>/dev/null || true
cp -rn tests "$DIST_DIR/" 2>/dev/null || true

# ── Kill any running instance ─────────────────────────────────
if [[ -f "$EXE" ]]; then
    pkill -f "$EXE" 2>/dev/null || true
    rm -f "$EXE" 2>/dev/null || true
    if [[ -f "$EXE" ]]; then
        sleep 1
        pkill -f "$EXE" 2>/dev/null || true
        rm -f "$EXE" 2>/dev/null || true
    fi
fi

# ── Format ────────────────────────────────────────────────────
if [[ "$1" == "--format" ]]; then
    echo "Running clang-format..."
    find src/ -name '*.c' -o -name '*.h' | xargs clang-format -i
    clang-format -i main.c
    echo "Formatting complete."
    exit 0
fi

# ── Build SQLite Shared Library (Dynamic) ─────────────────────
if [[ "$1" != "--minimal" ]]; then
    echo "Building SQLite shared library..."
    gcc -O2 -fPIC -shared sqlite/sqlite3.c -o "$SQLITE_LIB"
fi

# RPATH tells the Linux linker to look in the executable's current directory for .so files
RPATH_FLAG='-Wl,-rpath,$ORIGIN'

# ── Compile Executable ────────────────────────────────────────
if [[ "$1" == "--minimal" ]]; then
    echo "Building in minimal mode (sqlite completely disabled, ZSMINIMAL defined)..."
    SRC_FILES=(main.c $(find src/core -name '*.c' | grep -vE 'sqlite') $(find src -maxdepth 1 -name '*.c') $(find utf -name '*.c') $(find utf/utf8proc -name '*.c') $(find libbf -name '*.c') $(find mongoose -name '*.c'))
    gcc -O3 -DNDEBUG -DZSMINIMAL -DBUILD_DATE="\"$BUILD_DATE\"" -Wno-pointer-sign "${SRC_FILES[@]}" -o "$EXE" -lm -ldl -lpthread
elif [[ "$1" == "--release" ]]; then
    echo "Building in release mode..."
    gcc -O3 -DNDEBUG -DBUILD_DATE="\"$BUILD_DATE\"" -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c ./libbf/*.c mongoose/mongoose.c -o "$EXE" -L"$DIST_DIR" -lsqlite3 "$RPATH_FLAG" -lm -ldl -lpthread
else
    echo "Building in debug mode..."
    gcc -g -O3 -DBUILD_DATE="\"$BUILD_DATE\"" -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c ./libbf/*.c mongoose/mongoose.c -o "$EXE" -L"$DIST_DIR" -lsqlite3 "$RPATH_FLAG" -lm -ldl -lpthread
fi

# ── Execution Logic ───────────────────────────────────────────
export LC_ALL=en_US.UTF-8

if [[ ! -f "$EXE" ]]; then
    echo "Error: Failed to build $EXE"
    read -n 1 -s -r -p "Press any key to continue..."
    echo
    exit 1
fi

echo "Build successful -> $EXE"

if [[ "$1" == "--compile" || "$1" == "--release" || "$1" == "--minimal" ]]; then
    # build-only, do nothing
    exit 0
elif [[ "$1" == "--dbg" ]]; then
    gdb -ex run -ex bt --args "$EXE" --run "$2"
else
    "$EXE"
fi