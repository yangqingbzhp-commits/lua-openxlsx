#!/usr/bin/env bash
# =============================================================================
# cross_build_windows.sh — 在 Linux 上交叉构建 Windows(x64) 版模块
#
# 前置：apt install mingw-w64 cmake ninja git curl
# 产物：build-win/openxlsx.dll（运行需目标机有对应 lua55.dll）
#
# 说明：本脚本只做"编译+链接"验证；运行验证请在 Windows 真机或 CI 上进行。
# 环境变量：LUA_VERSION（默认 5.5.1）、JOBS、GENERATOR
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LUA_VERSION="${LUA_VERSION:-5.5.1}"
DEPS="$ROOT/.deps-win"
BUILD_DEPS="$ROOT/build-win/openxlsx"
BUILD_MOD="$ROOT/build-win"
LUA_SRC="$ROOT/third_party/lua-$LUA_VERSION"

if ! command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
    echo "error: mingw-w64 not installed (apt install mingw-w64)" >&2
    exit 1
fi

# ---- 1. Lua for Windows（提供编译所需头文件与运行/链接用 lua55.dll） --------
if [ ! -f "$LUA_SRC/src/lua55.dll" ]; then
    echo "[cross] fetching Lua $LUA_VERSION"
    curl -fsSL -o "/tmp/lua-$LUA_VERSION.tar.gz" "https://www.lua.org/ftp/lua-$LUA_VERSION.tar.gz"
    mkdir -p "$ROOT/third_party"
    tar -xzf "/tmp/lua-$LUA_VERSION.tar.gz" -C "$ROOT/third_party"
    echo "[cross] building Lua for Windows"
    make -C "$LUA_SRC/src" mingw CC=x86_64-w64-mingw32-gcc RANLIB=x86_64-w64-mingw32-ranlib >/dev/null
fi

# ---- 2. OpenXLSX 依赖（Windows 目标；含 pugixml/miniz/Nowide） ---------------
echo "[cross] building OpenXLSX for Windows -> $DEPS"
DEPS="$DEPS" BUILD="$BUILD_DEPS" \
    "$ROOT/scripts/bootstrap.sh" "-DCMAKE_TOOLCHAIN_FILE=$ROOT/cmake/mingw-w64-x86_64.cmake"

# ---- 3. 模块 -----------------------------------------------------------------
echo "[cross] building openxlsx.dll"
cmake -S "$ROOT" -B "$BUILD_MOD" -G "${GENERATOR:-Ninja}" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/mingw-w64-x86_64.cmake" \
    -DOX_DEPS_DIR="$DEPS" \
    -DLUA_INCLUDE_DIR="$LUA_SRC/src" \
    -DLUA_LIBRARY="$LUA_SRC/src/lua55.dll"
cmake --build "$BUILD_MOD" --parallel "${JOBS:-4}"

echo "[cross] done: $BUILD_MOD/openxlsx.dll"
x86_64-w64-mingw32-objdump -p "$BUILD_MOD/openxlsx.dll" | grep -E 'DLL Name' | sort -u | sed 's/^/  import: /'
