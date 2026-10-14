# =============================================================================
# mingw-w64-x86_64.cmake — 交叉编译 Lua 模块到 Windows x64（MinGW-w64）
#
# 用法（先安装工具链：apt install mingw-w64）：
#   1) 交叉构建 OpenXLSX 依赖：
#        scripts/bootstrap.sh -- \
#            -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/mingw-w64-x86_64.cmake" \
#            -DDEPS="$PWD/.deps-win" -DBUILD="$PWD/build-win/openxlsx" \
#            -DOPENXLSX_NOWIDE_STANDALONE=ON
#   2) 交叉构建模块（需要 Windows 版 Lua 头文件与库）：
#        cmake -S . -B build-win -G Ninja \
#            -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake \
#            -DCMAKE_PREFIX_PATH="$PWD/.deps-win" \
#            -DLUA_INCLUDE_DIR=<lua-5.5/src> \
#            -DLUA_LIBRARY=<liblua.a 或 lua55.dll 的路径>
#        cmake --build build-win
#
# 产物：build-win/openxlsx.dll（运行需目标机有对应 lua55.dll）
# =============================================================================
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)
set(CMAKE_AR           x86_64-w64-mingw32-ar)
set(CMAKE_RANLIB       x86_64-w64-mingw32-ranlib)

# 在宿主机查找程序（git/cmake），但库和头文件只查找 Windows 目标目录；
# 包（*.cmake config，例如项目内 .deps-win 里的 OpenXLSX）允许在项目侧查找
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
