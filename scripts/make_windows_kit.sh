#!/usr/bin/env bash
# =============================================================================
# make_windows_kit.sh — 组装"Windows 测试包"（zip）
#
# 前置：已交叉编译出 build-win/openxlsx.dll，且已构建 Windows 版 Lua
#       （scripts/cross_build_windows.sh 会自动完成两者）
# 产物：dist/lua-openxlsx-windows-kit-<版本>.zip
#
# 把 zip 拷到任意 64 位 Windows 机器，解压后双击 run_smoke.cmd / run_tests.cmd。
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LUA_VERSION="${LUA_VERSION:-5.5.1}"
LUA_SRC="$ROOT/third_party/lua-$LUA_VERSION/src"
MODULE="$ROOT/build-win/openxlsx.dll"
OUT_DIR="$ROOT/dist"
STAGE="$OUT_DIR/windows-kit"
ZIP="$OUT_DIR/lua-openxlsx-windows-kit.zip"

for f in "$MODULE" "$LUA_SRC/lua.exe" "$LUA_SRC/lua55.dll"; do
    if [ ! -f "$f" ]; then
        echo "error: missing $f" >&2
        echo "       run scripts/cross_build_windows.sh first" >&2
        exit 1
    fi
done

rm -rf "$STAGE"
mkdir -p "$STAGE/test"

# ---- binaries ----
cp "$MODULE" "$STAGE/openxlsx.dll"
cp "$LUA_SRC/lua.exe" "$STAGE/lua.exe"
cp "$LUA_SRC/lua55.dll" "$STAGE/lua55.dll"
[ -f "$LUA_SRC/luac.exe" ] && cp "$LUA_SRC/luac.exe" "$STAGE/luac.exe" || true

# ---- scripts / docs (CRLF for cmd.exe) ----
cp "$ROOT/scripts/windows_kit/smoke.lua" "$STAGE/smoke.lua"
for f in run_smoke.cmd run_tests.cmd README.txt; do
    sed 's/$/\r/' "$ROOT/scripts/windows_kit/$f" > "$STAGE/$f"
done

# ---- the full test suite (testutil.lua is picked up by the tests) ----
cp "$ROOT"/test/*.lua "$STAGE/test/"

# ---- zip via python3 (does not require the zip package) ----
python3 - "$STAGE" "$ZIP" <<'PYEOF'
import os, sys, zipfile

stage, zip_path = sys.argv[1], sys.argv[2]
os.makedirs(os.path.dirname(zip_path), exist_ok=True)
with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
    for root, _dirs, files in os.walk(stage):
        for name in sorted(files):
            full = os.path.join(root, name)
            rel = os.path.relpath(full, os.path.dirname(stage))
            z.write(full, rel)
print(f"zip written: {zip_path}")
with zipfile.ZipFile(zip_path) as z:
    for info in z.infolist():
        print(f"  {info.file_size:>9}  {info.filename}")
PYEOF
