#!/usr/bin/env bash
# Build and run zscript using the same rules as the Makefile (see Makefile targets).
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

# clear exits non-zero without a TTY; do not trip `set -e`
if [[ -t 1 ]]; then clear; fi

if [[ "${1:-}" == "--format" ]]; then
	echo "Running clang-format..."
	find src -type f \( -name '*.c' -o -name '*.h' \) -print0 | xargs -0 clang-format -i
	clang-format -i main.c
	echo "Formatting complete."
	exit 0
fi

case "${1:-}" in
--release)
	make release
	exit 0
	;;
--compile)
	make debug
	exit 0
	;;
--dbg)
	make debug
	gdb -ex run -ex bt --args "$ROOT/dist/zscript.exe" --run "${2:-}"
	;;
*)
	make run
	;;
esac
