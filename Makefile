# ============================================================
#  Makefile for zscript
# ============================================================

TARGET   := zscript.exe

CC       := clang
CFLAGS   := -Wno-pointer-sign -fsanitize=address,leak -g3 -fno-omit-frame-pointer -fno-optimize-sibling-calls
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

# Build date stamp
BUILD_DATE := $(shell date '+%Y-%m-%d %H:%M:%S')

# Source files
SRCS     := main.c \
            $(wildcard src/*.c) \
            $(wildcard src/core/*.c) \
            $(wildcard utf/*.c) \
            $(wildcard utf/utf8proc/*.c) \
            $(wildcard libbf/*.c)

SQLITE_SO   := sqlite/libsqlite3.so
MONGOOSE_SO := mongoose/libmongoose.so

LDFLAGS       := -lm -ldl -lpthread -Lsqlite -lsqlite3 -Lmongoose -lmongoose -Wl,-rpath,'$$ORIGIN/sqlite' -Wl,-rpath,'$$ORIGIN/mongoose'
LDFLAGS_INST  := -lm -ldl -lpthread -Lsqlite -lsqlite3 -Lmongoose -lmongoose -Wl,-rpath,'$$ORIGIN/../lib/zscript' -Wl,-rpath,'$$ORIGIN/../lib/zscript'

# ----------------------------------------------------------------

PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin
LIBDIR   ?= $(PREFIX)/lib/zscript

.PHONY: all release release-install debug clean run install uninstall amalgamate

all: debug

release: $(SQLITE_SO) $(MONGOOSE_SO)
	@echo "Building in release mode (clang, super-optimized)..."
	$(CC) $(CFLAGSR) $(CFLAGS_OPT) -DBUILD_DATE='"$(BUILD_DATE)"' $(SRCS) -o $(TARGET) $(LDFLAGS) $(LDFLAGS_OPT)
	@echo "Build successful → $(TARGET)"

release-install: $(SQLITE_SO) $(MONGOOSE_SO)
	@echo "Building in release mode (install RPATH)..."
	$(CC) $(CFLAGSR) $(CFLAGS_OPT) -DBUILD_DATE='"$(BUILD_DATE)"' $(SRCS) -o $(TARGET) $(LDFLAGS_INST) $(LDFLAGS_OPT)
	@echo "Build successful → $(TARGET)"

debug: $(SQLITE_SO) $(MONGOOSE_SO)
	@echo "Building in debug mode..."
	$(CC) $(CFLAGS) -O0 -DBUILD_DATE='"$(BUILD_DATE)"' $(SRCS) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful → $(TARGET)"

$(SQLITE_SO): sqlite/sqlite3.c
	@echo "Building SQLite shared library..."
	$(CC) -fPIC -shared -O2 -o $@ $<

$(MONGOOSE_SO): mongoose/mongoose.c
	@echo "Building Mongoose shared library..."
	$(CC) -fPIC -shared -O2 -o $@ $<

clean:
	rm -f $(TARGET) $(SQLITE_SO) $(MONGOOSE_SO)

install: release-install
	@echo "Installing $(TARGET) → $(BINDIR)/zscript"
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/zscript
	install -d $(LIBDIR)
	@if [ ! -f $(LIBDIR)/libsqlite3.so ]; then \
		echo "Installing $(SQLITE_SO) → $(LIBDIR)/libsqlite3.so"; \
		install -m 755 $(SQLITE_SO) $(LIBDIR)/libsqlite3.so; \
	else \
		echo "Skipping $(LIBDIR)/libsqlite3.so (already exists)"; \
	fi
	@if [ ! -f $(LIBDIR)/libmongoose.so ]; then \
		echo "Installing $(MONGOOSE_SO) → $(LIBDIR)/libmongoose.so"; \
		install -m 755 $(MONGOOSE_SO) $(LIBDIR)/libmongoose.so; \
	else \
		echo "Skipping $(LIBDIR)/libmongoose.so (already exists)"; \
	fi
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