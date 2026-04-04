#!/usr/bin/env bash
set -e

clear

if [[ -f zscript.exe ]]; then
    pkill -f zscript.exe 2>/dev/null || true
    rm -f zscript.exe 2>/dev/null || true
    if [[ -f zscript.exe ]]; then
        sleep 1
        pkill -f zscript.exe 2>/dev/null || true
        rm -f zscript.exe 2>/dev/null || true
    fi
fi

BUILD_DATE=$(date '+%Y-%m-%d %H:%M:%S')

# Build SQLite as a shared library if not already built
if [[ ! -f sqlite/libsqlite3.so ]]; then
    echo "Building SQLite shared library..."
    gcc -fPIC -shared -O2 -o sqlite/libsqlite3.so sqlite/sqlite3.c
fi

# Build Mongoose as a shared library if not already built
if [[ ! -f mongoose/libmongoose.so ]]; then
    echo "Building Mongoose shared library..."
    gcc -fPIC -shared -O2 -o mongoose/libmongoose.so mongoose/mongoose.c
fi

if [[ "$1" == "--release" ]]; then
    echo "Building in release mode..."
    gcc -O3 -DNDEBUG -DBUILD_DATE="\"$BUILD_DATE\"" -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c ./libbf/*.c -o zscript.exe -lm -ldl -lpthread -Lsqlite -lsqlite3 -Lmongoose -lmongoose -Wl,-rpath,'$ORIGIN/sqlite' -Wl,-rpath,'$ORIGIN/mongoose'
else
    gcc -g -O3 -DBUILD_DATE="\"$BUILD_DATE\"" -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c ./libbf/*.c -o zscript.exe -lm -ldl -lpthread -Lsqlite -lsqlite3 -Lmongoose -lmongoose -Wl,-rpath,'$ORIGIN/sqlite' -Wl,-rpath,'$ORIGIN/mongoose'
fi

export LC_ALL=en_US.UTF-8

if [[ "$1" == "--compile" ]]; then
    if [[ -f zscript.exe ]]; then
        echo "Build successful."
    else
        echo "Error: Failed to build zscript.exe"
    fi
elif [[ "$1" == "--format" ]]; then
    echo "Running clang-format..."
    find src/ -name '*.c' -o -name '*.h' | xargs clang-format -i
    clang-format -i main.c
    echo "Formatting complete."
elif [[ "$1" == "--dbg" ]]; then
    # compile first
    echo "Building in debug mode..."
    gcc -g -O3 -DBUILD_DATE="\"$BUILD_DATE\"" -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c ./libbf/*.c -o zscript.exe -lm -ldl -lpthread -Lsqlite -lsqlite3 -Lmongoose -lmongoose -Wl,-rpath,'$ORIGIN/sqlite' -Wl,-rpath,'$ORIGIN/mongoose'
    if [[ -f zscript.exe ]]; then
        gdb -ex run -ex bt --args ./zscript.exe --run "$2"
    else
        echo "Error: Failed to build zscript.exe"
        read -n 1 -s -r -p "Press any key to continue..."
        echo
    fi
else
    if [[ -f zscript.exe ]]; then
        ./zscript.exe
    else
        echo "Error: Failed to build zscript.exe"
        read -n 1 -s -r -p "Press any key to continue..."
        echo
    fi
fi

read -n 1 -s -r -p "Press any key to continue..."
echo