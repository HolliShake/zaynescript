# ============================================================
#  Makefile for zscript
# ============================================================

TARGET   := zscript.exe

CC       := clang
CFLAGS   := -Wno-pointer-sign -fsanitize=address,leak -g3 -fno-omit-frame-pointer
CFLAGSR  := -Wno-pointer-sign

# ── Super-optimized release flags ──────────────────────────
CFLAGS_OPT := -O3 -march=native -mtune=native \
              -flto=thin -fomit-frame-pointer -funroll-loops -fno-plt \
              -ffunction-sections -fdata-sections \
              -fmerge-all-constants -fno-semantic-interposition \
              -fno-math-errno -fno-trapping-math \
              -fstrict-aliasing -fvectorize -fslp-vectorize \
              -pipe -DNDEBUG

LDFLAGS_OPT := -flto=thin -fuse-ld=lld -Wl,--gc-sections -Wl,-O2 -Wl,--strip-all

# Source files
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

.PHONY: all release debug clean run install uninstall amalgamate

all: debug

release:
	@echo "Building in release mode (clang, super-optimized)..."
	$(CC) $(CFLAGSR) $(CFLAGS_OPT) $(SRCS) -o $(TARGET) $(LDFLAGS) $(LDFLAGS_OPT)
	@echo "Build successful → $(TARGET)"

debug:
	@echo "Building in debug mode..."
	$(CC) $(CFLAGS) -O0 $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful → $(TARGET)"

clean:
	rm -f $(TARGET)

install: release
	@echo "Installing $(TARGET) → $(BINDIR)/zscript"
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/zscript
	@echo "Installing lib → $(LIBDIR)/lib/"
	cd lib && find . -type d -exec install -d $(LIBDIR)/lib/{} \; \
	       && find . -type f -exec install -m 644 {} $(LIBDIR)/lib/{} \;
	@echo "Installing tests → $(LIBDIR)/tests/"
	cd tests && find . -type d -exec install -d $(LIBDIR)/tests/{} \; \
	         && find . -type f -exec install -m 644 {} $(LIBDIR)/tests/{} \;

uninstall:
	@echo "Removing $(BINDIR)/zscript"
	rm -f $(BINDIR)/zscript
	@echo "Removing $(LIBDIR)/"
	rm -rf $(LIBDIR)

run: debug
	ASAN_OPTIONS=fast_unwind_on_malloc=0:malloc_context_size=30 LC_ALL=en_US.UTF-8 ./$(TARGET)

amalgamate:
	@echo "Running amalgamation..."
	python3 amalgamate.py
	@echo "Amalgamated files in dist/"