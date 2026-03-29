#!/usr/bin/env python3
"""
Amalgamation script for ZayneScript.
Generates a single zscript.h and zscript.c in the dist/ folder
from all src/ headers/sources and main.c.
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.join(ROOT, "src")
CORE_DIR = os.path.join(SRC_DIR, "core")
DIST_DIR = os.path.join(ROOT, "dist")
MAIN_C = os.path.join(ROOT, "main.c")

# External dependency headers (libbf, utf) — ordered so that
# each file only depends on things already emitted above it.
EXT_HEADER_ORDER = [
    # libbf
    "libbf/cutils.h",
    "libbf/libbf.h",
    # utf8proc (low-level)
    "utf/utf8proc/utf8proc.h",
    # utf wrapper
    "utf/utf.h",
]

# External dependency sources.
EXT_SOURCE_ORDER = [
    "libbf/cutils.c",
    "libbf/libbf.c",
    "utf/utf8proc/utf8proc_data.h",
    "utf/utf8proc/utf8proc.c",
    "utf/utf.c",
]

# Header files in topological (dependency) order.
# Each path is relative to ROOT.
HEADER_ORDER = [
    # Layer 1: no internal dependencies
    "src/opcode.h",
    "src/keyword.h",
    "src/global.h",
    # Layer 2: depend on global.h only
    "src/position.h",
    "src/error.h",
    "src/array.h",
    "src/astnode.h",
    "src/hashmap.h",
    "src/environment.h",
    "src/function.h",
    "src/statemachine.h",
    # Layer 3: depend on layers 1-2
    "src/scope.h",
    "src/class.h",
    "src/value.h",
    "src/lexer.h",
    "src/decompiler.h",
    "src/gc.h",
    # Layer 4: core module headers
    "src/core/array.h",
    "src/core/date.h",
    "src/core/io.h",
    "src/core/math.h",
    "src/core/os.h",
    "src/core/promise.h",
    "src/core/loader.h",
    # Layer 5: high-level headers
    "src/operation.h",
    "src/parser.h",
    "src/compiler.h",
    "src/interpreter.h",
]

# Source files in a sensible compilation order (matching header order,
# then main.c last).
SOURCE_ORDER = [
    "src/global.c",
    "src/position.c",
    "src/array.c",
    "src/astnode.c",
    "src/hashmap.c",
    "src/environment.c",
    "src/function.c",
    "src/statemachine.c",
    "src/scope.c",
    "src/class.c",
    "src/value.c",
    "src/lexer.c",
    "src/decompiler.c",
    "src/gc.c",
    "src/core/array.c",
    "src/core/date.c",
    "src/core/io.c",
    "src/core/math.c",
    "src/core/os.c",
    "src/core/promise.c",
    "src/core/loader.c",
    "src/operation.c",
    "src/parser.c",
    "src/compiler.c",
    "src/interpreter.c",
    "main.c",
]


# Detect any quoted #include (project-local headers use "...", system headers use <...>)
def is_project_include(line):
    """Return True if the line is a quoted #include of any project header
    (internal src/ OR external libbf/, utf/, etc.).  These are all embedded
    in the amalgamated output so must be stripped."""
    m = re.match(r'^\s*#\s*include\s+"([^"]+)"', line)
    if not m:
        return False
    # All quoted includes are project-local; system headers use <...>
    return True


def read_file(rel_path):
    """Read a file relative to ROOT and return its contents."""
    full = os.path.join(ROOT, rel_path)
    with open(full, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def strip_project_includes(text):
    """Remove #include lines that reference any project header (internal or
    external deps like libbf/utf).  System <...> includes are kept."""
    lines = text.split("\n")
    out = []
    for line in lines:
        if is_project_include(line):
            continue
        out.append(line)
    return "\n".join(out)


def strip_header_guards(text, guard_macro):
    """Strip the #ifndef/#define guard pair and matching closing #endif.

    Uses #if*/#endif nesting to find the correct closing #endif.
    """
    lines = text.split("\n")

    # Find and remove the #ifndef GUARD / #define GUARD pair
    guard_line = None
    define_line = None
    for i, line in enumerate(lines):
        stripped = line.strip()
        if guard_line is None and re.match(
            r"#\s*ifndef\s+" + re.escape(guard_macro) + r"\s*$", stripped
        ):
            guard_line = i
        elif (
            guard_line is not None
            and define_line is None
            and re.match(r"#\s*define\s+" + re.escape(guard_macro) + r"\s*$", stripped)
        ):
            define_line = i
            break

    if guard_line is None or define_line is None:
        return text

    # Find the matching #endif by tracking nesting from guard_line
    depth = 1
    endif_line = None
    for i in range(define_line + 1, len(lines)):
        stripped = lines[i].strip()
        if re.match(r"#\s*(?:if|ifdef|ifndef)\b", stripped):
            depth += 1
        elif re.match(r"#\s*endif\b", stripped):
            depth -= 1
            if depth == 0:
                endif_line = i
                break

    if endif_line is None:
        return text

    # Remove the guard open lines and the closing #endif
    result = (
        lines[:guard_line]
        + lines[define_line + 1 : endif_line]
        + lines[endif_line + 1 :]
    )
    return "\n".join(result)


def detect_guard_macro(text):
    """Try to detect the header guard macro from the file content."""
    m = re.search(r"^\s*#\s*ifndef\s+(\w+)\s*$", text, re.MULTILINE)
    if m:
        candidate = m.group(1)
        # Verify there's a matching #define right after
        if re.search(r"#\s*define\s+" + re.escape(candidate), text):
            return candidate
    return None


def _pp_depth_change(stripped):
    """Return +1 for #if/#ifdef/#ifndef, -1 for #endif, else 0."""
    if re.match(r"#\s*(?:if|ifdef|ifndef)\b", stripped):
        return 1
    if re.match(r"#\s*endif\b", stripped):
        return -1
    return 0


# Preferred ordering for system headers. Headers listed here will be
# emitted first (in this order); any remaining headers discovered from
# the source files will follow in the order they were encountered.
_SYSTEM_HEADER_PRIORITY = [
    "<stddef.h>",
    "<stdint.h>",
    "<stdbool.h>",
    "<inttypes.h>",
    "<limits.h>",
    "<stdarg.h>",
    "<stdio.h>",
    "<stdlib.h>",
    "<string.h>",
    "<math.h>",
    "<ctype.h>",
    "<errno.h>",
    "<time.h>",
    "<unistd.h>",
    "<dirent.h>",
]


def collect_system_includes_from_text(text):
    """Return a list of unconditional (depth==0) system include headers
    found in already-processed text."""
    found = []
    depth = 0
    for line in text.split("\n"):
        stripped = line.strip()
        depth += _pp_depth_change(stripped)
        if depth == 0:
            m = re.match(r"^\s*#\s*include\s+(<[^>]+>)", line)
            if m:
                found.append(m.group(1))
    return found


def collect_all_system_includes(file_list, is_header=False):
    """Process files (strip guards for headers, strip project includes),
    then collect and return de-duplicated, priority-ordered system includes."""
    seen = set()
    discovered = []
    for rel_path in file_list:
        content = read_file(rel_path)
        if is_header:
            guard = detect_guard_macro(content)
            if guard:
                content = strip_header_guards(content, guard)
        content = strip_project_includes(content)
        for header in collect_system_includes_from_text(content):
            if header not in seen:
                seen.add(header)
                discovered.append(header)

    # Sort: priority headers first, then the rest in discovery order
    priority_set = set(_SYSTEM_HEADER_PRIORITY)
    result = [f"#include {h}" for h in _SYSTEM_HEADER_PRIORITY if h in seen]
    result += [f"#include {h}" for h in discovered if h not in priority_set]
    return result


def strip_system_includes(text):
    """Remove only *unconditional* #include <...> lines (preprocessor
    depth == 0).  Conditional includes inside #ifdef blocks are kept."""
    lines = text.split("\n")
    out = []
    depth = 0
    for line in lines:
        stripped = line.strip()
        depth += _pp_depth_change(stripped)
        if depth == 0 and re.match(r"^\s*#\s*include\s+<[^>]+>", line):
            continue
        out.append(line)
    return "\n".join(out)


def emit_header(rel_path):
    """Read a header, strip its guard, project includes, and system includes."""
    content = read_file(rel_path)
    guard = detect_guard_macro(content)
    if guard:
        content = strip_header_guards(content, guard)
    content = strip_project_includes(content)
    content = strip_system_includes(content)
    return content.strip()


def build_header(out_path):
    """Build the amalgamated header file."""
    parts = []
    parts.append("/*")
    parts.append(" * zscript.h - ZayneScript amalgamated header")
    parts.append(" * Auto-generated by amalgamate.py - DO NOT EDIT")
    parts.append(" */")
    parts.append("")
    parts.append("#ifndef ZSCRIPT_H")
    parts.append("#define ZSCRIPT_H")
    parts.append("")

    # Collect and emit all system includes once at the top
    all_headers = EXT_HEADER_ORDER + HEADER_ORDER
    sys_includes = collect_all_system_includes(all_headers, is_header=True)
    parts.append("/* ---- system includes ---- */")
    parts.extend(sys_includes)
    parts.append("")

    # External dependency headers first (libbf, utf)
    for rel_path in EXT_HEADER_ORDER:
        content = emit_header(rel_path)
        if not content:
            continue
        parts.append(f"/* ---- {rel_path} ---- */")
        parts.append(content)
        parts.append("")

    # Project headers
    for rel_path in HEADER_ORDER:
        content = emit_header(rel_path)
        if not content:
            continue
        parts.append(f"/* ---- {rel_path} ---- */")
        parts.append(content)
        parts.append("")

    parts.append("#endif /* ZSCRIPT_H */")
    parts.append("")

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(parts))

    print(f"  Created {out_path}")


def emit_source(rel_path):
    """Read a source file, strip project and system includes, return content."""
    content = read_file(rel_path)
    content = strip_project_includes(content)
    content = strip_system_includes(content)
    return content.strip()


def build_source(out_path):
    """Build the amalgamated source file."""
    parts = []
    parts.append("/*")
    parts.append(" * zscript.c - ZayneScript amalgamated source")
    parts.append(" * Auto-generated by amalgamate.py - DO NOT EDIT")
    parts.append(" */")
    parts.append("")
    parts.append('#include "zscript.h"')
    parts.append("")

    # Collect and emit system includes used only by .c files (not already in .h)
    all_headers = EXT_HEADER_ORDER + HEADER_ORDER
    header_sys = set()
    for inc in collect_all_system_includes(all_headers, is_header=True):
        m = re.match(r"#include\s+(<[^>]+>)", inc)
        if m:
            header_sys.add(m.group(1))
    all_sources = EXT_SOURCE_ORDER + SOURCE_ORDER
    src_sys = collect_all_system_includes(all_sources, is_header=False)
    extra = [
        inc
        for inc in src_sys
        if re.match(r"#include\s+(<[^>]+>)", inc)
        and re.match(r"#include\s+(<[^>]+>)", inc).group(1) not in header_sys
    ]
    if extra:
        parts.append("/* ---- additional system includes (from .c files) ---- */")
        parts.extend(extra)
        parts.append("")

    # External dependency sources first (libbf, utf)
    for rel_path in EXT_SOURCE_ORDER:
        content = emit_source(rel_path)
        if not content:
            continue
        parts.append(f"/* ---- {rel_path} ---- */")
        parts.append(content)
        parts.append("")

        # libbf.c #defines malloc/free/realloc as forbidden — undo that
        # so subsequent code (utf8proc, etc.) can use standard allocators.
        if rel_path == "libbf/libbf.c":
            parts.append("/* undo libbf.c malloc/free/realloc poison */")
            parts.append("#undef malloc")
            parts.append("#undef free")
            parts.append("#undef realloc")
            parts.append("")

    # Project sources
    for rel_path in SOURCE_ORDER:
        content = emit_source(rel_path)
        if not content:
            continue
        parts.append(f"/* ---- {rel_path} ---- */")
        parts.append(content)
        parts.append("")

    parts.append("")

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(parts))

    print(f"  Created {out_path}")


def main():
    # Verify files exist
    missing = []
    all_files = EXT_HEADER_ORDER + EXT_SOURCE_ORDER + HEADER_ORDER + SOURCE_ORDER
    for rel_path in all_files:
        full = os.path.join(ROOT, rel_path)
        if not os.path.isfile(full):
            missing.append(rel_path)
    if missing:
        print("Error: missing files:", file=sys.stderr)
        for m in missing:
            print(f"  {m}", file=sys.stderr)
        sys.exit(1)

    os.makedirs(DIST_DIR, exist_ok=True)

    print("Amalgamating ZayneScript...")
    build_header(os.path.join(DIST_DIR, "zscript.h"))
    build_source(os.path.join(DIST_DIR, "zscript.c"))
    print("Done!")


if __name__ == "__main__":
    main()
