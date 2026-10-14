#!/usr/bin/env bash
# =============================================================================
# run_tests.sh — run the Lua test suite (+ optional LibreOffice round-trip)
#
# Usage: scripts/run_tests.sh [build-dir]
# Supports Linux / macOS / Windows (MSYS2 or Git-Bash).
# Environment overrides: LUA, BUILD, OX_TEST_OUT
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${1:-${BUILD:-$ROOT/build}}"

# Lua interpreter (allow override)
LUA="${LUA:-}"
if [ -z "$LUA" ]; then
    for candidate in lua lua5.5 lua55 lua5.4 lua54; do
        if command -v "$candidate" >/dev/null 2>&1; then LUA="$candidate"; break; fi
    done
fi
if [ -z "$LUA" ]; then
    echo "error: no Lua interpreter found (set LUA=/path/to/lua)" >&2
    exit 1
fi

# module extension depends on the platform
MODULE="$BUILD/openxlsx.so"
if [ ! -f "$MODULE" ] && [ -f "$BUILD/openxlsx.dll" ]; then
    MODULE="$BUILD/openxlsx.dll"
fi
if [ ! -f "$MODULE" ]; then
    echo "error: no module found in $BUILD (build first: cmake --build build)" >&2
    exit 1
fi
export OX_MODULE_PATH="$MODULE"

# On MSYS2/Git-Bash, Lua (a native Windows program) needs Windows-style paths.
BUILD_LUA="$BUILD"
if command -v cygpath >/dev/null 2>&1; then
    BUILD_LUA="$(cygpath -m "$BUILD")"
fi

export OX_TEST_OUT="${OX_TEST_OUT:-${TMPDIR:-/tmp}/openxlsx-tests}"
export OX_TEST_DIR="$ROOT/test"
if command -v cygpath >/dev/null 2>&1; then
    OX_TEST_OUT="$(cygpath -m "$OX_TEST_OUT")"
    OX_TEST_DIR="$(cygpath -m "$OX_TEST_DIR")"
    export OX_TEST_OUT OX_TEST_DIR
fi
mkdir -p "$OX_TEST_OUT"

# Use strictly the freshly built module (no fallback to an installed copy)
export LUA_CPATH="$BUILD_LUA/?.so;$BUILD_LUA/?.dll"
echo "module: $MODULE"

cd "$ROOT"

fail=0
for t in "$ROOT"/test/*.lua; do
    case "$(basename "$t")" in
        testutil.lua|helpers.lua) continue ;;
    esac
    echo "--- $(basename "$t")"
    if ! "$LUA" "$t"; then
        echo "    FAILED"
        fail=1
    fi
done

# ---- independent verification with LibreOffice (if available) ----------------
SOFFICE=""
if command -v soffice >/dev/null 2>&1; then
    SOFFICE="soffice"
elif [ -x "/Applications/LibreOffice.app/Contents/MacOS/soffice" ]; then
    SOFFICE="/Applications/LibreOffice.app/Contents/MacOS/soffice"
elif [ -n "${PROGRAMFILES:-}" ] && [ -x "$PROGRAMFILES/LibreOffice/program/soffice.exe" ]; then
    SOFFICE="$PROGRAMFILES/LibreOffice/program/soffice.exe"
fi

if [ -n "$SOFFICE" ] && [ -f "$OX_TEST_OUT/smoke.xlsx" ]; then
    echo "--- libreoffice round-trip"
    LO_PROFILE="${TMPDIR:-/tmp}/lo-profile-$$"
    OUT="${TMPDIR:-/tmp}/lo-convert-$$"
    mkdir -p "$OUT"
    if "$SOFFICE" -env:UserInstallation="file://$LO_PROFILE" --headless --norestore \
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
