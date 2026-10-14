#!/usr/bin/env bash
# =============================================================================
# bootstrap.sh — 拉取并构建 OpenXLSX（含 pugixml / miniz）到项目内 .deps/
#
# 全程无需 root。产物：
#   third_party/OpenXLSX/    上游源码（git clone，固定 tag）
#   build/openxlsx/          OpenXLSX 构建目录
#   .deps/                   安装前缀（静态 + PIC，供 openxlsx.so/.dll 链接）
#   build/_deps/             FetchContent 下载的依赖源码
#
# 平台支持：Linux / macOS / Windows(MSYS2 或 Git-Bash) / 交叉编译
#   - 交叉编译示例：
#       GENERATOR=Ninja script       # 或用 Unix Makefiles / MinGW Makefiles
#       scripts/bootstrap.sh -- -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake \
#         -DDEPS=...                 # 见下方环境变量
# 环境变量（均可选）：
#   OPENXLSX_REPO / OPENXLSX_TAG   上游仓库与 tag
#   SRC_DIR / BUILD / DEPS         源码、构建、安装目录
#   GENERATOR                       CMake 生成器（默认自动检测 Ninja/Makefiles）
#   JOBS                            并行度（默认 nproc / sysctl / 4）
#   传给脚本的额外参数             原样追加到 cmake 配置命令（可放交叉编译 toolchain）
# =============================================================================
set -euo pipefail

# 容忍可选的 "--" 分隔符（用于把额外参数与脚本参数分开书写）
if [ "${1:-}" = "--" ]; then
    shift
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 上游仓库（Codeberg 迁移期地址；若日后迁到 troldal/OpenXLSX，改这里即可）
OPENXLSX_REPO="${OPENXLSX_REPO:-https://codeberg.org/lars_uffmann/OpenXLSX.git}"
OPENXLSX_TAG="${OPENXLSX_TAG:-v0.5.1}"

SRC="${SRC_DIR:-$ROOT/third_party/OpenXLSX}"
DEPS="${DEPS:-$ROOT/.deps}"
BUILD="${BUILD:-$ROOT/build/openxlsx}"

# 生成器：优先 Ninja，其次平台默认
if [ -z "${GENERATOR:-}" ]; then
    if command -v ninja >/dev/null 2>&1; then
        GENERATOR="Ninja"
    else
        GENERATOR="Unix Makefiles"
    fi
fi

# 并行度：nproc(Linux) / sysctl(macOS) / 回退 4
if [ -z "${JOBS:-}" ]; then
    if command -v nproc >/dev/null 2>&1; then
        JOBS="$(nproc)"
    elif command -v sysctl >/dev/null 2>&1; then
        JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
    else
        JOBS=4
    fi
fi

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
#  - 静态库 + PIC：openxlsx 模块需要链接静态 OpenXLSX；
#  - OPENXLSX_LOCAL_PACKAGES_ONLY=OFF（默认）+ FETCH_DEPS_AUTO：自动拉取
#    pugixml v1.15 与 miniz 3.0.2（Windows 上还包括 Boost.Nowide）；
#  - 关闭 docs/samples/tests/benchmarks，避免额外依赖。
#  - 生成器与交叉编译 toolchain 都可通过环境变量/额外参数覆盖。
echo "[bootstrap] configuring OpenXLSX (generator: $GENERATOR)"
mkdir -p "$BUILD"
cmake -S "$SRC" -B "$BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$DEPS" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DOPENXLSX_CREATE_DOCS=OFF \
    -DOPENXLSX_BUILD_SAMPLES=OFF \
    -DOPENXLSX_BUILD_TESTS=OFF \
    -DOPENXLSX_BUILD_BENCHMARKS=OFF \
    -DFETCHCONTENT_BASE_DIR="$BUILD/_deps" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    "$@"

# ---- 3. 构建并安装 ------------------------------------------------------------
echo "[bootstrap] building OpenXLSX (jobs=$JOBS)"
cmake --build "$BUILD" --config Release --parallel "$JOBS"

echo "[bootstrap] installing to $DEPS"
cmake --install "$BUILD" --config Release

echo "[bootstrap] done: $(find "$DEPS" \( -name '*.a' -o -name '*.lib' \) 2>/dev/null | tr '\n' ' ')"
