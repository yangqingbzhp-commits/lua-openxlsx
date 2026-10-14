#!/usr/bin/env bash
# =============================================================================
# bootstrap.sh — 拉取并构建 OpenXLSX（含 pugixml / miniz）到项目内 .deps/
#
# 全程无需 root。产物：
#   third_party/OpenXLSX/    上游源码（git clone，固定 tag）
#   build/openxlsx/          OpenXLSX 构建目录
#   .deps/                   安装前缀（静态 + PIC，供 openxlsx.so 链接）
#   build/_deps/             FetchContent 下载的依赖源码
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 上游仓库（Codeberg 迁移期地址；若日后迁到 troldal/OpenXLSX，改这里即可）
OPENXLSX_REPO="${OPENXLSX_REPO:-https://codeberg.org/lars_uffmann/OpenXLSX.git}"
OPENXLSX_TAG="${OPENXLSX_TAG:-v0.5.1}"

SRC="$ROOT/third_party/OpenXLSX"
DEPS="$ROOT/.deps"
BUILD="$ROOT/build/openxlsx"
JOBS="${JOBS:-$(nproc)}"

# ---- 1. 拉取源码（固定 tag，保证可复现） ------------------------------------
if [ ! -d "$SRC/.git" ]; then
    echo "[bootstrap] cloning $OPENXLSX_REPO ($OPENXLSX_TAG) -> $SRC"
    mkdir -p "$(dirname "$SRC")"
    git clone --branch "$OPENXLSX_TAG" --depth 1 "$OPENXLSX_REPO" "$SRC"
else
    echo "[bootstrap] $SRC already exists, skipping clone"
fi

# ---- 2. 配置 -----------------------------------------------------------------
# 说明：
#  - 静态库 + PIC：openxlsx.so（MODULE）需要链接静态 OpenXLSX；
#  - OPENXLSX_LOCAL_PACKAGES_ONLY=OFF（默认）+ FETCH_DEPS_AUTO：自动从 GitHub
#    拉取 pugixml v1.15 与 miniz 3.0.2；
#  - 关闭 docs/samples/tests/benchmarks，避免额外依赖。
echo "[bootstrap] configuring OpenXLSX"
cmake -S "$SRC" -B "$BUILD" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$DEPS" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DOPENXLSX_CREATE_DOCS=OFF \
    -DOPENXLSX_BUILD_SAMPLES=OFF \
    -DOPENXLSX_BUILD_TESTS=OFF \
    -DOPENXLSX_BUILD_BENCHMARKS=OFF \
    -DFETCHCONTENT_BASE_DIR="$ROOT/build/_deps"

# ---- 3. 构建并安装 ------------------------------------------------------------
echo "[bootstrap] building OpenXLSX (jobs=$JOBS)"
cmake --build "$BUILD" --parallel "$JOBS"

echo "[bootstrap] installing to $DEPS"
cmake --install "$BUILD"

echo "[bootstrap] done: $(find "$DEPS" -name '*.a' | tr '\n' ' ')"
