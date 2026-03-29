# ============================================================
#  Makefile for zscript
# ============================================================

TARGET   := zscript.exe

CC       := gcc
# ADDED: -fno-omit-frame-pointer for deep ASan traces
CFLAGS   := -Wno-pointer-sign -fsanitize=address,leak -g3 -fno-omit-frame-pointer
CFLAGSR  := -Wno-pointer-sign

# ── Super-optimized release flags ──────────────────────────
# -O3              : full optimizations (vectorize, inline, unroll)
# -march=native    : target the exact CPU this is built on
# -mtune=native    : schedule instructions for this CPU
# -flto            : link-time optimization across all TUs
# -fomit-frame-pointer  : free up a register
# -funroll-loops   : unroll hot loops
# -fno-plt         : direct calls, skip PLT indirection
# -ffunction-sections / -fdata-sections + --gc-sections : strip dead code
# -fmerge-all-constants  : merge identical read-only data
# -fno-semantic-interposition : allow aggressive inlining
# -fipa-pta        : interprocedural pointer analysis
# -fdevirtualize-at-ltrans : devirt at LTO time
# -fno-math-errno / -fno-trapping-math : safe fast-math subset
# -pipe            : use pipes between compiler stages
CFLAGS_OPT := -O3 -march=native -mtune=native \
              -flto -fomit-frame-pointer -funroll-loops -fno-plt \
              -ffunction-sections -fdata-sections \
              -fmerge-all-constants -fno-semantic-interposition \
              -fipa-pta -fdevirtualize-at-ltrans \
              -fno-math-errno -fno-trapping-math \
              -pipe -DNDEBUG

LDFLAGS_OPT := -flto -Wl,--gc-sections -Wl,-O1 -Wl,--strip-all

# PGO profile directory
PGO_DIR  := .pgo-data

# Source files (mirrors run.bash)
SRCS     := main.c \
            $(wildcard src/*.c) \
            $(wildcard src/core/*.c) \
            $(wildcard utf/*.c) \
            $(wildcard utf/utf8proc/*.c) \
            $(wildcard libbf/*.c)

LDFLAGS  := -lm -ldl -lpthread

# ----------------------------------------------------------------

PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin
LIBDIR   ?= $(PREFIX)/lib/zscript

.PHONY: all release debug clean run pgo-gen pgo-use pgo clean-pgo install uninstall

all: debug

release:
	@echo "Building in release mode (super-optimized)..."
	$(CC) $(CFLAGSR) $(CFLAGS_OPT) $(SRCS) -o $(TARGET) $(LDFLAGS) $(LDFLAGS_OPT)
	@echo "Build successful → $(TARGET)"

debug:
	@echo "Building in debug mode..."
	# FIXED: Changed -O3 to -O0 to prevent function inlining
	$(CC) $(CFLAGS) -O0 $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful → $(TARGET)"

# ── Profile-Guided Optimization (2-pass) ──────────────────
# Usage:  make pgo TRAIN=./tests/test_benchloop.zs
TRAIN ?= ./tests/test_benchloop.zs

pgo-gen:
	@echo "[PGO 1/2] Building instrumented binary..."
	@mkdir -p $(PGO_DIR)
	$(CC) $(CFLAGSR) $(CFLAGS_OPT) -fprofile-generate=$(PGO_DIR) \
	    $(SRCS) -o $(TARGET) $(LDFLAGS) -lgcov
	@echo "Running training workload: $(TRAIN)"
	./$(TARGET) --run $(TRAIN) || true

pgo-use:
	@echo "[PGO 2/2] Building PGO-optimized binary..."
	$(CC) $(CFLAGSR) $(CFLAGS_OPT) -fprofile-use=$(PGO_DIR) -fprofile-correction \
	    $(SRCS) -o $(TARGET) $(LDFLAGS) $(LDFLAGS_OPT)
	@echo "Build successful (PGO) → $(TARGET)"

pgo: pgo-gen pgo-use
	@echo "PGO build complete → $(TARGET)"

clean-pgo:
	rm -rf $(PGO_DIR)

clean: clean-pgo
	rm -f $(TARGET)

install: release
	@echo "Installing $(TARGET) → $(BINDIR)/zscript"
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/zscript
	@echo "Installing lib → $(LIBDIR)/lib/"
	install -d $(LIBDIR)/lib
	install -m 644 lib/*.zs $(LIBDIR)/lib/
	@echo "Installing tests → $(LIBDIR)/tests/"
	install -d $(LIBDIR)/tests
	install -m 644 tests/*.zs $(LIBDIR)/tests/

uninstall:
	@echo "Removing $(BINDIR)/zscript"
	rm -f $(BINDIR)/zscript
	@echo "Removing $(LIBDIR)/"
	rm -rf $(LIBDIR)

run: debug
	# ADDED: ASAN_OPTIONS for deep unwinding
	ASAN_OPTIONS=fast_unwind_on_malloc=0:malloc_context_size=30 LC_ALL=en_US.UTF-8 ./$(TARGET)