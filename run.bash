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

if [[ "$1" == "--release" ]]; then
    echo "Building in release mode..."
    gcc -O3 -DNDEBUG -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c ./libbf/*.c -o zscript.exe -lm -ldl -lpthread
else
    gcc -g -O3 -Wno-pointer-sign main.c src/core/*.c src/*.c utf/*.c utf/utf8proc/*.c ./libbf/*.c -o zscript.exe -lm -ldl -lpthread
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