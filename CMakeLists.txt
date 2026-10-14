cmake_minimum_required(VERSION 3.16)
project(lua_openxlsx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Lua headers only.
# The module resolves Lua symbols from the host interpreter at load time
# (standard for Lua C modules on Linux), so we intentionally do NOT link liblua.
# ---------------------------------------------------------------------------
set(LUA_INCLUDE_DIR "/usr/local/include" CACHE PATH "Directory containing lua.h")
if(NOT EXISTS "${LUA_INCLUDE_DIR}/lua.h")
    find_path(LUA_INCLUDE_DIR lua.h HINTS /usr/local/include /usr/include/lua5.5 /usr/include)
endif()
if(NOT EXISTS "${LUA_INCLUDE_DIR}/lua.h")
    message(FATAL_ERROR "lua.h not found - pass -DLUA_INCLUDE_DIR=/path/to/lua/includes")
endif()

# ---------------------------------------------------------------------------
# OpenXLSX (built by scripts/bootstrap.sh into .deps/)
# ---------------------------------------------------------------------------
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
find_package(OpenXLSX CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# The module
# ---------------------------------------------------------------------------
set(OX_LUA_ABI "5.5" CACHE STRING "Lua ABI version used for install paths (e.g. 5.5)")

file(GLOB OX_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(openxlsx MODULE ${OX_SOURCES})
target_include_directories(openxlsx PRIVATE "${LUA_INCLUDE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(openxlsx PRIVATE OpenXLSX::OpenXLSX)
target_compile_options(openxlsx PRIVATE -Wall -Wextra -Wno-unused-parameter)

set_target_properties(openxlsx PROPERTIES
    PREFIX ""
    OUTPUT_NAME "openxlsx"
)

# Default install prefix: ~/.local (no root required)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install prefix" FORCE)
endif()

install(TARGETS openxlsx LIBRARY DESTINATION "lib/lua/${OX_LUA_ABI}")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lua/openxlsx/init.lua")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lua/openxlsx/init.lua"
            DESTINATION "share/lua/${OX_LUA_ABI}/openxlsx")
endif()

# Helpful summary
message(STATUS "lua-openxlsx: Lua headers  = ${LUA_INCLUDE_DIR}")
message(STATUS "lua-openxlsx: install prefix = ${CMAKE_INSTALL_PREFIX}")
message(STATUS "lua-openxlsx: install path   = lib/lua/${OX_LUA_ABI}/openxlsx.so")
