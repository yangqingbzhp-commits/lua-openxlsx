# 跨平台构建与使用（Linux / macOS / Windows）

本模块（`openxlsx`）的绑定代码是纯 C++17 + Lua C API，无平台相关调用；上游
OpenXLSX / pugixml / miniz 官方支持三大平台。差异集中在**构建与链接方式**和
**测试脚本**上，本页说明如何处理。

## 总览

| | Linux | macOS | Windows (MinGW-w64) |
|---|---|---|---|
| 模块文件名 | `openxlsx.so` | `openxlsx.so` | `openxlsx.dll` |
| Lua 符号解析 | 解释器导出符号（不链接 liblua） | 链接器 `-undefined dynamic_lookup` + 解释器 | **必须链接 Lua 导入库**（DLL 不允许未定义符号） |
| 本工程默认 | `OX_LINK_LUA=OFF` | `OX_LINK_LUA=OFF` | `OX_LINK_LUA=ON` |
| Lua 安装建议 | 源码 `make linux` 或发行版包 | `brew install lua` | MSYS2 `mingw-w64-x86_64-lua`、vcpkg、或源码 `make mingw` |
| 依赖构建 | `scripts/bootstrap.sh` | 同左（系统 bash 3.2 即可） | 同左（MSYS2/Git-Bash 中运行） |
| 验证状态 | ✅ 本机 + CI | ✅ **CI 真机 320 项全绿** | ✅ **CI 真机 320 项全绿** |

## CI 验证结果（三平台真机）

`.github/workflows/ci.yml` 在 GitHub 托管的真实机器上构建并运行全套测试：

| 平台 | runner | 结果 |
|---|---|---|
| Linux | `ubuntu-latest` + GCC | ✅ 320 项断言 + LibreOffice 回读校验 |
| macOS | `macos-latest` + AppleClang 21 | ✅ 320 项断言 |
| Windows | `windows-latest` + MSYS2 MinGW-w64（GCC 14） | ✅ 320 项断言（`openxlsx.dll` 链接 `lua55.dll` 运行） |

运行记录：<https://github.com/yangqingbzhp-commits/lua-openxlsx/actions/workflows/ci.yml>

### CI 上踩过的平台坑（均已修复）

| 现象 | 原因 | 修复 |
|---|---|---|
| Linux：`dynamic libraries not enabled` | Lua 用 `posix` 目标编译，未定义 `LUA_USE_DLOPEN` | 改用 `make -C src linux`（`LUA_USE_LINUX` = POSIX + DLOPEN） |
| macOS：miniz 配置报 `cmake_minimum_required` 错误 | 新版 CMake（≥4）不再兼容 `< 3.5` 的 miniz | bootstrap 传 `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` |
| Windows：`make: command not found` | MSYS2 工具链提供的是 `mingw32-make` | CI 改用 `mingw32-make`；MSYS2 用户也可 `pacman -S make` |
| Windows：`tar: Cannot connect to D: resolve failed` | MSYS2 的 `tar` 是 GNU tar，把 `D:/...` 当远程主机 | 优先用 `unzip`；回退到 `%SystemRoot%\System32\tar.exe`（Windows 自带 bsdtar） |
| Windows：Lua 读不到 `/d/...` 路径 | Lua 是原生 Windows 程序，不认 MSYS 路径；环境变量不会被自动转换 | `run_tests.sh` 用 `cygpath -m` 转换 `BUILD`/`OX_TEST_OUT`/`OX_TEST_DIR` |

## Linux

```bash
scripts/bootstrap.sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
scripts/run_tests.sh
cmake --install build          # -> ~/.local/lib/lua/5.5/
```

## macOS

与 Linux 基本相同，注意：

1. **Lua**：`brew install lua`（headers 在 `$(brew --prefix)/include`，CMake 会自动找到；
   Apple Silicon 前缀为 `/opt/homebrew`，已加入 `find_path` 的 HINTS）。
2. **未定义符号**：CMake 的 `MODULE` 库在 macOS 上链接时不允许未定义符号，
   本工程已自动加 `-Wl,-undefined,dynamic_lookup`（`OX_LINK_LUA=OFF` 时），
   由解释器在加载时提供 Lua 符号。
3. **并行度/脚本**：`bootstrap.sh` 会自动用 `sysctl -n hw.ncpu`（macOS 没有 `nproc`）；
   系统自带 bash 3.2 可运行脚本。
4. **LibreOffice 校验**（可选）：`run_tests.sh` 会找
   `/Applications/LibreOffice.app/Contents/MacOS/soffice`。

```bash
brew install lua cmake ninja     # 若尚未安装
scripts/bootstrap.sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
scripts/run_tests.sh
cmake --install build
# 新终端：export LUA_CPATH="$(brew --prefix)/lib/lua/5.5/?.so;;"（或安装到 ~/.local）
```

## Windows（MSYS2 / MinGW-w64 推荐）

### 1. 准备环境（MSYS2 MINGW64 终端）

```bash
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
                   mingw-w64-x86_64-ninja mingw-w64-x86_64-git \
                   mingw-w64-x86_64-lua
```

> `mingw-w64-x86_64-lua` 提供的可能是 Lua 5.4；若需要 5.5，可用源码构建：
> `make mingw CC=gcc`（在 lua-5.5.x 源码的 `src/` 目录），会产出 `lua55.dll` 与 `lua.exe`。
> 本工程按 Lua 5.5 ABI 安装到 `lib/lua/5.5`，如需 5.4 请加 `-DOX_LUA_ABI=5.4`。

### 2. 构建

```bash
scripts/bootstrap.sh                     # 自动拉取/编译 OpenXLSX + pugixml + miniz + Nowide
cmake -S . -B build -G Ninja \
      -DLUA_INCLUDE_DIR="$(dirname "$(pacman -Ql mingw-w64-x86_64-lua | grep '/lua.h$' | head -1 | cut -d' ' -f2)")" \
      -DLUA_LIBRARY="$(pacman -Ql mingw-w64-x86_64-lua | grep -E '/(liblua[a-z0-9]*\.dll\.a|lua[0-9]*\.lib)$' | head -1 | cut -d' ' -f2)"
cmake --build build                      # -> build/openxlsx.dll
scripts/run_tests.sh                     # 在 MSYS2 中运行；LibreOffice 校验可选
cmake --install build                    # -> %USERPROFILE%\.local\lib\lua\5.5\
```

**关键点**：Windows 的 DLL 不能有未定义符号，所以 `OX_LINK_LUA` 默认为 `ON`，
必须提供 `LUA_LIBRARY`（导入库 `liblua*.dll.a`/`lua*.lib`，或直接指向 `lua55.dll`——
MinGW 链接器可以直接从 DLL 生成导入）。

### 3. 使用

```powershell
# 设置模块搜索路径（当前会话；永久生效可用 setx）
set LUA_CPATH=%USERPROFILE%\.local\lib\lua\5.5\?.dll;;
lua -e "local x = require('openxlsx'); print(x.version())"
```

> 注意 Windows 上 Lua 的路径分隔符是 `;`，`?.dll` 是模块扩展名。

### 4. MSVC（Visual Studio）说明

理论可行但需自行准备 Lua 与 OpenXLSX 的 MSVC 构建（`cmake -G "Visual Studio 17 2022"`，
依赖会从 GitHub 拉取源码编译）。本工程已做 MSVC 分支：编译选项用 `/W3`，
模块后缀 `.dll`，安装前缀回退 `%USERPROFILE%\.local`。未经实测，欢迎反馈。

## 交叉编译（Linux → Windows）

已在本仓库验证（2026-10，Debian + mingw-w64 12/GCC 14）：可交叉产出 `openxlsx.dll`，
其导入表仅含 `lua55.dll`、`KERNEL32.dll`、`msvcrt.dll`（MinGW 运行时已静态链接），
即目标机只需要 Lua 与系统 DLL。一键脚本：`scripts/cross_build_windows.sh`。

```bash
# 1) Windows 版 Lua（含 lua55.dll）
curl -O https://www.lua.org/ftp/lua-5.5.1.tar.gz && tar xf lua-5.5.1.tar.gz -C third_party/
make -C third_party/lua-5.5.1/src mingw CC=x86_64-w64-mingw32-gcc

# 2) 交叉构建依赖（独立目录，避免与 Linux 的 .deps 混淆）
DEPS="$PWD/.deps-win" BUILD="$PWD/build-win/openxlsx" scripts/bootstrap.sh -- \
    -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/mingw-w64-x86_64.cmake"

# 3) 交叉构建模块
cmake -S . -B build-win -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/mingw-w64-x86_64.cmake" \
    -DOX_DEPS_DIR="$PWD/.deps-win" \
    -DLUA_INCLUDE_DIR="$PWD/third_party/lua-5.5.1/src" \
    -DLUA_LIBRARY="$PWD/third_party/lua-5.5.1/src/lua55.dll"
cmake --build build-win       # -> build-win/openxlsx.dll
```

（交叉编译只能验证"编译+链接通过"，运行验证需要 Windows 真机/虚拟机或 CI。）

## CI（GitHub Actions）

`.github/workflows/ci.yml` 提供三平台矩阵：`ubuntu-latest` / `macos-latest` /
`windows-latest`(MSYS2 MINGW64)，各自构建 Lua 5.5、OpenXLSX 依赖与模块，并运行
`scripts/run_tests.sh` 全部测试。推送到 GitHub 后即可看到真机验证结果。

## 平台适配清单（本工程已处理）

| 位置 | 处理 |
|---|---|
| `CMakeLists.txt` | MSVC 编译选项分支；`OX_LINK_LUA`（Windows 默认 ON）；`.dll`/`.so` 后缀；`%USERPROFILE%` 安装前缀回退；Apple `-undefined dynamic_lookup`；`OX_DEPS_DIR` 支持独立依赖前缀 |
| `scripts/bootstrap.sh` | 无 `nproc` 时用 `sysctl`；生成器自动检测/可覆盖；支持 toolchain 参数透传；交叉编译友好 |
| `scripts/run_tests.sh` | Lua 解释器探测；`.so`/`.dll` 模块探测；macOS/Windows LibreOffice 路径；严格只用新构建的模块 |
| `test/testutil.lua` | `makedirs` / `rmtree` / `extract_zip` / `read_first_line` 的平台分支（Windows 用 `tar.exe` 解压） |
| `cmake/mingw-w64-x86_64.cmake` | 交叉编译 toolchain 文件 |
| `.github/workflows/ci.yml` | 三平台 CI |
