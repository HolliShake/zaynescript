# ============================================================
#  Makefile for zscript
# ============================================================

TARGET   := zscript.exe

CC       := gcc
# ADDED: -fno-omit-frame-pointer for deep ASan traces
CFLAGS   := -Wno-pointer-sign -fsanitize=address,leak -g3 -fno-omit-frame-pointer
CFLAGSR  := -Wno-pointer-sign

# Source files (mirrors run.bash)
SRCS     := main.c \
            $(wildcard src/*.c) \
            $(wildcard src/core/*.c) \
            $(wildcard utf/*.c) \
            $(wildcard utf/utf8proc/*.c) \
            $(wildcard libbf/*.c)

LDFLAGS  := -lm -ldl -lpthread

# ----------------------------------------------------------------

.PHONY: all release debug clean run

all: debug

release:
	@echo "Building in release mode..."
	$(CC) $(CFLAGSR) -O3 -DNDEBUG $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful → $(TARGET)"

debug:
	@echo "Building in debug mode..."
	# FIXED: Changed -O3 to -O0 to prevent function inlining
	$(CC) $(CFLAGS) -O0 $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful → $(TARGET)"

clean:
	rm -f $(TARGET)

run: debug
	# ADDED: ASAN_OPTIONS for deep unwinding
	ASAN_OPTIONS=fast_unwind_on_malloc=0:malloc_context_size=30 LC_ALL=en_US.UTF-8 ./$(TARGET)