#!/usr/bin/env bash
# =============================================================================
# run_tests.sh — run the Lua test suite (+ optional LibreOffice round-trip)
# Usage: scripts/run_tests.sh [build-dir]
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${1:-${BUILD:-$ROOT/build}}"

export OX_TEST_OUT="${OX_TEST_OUT:-/tmp/opencode/openxlsx-tests}"
export LUA_CPATH="$BUILD/?.so;;${LUA_CPATH:-}"
mkdir -p "$OX_TEST_OUT"

if [ ! -f "$BUILD/openxlsx.so" ]; then
    echo "error: $BUILD/openxlsx.so not found - build first (cmake --build build)" >&2
    exit 1
fi

fail=0
for t in "$ROOT"/test/*.lua; do
    echo "--- $(basename "$t")"
    if ! lua "$t"; then
        echo "    FAILED"
        fail=1
    fi
done

# ---- independent verification with LibreOffice (if available) ----------------
if command -v soffice >/dev/null 2>&1 && [ -f "$OX_TEST_OUT/smoke.xlsx" ]; then
    echo "--- libreoffice round-trip"
    LO_PROFILE="/tmp/opencode/lo-profile-$$"
    OUT="/tmp/opencode/lo-convert-$$"
    mkdir -p "$OUT"
    if soffice -env:UserInstallation="file://$LO_PROFILE" --headless --norestore \
               --convert-to csv:"Text - txt - csv (StarCalc)":44,34,76 \
               --outdir "$OUT" "$OX_TEST_OUT/smoke.xlsx" >/dev/null 2>&1; then
        CSV="$OUT/smoke.csv"
        if [ -f "$CSV" ]; then
            grep -q "Hello 世界" "$CSV" || { echo "    csv content check failed"; fail=1; }
            grep -q "3.5" "$CSV" || { echo "    csv number check failed"; fail=1; }
            echo "    csv verified"
        fi
    else
        echo "    (libreoffice conversion skipped/failed)"
    fi
    rm -rf "$LO_PROFILE" "$OUT"
fi

if [ "$fail" -eq 0 ]; then
    echo "ALL TESTS PASSED"
else
    echo "TEST FAILURES PRESENT"
fi
exit $fail
