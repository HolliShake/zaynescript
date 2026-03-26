# ============================================================
#  Makefile for zscript
# ============================================================

TARGET   := zscript.exe

CC       := gcc
CFLAGS   := -Wno-pointer-sign

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
	$(CC) $(CFLAGS) -O3 -DNDEBUG $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful → $(TARGET)"

debug:
	@echo "Building in debug mode..."
	$(CC) $(CFLAGS) -g -O3 $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful → $(TARGET)"

clean:
	rm -f $(TARGET)

run: debug
	LC_ALL=en_US.UTF-8 ./$(TARGET)
