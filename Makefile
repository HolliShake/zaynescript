# ============================================================
#  Makefile for zscript
# ============================================================

# ── Toolchain ───────────────────────────────────────────────
# Forced to clang to prevent GCC cc1 errors with -flto=thin
CC  := clang
LLD := lld

# ── Output ──────────────────────────────────────────────────
DIST_DIR   := dist
TARGET     := $(DIST_DIR)/zscript.exe
SQLITE_LIB := $(DIST_DIR)/libsqlite3.so
MARIADB_LIB := $(DIST_DIR)/libmariadb.so
THIRDPARTY_DIR := thirdparty
SQLITE_SRC := $(THIRDPARTY_DIR)/sqlite/sqlite3.c

# ── MariaDB connector build dir ─────────────────────────────
MARIADB_SRC       := $(THIRDPARTY_DIR)/mariadb-connector-c
MARIADB_BUILD_DIR := $(MARIADB_SRC)/build-zscript

# ── Install paths ────────────────────────────────────────────
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib/zscript

# ── Sources ──────────────────────────────────────────────────
# libbf/cutils.c omitted: one shared cutils from libregex (see libbf/cutils.h).
ALL_SRCS := \
	main.c \
	$(wildcard src/*.c) \
	$(wildcard src/core/*.c) \
	$(wildcard utf/*.c) \
	$(wildcard utf/utf8proc/*.c) \
	$(filter-out $(THIRDPARTY_DIR)/libbf/cutils.c, $(wildcard $(THIRDPARTY_DIR)/libbf/*.c)) \
	$(wildcard $(THIRDPARTY_DIR)/libregex/*.c) \
	$(wildcard $(THIRDPARTY_DIR)/mongoose/*.c)

# sqlite/ is excluded for all builds — linked dynamically via dist/libsqlite3.so
SRCS := $(filter-out $(THIRDPARTY_DIR)/sqlite/%, $(ALL_SRCS))

# ── Build metadata ───────────────────────────────────────────
BUILD_DATE := $(shell date '+%Y-%m-%d %H:%M:%S')

# ── Shared flags (all targets) ───────────────────────────────
CFLAGS_COMMON := \
	-Wno-pointer-sign \
	-DBUILD_DATE='"$(BUILD_DATE)"' \
	-I$(MARIADB_SRC)/include

LDFLAGS_COMMON := \
	-lm -ldl -lpthread \
	-L$(DIST_DIR) -lsqlite3 -lmariadb

# RPATH: $ORIGIN → look beside the binary at runtime (dev/release builds)
RPATH_ORIGIN  := -Wl,-rpath,'$$ORIGIN'
# RPATH: fixed install path (used by release-install / install)
RPATH_INSTALL := -Wl,-rpath,'$(LIBDIR)'

# ── Debug flags ──────────────────────────────────────────────
CFLAGS_DEBUG := \
	-g3 -O0 \
	-fsanitize=address,leak \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls

# ── Release flags ────────────────────────────────────────────
CFLAGS_RELEASE := \
	-O3 -march=native -mtune=native \
	-flto=thin \
	-fomit-frame-pointer \
	-funroll-loops \
	-fno-plt \
	-ffunction-sections -fdata-sections \
	-fmerge-all-constants \
	-fno-semantic-interposition \
	-fno-math-errno -fno-trapping-math \
	-fstrict-aliasing \
	-fvectorize -fslp-vectorize \
	-pipe \
	-DNDEBUG \
	-DMG_ENABLE_LOG=0

LDFLAGS_RELEASE := \
	-flto=thin \
	-fuse-ld=$(LLD) \
	-Wl,--gc-sections \
	-Wl,-O2 \
	-Wl,--strip-all

# ============================================================
#  Phony targets
# ============================================================
.PHONY: all debug release release-install install uninstall clean run amalgamate 64 32 32i386 win32 win32-64 win32-32 win32-32i386 copy_assets_win32 check-native-tools check-win32-tools

# ── Architecture selection ────────────────────────────────────
# BUILD_ARCH options:
#   auto   -> detect from current machine (default)
#   64     -> x86_64 64-bit
#   32     -> x32 ABI on x86_64 toolchains (-mx32)
#   32i386 -> 32-bit i386/generic 32-bit (-m32)
HOST_ARCH  := $(shell uname -m)
BUILD_ARCH ?= auto

ifeq ($(BUILD_ARCH),auto)
  ifeq ($(HOST_ARCH),x86_64)
    ARCH_CFLAGS := -m64
    ARCH_NAME   := 64-bit x86_64 (auto)
  else
    ARCH_CFLAGS :=
    ARCH_NAME   := native ($(HOST_ARCH), auto)
  endif
else ifeq ($(BUILD_ARCH),64)
  ARCH_CFLAGS := -m64
  ARCH_NAME   := 64-bit x86_64
else ifeq ($(BUILD_ARCH),32)
  ARCH_CFLAGS := -mx32
  ARCH_NAME   := 32-bit x86_64 (x32 ABI)
else ifeq ($(BUILD_ARCH),32i386)
  ARCH_CFLAGS := -m32 -march=i386
  ARCH_NAME   := 32-bit i386/generic
else
  $(error Unsupported BUILD_ARCH='$(BUILD_ARCH)'. Use auto, 64, 32, or 32i386)
endif

all: debug

# Convenience arch targets
64:
	@$(MAKE) BUILD_ARCH=64 debug

32:
	@$(MAKE) BUILD_ARCH=32 debug

32i386:
	@$(MAKE) BUILD_ARCH=32i386 debug

# Windows (MinGW) convenience arch targets
win32-64:
	@$(MAKE) BUILD_ARCH=64 win32

win32-32:
	@$(MAKE) BUILD_ARCH=32 win32

win32-32i386:
	@$(MAKE) BUILD_ARCH=32i386 win32

# ── Windows cross-compile selection (MinGW) ──────────────────
# NOTE: x32 ABI (-mx32) is not used for MinGW; BUILD_ARCH=32 maps to i686-w64-mingw32.
WIN32_DIR := win32
WIN32_ARCH_DIR := $(WIN32_DIR)/$(BUILD_ARCH)
WIN32_TARGET := $(WIN32_ARCH_DIR)/zscript.exe
WIN32_SQLITE_DLL := $(WIN32_ARCH_DIR)/sqlite3.dll
WIN32_MARIADB_DLL := $(WIN32_ARCH_DIR)/libmariadb.dll

ifeq ($(BUILD_ARCH),auto)
  ifeq ($(HOST_ARCH),x86_64)
    WIN_MINGW_PREFIX := x86_64-w64-mingw32
    WIN_ARCH_CFLAGS  := -m64
    WIN_ARCH_NAME    := win64 (auto)
  else
    WIN_MINGW_PREFIX := i686-w64-mingw32
    WIN_ARCH_CFLAGS  := -m32 -march=i386
    WIN_ARCH_NAME    := win32 i386 (auto)
  endif
else ifeq ($(BUILD_ARCH),64)
  WIN_MINGW_PREFIX := x86_64-w64-mingw32
  WIN_ARCH_CFLAGS  := -m64
  WIN_ARCH_NAME    := win64
else ifeq ($(BUILD_ARCH),32)
  WIN_MINGW_PREFIX := i686-w64-mingw32
  WIN_ARCH_CFLAGS  := -m32
  WIN_ARCH_NAME    := win32 (i686)
else ifeq ($(BUILD_ARCH),32i386)
  WIN_MINGW_PREFIX := i686-w64-mingw32
  WIN_ARCH_CFLAGS  := -m32 -march=i386
  WIN_ARCH_NAME    := win32 (i386)
endif

WINCC := $(WIN_MINGW_PREFIX)-gcc
WIN_CFLAGS_COMMON := -DWIN32 -D_WINDOWS -I$(MARIADB_SRC)/include
WIN_CFLAGS_RELEASE := -O2 -pipe -DNDEBUG -DMG_ENABLE_LOG=0
WIN_LDFLAGS_COMMON := -L$(WIN32_ARCH_DIR) -lsqlite3 -lmariadb -lws2_32 -lcrypt32 -lbcrypt -liphlpapi

# ============================================================
#  Tool checks
# ============================================================
check-native-tools:
	@command -v $(CC) >/dev/null 2>&1 || { \
		echo "Error: $(CC) is required but not installed."; \
		echo "Install it with your package manager (example: sudo pacman -S clang)."; \
		exit 1; \
	}
	@command -v $(LLD) >/dev/null 2>&1 || { \
		echo "Error: $(LLD) is required but not installed."; \
		echo "Install it with your package manager (example: sudo pacman -S lld)."; \
		exit 1; \
	}
	@command -v cmake >/dev/null 2>&1 || { \
		echo "Error: cmake is required but not installed."; \
		echo "Install it with your package manager (example: sudo pacman -S cmake)."; \
		exit 1; \
	}

check-win32-tools:
	@command -v $(WINCC) >/dev/null 2>&1 || { \
		echo "Error: $(WINCC) is required for Win32 cross-compile but not installed."; \
		echo "Install MinGW-w64 (example: sudo pacman -S mingw-w64-gcc)."; \
		exit 1; \
	}

# ============================================================
#  Shared libraries
# ============================================================

$(DIST_DIR):
	mkdir -p $@

$(WIN32_ARCH_DIR):
	mkdir -p $@

# ── SQLite ───────────────────────────────────────────────────
$(SQLITE_LIB): $(SQLITE_SRC) | $(DIST_DIR)
	@echo "[sqlite] Building shared library..."
	$(CC) $(ARCH_CFLAGS) -fPIC -shared -O2 -o $@ $<
	@echo "[sqlite] → $@"

# ── MariaDB ──────────────────────────────────────────────────
$(MARIADB_LIB): $(MARIADB_SRC)/CMakeLists.txt | $(DIST_DIR)
	@echo "[mariadb] Building shared library..."
	@command -v cmake >/dev/null 2>&1 || { \
		echo "Error: cmake is required to build mariadb-connector-c."; \
		echo "       Install cmake, or place a pre-built libmariadb.so in $(DIST_DIR)."; \
		exit 1; \
	}
	cmake -S $(MARIADB_SRC) -B $(MARIADB_BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_COMPILE_WARNING_AS_ERROR=OFF \
		-DCMAKE_C_FLAGS="$(ARCH_CFLAGS)"
	cmake --build $(MARIADB_BUILD_DIR) --parallel
	@for candidate in \
		"$(MARIADB_BUILD_DIR)/libmariadb/libmariadb.so" \
		"/usr/lib/libmariadb.so" \
		"/usr/lib64/libmariadb.so" \
		"/usr/lib/x86_64-linux-gnu/libmariadb.so"; do \
		if [ -f "$$candidate" ]; then \
			cp -f "$$candidate" "$@"; \
			echo "[mariadb] → $@ (from $$candidate)"; \
			rm -rf "$(MARIADB_BUILD_DIR)"; \
			exit 0; \
		fi; \
	done; \
	echo "Error: could not locate libmariadb.so. Install MariaDB Connector/C or cmake."; \
	exit 1

# ============================================================
#  Asset copy
# ============================================================
.PHONY: copy_assets
copy_assets: | $(DIST_DIR)
	@cp -rf lib   $(DIST_DIR)/ 2>/dev/null || true
	@cp -rf tests $(DIST_DIR)/ 2>/dev/null || true

copy_assets_win32: | $(WIN32_ARCH_DIR)
	@cp -rf lib   $(WIN32_ARCH_DIR)/ 2>/dev/null || true
	@cp -rf tests $(WIN32_ARCH_DIR)/ 2>/dev/null || true

# ============================================================
#  Build targets
# ============================================================

# ── Debug ────────────────────────────────────────────────────
debug: check-native-tools $(SQLITE_LIB) $(MARIADB_LIB) copy_assets | $(DIST_DIR)
	@echo "[zscript] Building debug ($(ARCH_NAME))..."
	$(CC) $(ARCH_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_DEBUG) $(SRCS) \
	    -o $(TARGET) \
	    $(LDFLAGS_COMMON) \
	    $(RPATH_ORIGIN)
	@rm -rf "$(MARIADB_BUILD_DIR)"
	@echo "[zscript] Debug build → $(TARGET)"

# ── Release (run from dist/, RPATH = $ORIGIN) ────────────────
release: check-native-tools $(SQLITE_LIB) $(MARIADB_LIB) copy_assets | $(DIST_DIR)
	@echo "[zscript] Building release ($(ARCH_NAME))..."
	$(CC) $(ARCH_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_RELEASE) $(SRCS) \
	    -o $(TARGET) \
	    $(LDFLAGS_COMMON) $(LDFLAGS_RELEASE) \
	    $(RPATH_ORIGIN)
	@rm -rf "$(MARIADB_BUILD_DIR)"
	@echo "[zscript] Release build → $(TARGET)"

# ── Release-install (RPATH baked to $(LIBDIR)) ───────────────
release-install: check-native-tools $(SQLITE_LIB) $(MARIADB_LIB) copy_assets | $(DIST_DIR)
	@echo "[zscript] Building release ($(ARCH_NAME), install RPATH=$(LIBDIR))..."
	$(CC) $(ARCH_CFLAGS) $(CFLAGS_COMMON) $(CFLAGS_RELEASE) $(SRCS) \
	    -o $(TARGET) \
	    $(LDFLAGS_COMMON) $(LDFLAGS_RELEASE) \
	    $(RPATH_INSTALL)
	@rm -rf "$(MARIADB_BUILD_DIR)"
	@echo "[zscript] Release-install build → $(TARGET)"

# ── Win32 cross-compile (MinGW) ──────────────────────────────
$(WIN32_SQLITE_DLL): $(SQLITE_SRC) | $(WIN32_ARCH_DIR)
	@echo "[sqlite] Building Windows DLL ($(WIN_ARCH_NAME))..."
	@command -v $(WINCC) >/dev/null 2>&1 || { \
		echo "Error: $(WINCC) not found. Install mingw-w64 toolchain."; \
		exit 1; \
	}
	$(WINCC) $(WIN_ARCH_CFLAGS) -shared -O2 -o $@ $<
	@echo "[sqlite] → $@"

$(WIN32_MARIADB_DLL): | $(WIN32_ARCH_DIR)
	@echo "[mariadb] Copying Windows DLL ($(WIN_ARCH_NAME))..."
	@for candidate in \
		"/usr/$(WIN_MINGW_PREFIX)/bin/libmariadb.dll" \
		"/usr/lib/$(WIN_MINGW_PREFIX)/libmariadb.dll" \
		"/usr/$(WIN_MINGW_PREFIX)/lib/libmariadb.dll"; do \
		if [ -f "$$candidate" ]; then \
			cp -f "$$candidate" "$@"; \
			echo "[mariadb] → $@ (from $$candidate)"; \
			exit 0; \
		fi; \
	done; \
	echo "Error: could not locate Windows libmariadb.dll for $(WIN_MINGW_PREFIX)."; \
	echo "       Install mingw-w64 mariadb connector package."; \
	exit 1

win32: check-win32-tools $(WIN32_SQLITE_DLL) $(WIN32_MARIADB_DLL) copy_assets_win32 | $(WIN32_ARCH_DIR)
	@echo "[zscript] Cross-compiling Windows build ($(WIN_ARCH_NAME))..."
	@command -v $(WINCC) >/dev/null 2>&1 || { \
		echo "Error: $(WINCC) not found. Install mingw-w64 toolchain."; \
		exit 1; \
	}
	$(WINCC) $(WIN_ARCH_CFLAGS) $(WIN_CFLAGS_COMMON) $(WIN_CFLAGS_RELEASE) $(SRCS) \
	    -o $(WIN32_TARGET) \
	    $(WIN_LDFLAGS_COMMON)
	@echo "[zscript] Windows build → $(WIN32_TARGET)"

# ============================================================
#  Install / Uninstall
# ============================================================

install: release-install
	@echo "[install] $(TARGET) → $(BINDIR)/zscript"
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/zscript

	@echo "[install] lib/ → $(LIBDIR)/lib/"
	install -d $(LIBDIR)/lib
	cd lib && find . -type d -exec install -d $(LIBDIR)/lib/{} \; \
	       && find . -type f -exec install -m 644 {} $(LIBDIR)/lib/{} \;

	@echo "[install] tests/ → $(LIBDIR)/tests/"
	install -d $(LIBDIR)/tests
	cd tests && find . -type d -exec install -d $(LIBDIR)/tests/{} \; \
	         && find . -type f -exec install -m 644 {} $(LIBDIR)/tests/{} \;

	@echo "[install] $(SQLITE_LIB) → $(LIBDIR)/libsqlite3.so"
	install -m 755 $(SQLITE_LIB) $(LIBDIR)/libsqlite3.so

	@echo "[install] $(MARIADB_LIB) → $(LIBDIR)/libmariadb.so"
	install -m 755 $(MARIADB_LIB) $(LIBDIR)/libmariadb.so

	@echo "[install] Done. Run: zscript"

uninstall:
	@echo "[uninstall] Removing $(BINDIR)/zscript"
	rm -f $(BINDIR)/zscript
	@echo "[uninstall] Removing $(LIBDIR)/"
	rm -rf $(LIBDIR)

# ============================================================
#  Utility
# ============================================================

clean:
	rm -rf $(DIST_DIR) $(WIN32_DIR) $(MARIADB_BUILD_DIR)

run: debug
	ASAN_OPTIONS=fast_unwind_on_malloc=0:malloc_context_size=30 \
	LC_ALL=en_US.UTF-8 \
	./$(TARGET)

amalgamate:
	@echo "[amalgamate] Running amalgamation..."
	python3 amalgamate.py
	@echo "[amalgamate] Done. Output in dist/"