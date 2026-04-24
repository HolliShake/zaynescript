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
.PHONY: all debug release release-install install uninstall clean run amalgamate

all: debug

# ============================================================
#  Shared libraries
# ============================================================

$(DIST_DIR):
	mkdir -p $@

# ── SQLite ───────────────────────────────────────────────────
$(SQLITE_LIB): $(SQLITE_SRC) | $(DIST_DIR)
	@echo "[sqlite] Building shared library..."
	$(CC) -fPIC -shared -O2 -o $@ $<
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
		-DCMAKE_COMPILE_WARNING_AS_ERROR=OFF
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

# ============================================================
#  Build targets
# ============================================================

# ── Debug ────────────────────────────────────────────────────
debug: $(SQLITE_LIB) $(MARIADB_LIB) copy_assets | $(DIST_DIR)
	@echo "[zscript] Building debug..."
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_DEBUG) $(SRCS) \
	    -o $(TARGET) \
	    $(LDFLAGS_COMMON) \
	    $(RPATH_ORIGIN)
	@rm -rf "$(MARIADB_BUILD_DIR)"
	@echo "[zscript] Debug build → $(TARGET)"

# ── Release (run from dist/, RPATH = $ORIGIN) ────────────────
release: $(SQLITE_LIB) $(MARIADB_LIB) copy_assets | $(DIST_DIR)
	@echo "[zscript] Building release..."
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_RELEASE) $(SRCS) \
	    -o $(TARGET) \
	    $(LDFLAGS_COMMON) $(LDFLAGS_RELEASE) \
	    $(RPATH_ORIGIN)
	@rm -rf "$(MARIADB_BUILD_DIR)"
	@echo "[zscript] Release build → $(TARGET)"

# ── Release-install (RPATH baked to $(LIBDIR)) ───────────────
release-install: $(SQLITE_LIB) $(MARIADB_LIB) copy_assets | $(DIST_DIR)
	@echo "[zscript] Building release (install RPATH=$(LIBDIR))..."
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_RELEASE) $(SRCS) \
	    -o $(TARGET) \
	    $(LDFLAGS_COMMON) $(LDFLAGS_RELEASE) \
	    $(RPATH_INSTALL)
	@rm -rf "$(MARIADB_BUILD_DIR)"
	@echo "[zscript] Release-install build → $(TARGET)"

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
	rm -rf $(DIST_DIR) $(MARIADB_BUILD_DIR)

run: debug
	ASAN_OPTIONS=fast_unwind_on_malloc=0:malloc_context_size=30 \
	LC_ALL=en_US.UTF-8 \
	./$(TARGET)

amalgamate:
	@echo "[amalgamate] Running amalgamation..."
	python3 amalgamate.py
	@echo "[amalgamate] Done. Output in dist/"