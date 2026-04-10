# ============================================================
#  Makefile for zscript
# ============================================================

DIST_DIR   := dist
TARGET     := $(DIST_DIR)/zscript.exe
SQLITE_LIB := $(DIST_DIR)/libsqlite3.so

# Forced to clang to prevent GCC cc1 errors with -flto=thin
CC         := clang

# ── Directories ─────────────────────────────────────────────
PREFIX     ?= /usr/local
BINDIR     ?= $(PREFIX)/bin
LIBDIR     ?= $(PREFIX)/lib/zscript

# ── Source Files ────────────────────────────────────────────
ALL_SRCS   := main.c \
              $(wildcard src/*.c) \
              $(wildcard src/core/*.c) \
              $(wildcard utf/*.c) \
              $(wildcard utf/utf8proc/*.c) \
              $(wildcard libbf/*.c) \
              $(wildcard mongoose/*.c)\
              $(wildcard sqlite/*.c)

# Exclude SQLite for dynamic builds
DYN_EXCLUDES := sqlite/%

# Helper macro to build exclude patterns for each feature
MIN_EXCLUDES := $(foreach f,$(MINIMAL_DISABLE_FEATURES),$f/% )

# Helper: Is a feature enabled? Usage: $(call feature_enabled,sqlite)
feature_enabled = $(if $(findstring $(1),$(MINIMAL_DISABLE_FEATURES)),0,1)

# Filter sources based on target
ifeq ($(MAKECMDGOALS),minimal)
    SRCS := $(filter-out $(MIN_EXCLUDES),$(ALL_SRCS))
else ifeq ($(MAKECMDGOALS),release)
    SRCS := $(filter-out $(DYN_EXCLUDES),$(ALL_SRCS))
else ifeq ($(MAKECMDGOALS),debug)
    SRCS := $(filter-out $(DYN_EXCLUDES),$(ALL_SRCS))
else
    SRCS := $(ALL_SRCS)
endif

BUILD_DATE := $(shell date '+%Y-%m-%d %H:%M:%S')

# ── Base Flags (Applied to all targets) ─────────────────────
CFLAGS_BASE := -Wno-pointer-sign -DBUILD_DATE='"$(BUILD_DATE)"'
LDFLAGS     := -lm -ldl -lpthread

# Default RPATH points to the directory containing the executable ($ORIGIN)
RPATH_FLAG  := -Wl,-rpath,'$$ORIGIN'

# ── Debug Flags ─────────────────────────────────────────────
CFLAGS_DBG  := -g3 -O0 -fsanitize=address,leak -fno-omit-frame-pointer -fno-optimize-sibling-calls

# ── Release Flags (Super-optimized for speed) ───────────────
CFLAGS_REL  := -O3 -march=native -mtune=native \
               -flto=thin -fomit-frame-pointer -funroll-loops -fno-plt \
               -ffunction-sections -fdata-sections \
               -fmerge-all-constants -fno-semantic-interposition \
               -fno-math-errno -fno-trapping-math \
               -fstrict-aliasing -fvectorize -fslp-vectorize \
               -pipe -DNDEBUG -DMG_ENABLE_LOG=0
LDFLAGS_REL := -flto=thin -fuse-ld=lld -Wl,--gc-sections -Wl,-O2 -Wl,--strip-all

# ── Minimal Flags (Super-optimized for speed + ZSMINIMAL) ───
CFLAGS_MIN  := -O3 -march=native -mtune=native \
               -flto=thin -fomit-frame-pointer -funroll-loops -fno-plt \
               -ffunction-sections -fdata-sections \
               -fmerge-all-constants -fno-semantic-interposition \
               -fno-math-errno -fno-trapping-math \
               -fstrict-aliasing -fvectorize -fslp-vectorize \
               -pipe -DNDEBUG -DZSMINIMAL -DMG_ENABLE_LOG=0
LDFLAGS_MIN := -flto=thin -fuse-ld=lld -Wl,--gc-sections -Wl,-O2 -Wl,--strip-all

# Features to disable in minimal mode (space-separated)
MINIMAL_DISABLE_FEATURES := sqlite

# ============================================================
#  Targets
# ============================================================

.PHONY: all release release-install minimal debug clean run install uninstall amalgamate copy_assets

all: debug

$(DIST_DIR):
	mkdir -p $(DIST_DIR)

copy_assets: | $(DIST_DIR)
	@cp -rn lib $(DIST_DIR)/ 2>/dev/null || true
	@cp -rn tests $(DIST_DIR)/ 2>/dev/null || true

# SQLite shared library build rule
$(SQLITE_LIB): sqlite/sqlite3.c | $(DIST_DIR)
	@echo "Building SQLite shared library..."
	$(CC) -fPIC -shared -O2 -o $@ $<

release: $(SQLITE_LIB) copy_assets | $(DIST_DIR)
	@echo "Building in release mode (clang, super-optimized, sqlite dynamic)..."
	$(CC) $(CFLAGS_BASE) $(CFLAGS_REL) $(SRCS) -o $(TARGET) $(LDFLAGS) $(LDFLAGS_REL) -L$(DIST_DIR) -lsqlite3 $(RPATH_FLAG)
	@echo "Build successful → $(TARGET)"

# Override RPATH for installation so the binary knows to look in $(LIBDIR) at runtime
release-install: RPATH_FLAG := -Wl,-rpath,'$(LIBDIR)'
release-install: $(SQLITE_LIB) copy_assets | $(DIST_DIR)
	@echo "Building in release mode (install RPATH, sqlite dynamic)..."
	$(CC) $(CFLAGS_BASE) $(CFLAGS_REL) $(SRCS) -o $(TARGET) $(LDFLAGS) $(LDFLAGS_REL) -L$(DIST_DIR) -lsqlite3 $(RPATH_FLAG)
	@echo "Build successful → $(TARGET)"

minimal: copy_assets | $(DIST_DIR)
	@echo "Building in minimal mode (super-optimized speed, sqlite completely disabled, ZSMINIMAL defined)..."
	$(CC) $(CFLAGS_BASE) $(CFLAGS_MIN) $(SRCS) -o $(TARGET) $(LDFLAGS) $(LDFLAGS_MIN)
	@echo "Build successful → $(TARGET)"

debug: $(SQLITE_LIB) copy_assets | $(DIST_DIR)
	@echo "Building in debug mode (sqlite dynamic)..."
	$(CC) $(CFLAGS_BASE) $(CFLAGS_DBG) $(SRCS) -o $(TARGET) $(LDFLAGS) -L$(DIST_DIR) -lsqlite3 $(RPATH_FLAG)
	@echo "Build successful → $(TARGET)"

clean:
	rm -rf $(DIST_DIR)

install: release-install
	@echo "Installing $(TARGET) → $(BINDIR)/zscript"
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/zscript
	install -d $(LIBDIR)
	@echo "Installing lib → $(LIBDIR)/lib/"
	cd lib && find . -type d -exec install -d $(LIBDIR)/lib/{} \; \
           && find . -type f -exec install -m 644 {} $(LIBDIR)/lib/{} \;
	@echo "Installing tests → $(LIBDIR)/tests/"
	cd tests && find . -type d -exec install -d $(LIBDIR)/tests/{} \; \
             && find . -type f -exec install -m 644 {} $(LIBDIR)/tests/{} \;
	@if [ "$(call feature_enabled,sqlite)" != "0" ]; then \
		echo "Installing sqlite/libsqlite3.so → $(LIBDIR)/libsqlite3.so"; \
		install -m 755 $(SQLITE_LIB) $(LIBDIR)/libsqlite3.so; \
	fi

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