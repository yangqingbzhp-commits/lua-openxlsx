cmake_minimum_required(VERSION 3.16)
project(lua_openxlsx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Lua
#
# Only the headers are needed on Linux/macOS: the module leaves the Lua symbols
# undefined and the host interpreter provides them (thin host = classic Lua
# module setup). Windows DLLs cannot have unresolved symbols, so there the Lua
# library is linked (OX_LINK_LUA defaults to ON on Windows).
# ---------------------------------------------------------------------------
if(NOT LUA_INCLUDE_DIR)
    find_path(LUA_INCLUDE_DIR lua.h
        HINTS
            /usr/local/include /usr/include/lua5.5 /usr/include
            /opt/homebrew/include /usr/local/opt/lua/include
            "$ENV{USERPROFILE}/lua/include" "$ENV{HOME}/.local/include"
    )
endif()
if(NOT EXISTS "${LUA_INCLUDE_DIR}/lua.h")
    message(FATAL_ERROR "lua.h not found - pass -DLUA_INCLUDE_DIR=/path/to/lua/includes")
endif()

if(WIN32)
    set(_ox_link_lua_default ON)
else()
    set(_ox_link_lua_default OFF)
endif()
option(OX_LINK_LUA "Link the Lua library into the module (required on Windows)" ${_ox_link_lua_default})

if(OX_LINK_LUA AND NOT LUA_LIBRARY)
    find_library(LUA_LIBRARY
        NAMES lua55 lua5.5 lua54 lua5.4 lua53 lua5.3 lua lua55.dll lua54.dll
        HINTS
            /usr/local/lib /usr/lib /opt/homebrew/lib /usr/local/opt/lua/lib
            "$ENV{USERPROFILE}/lua" "$ENV{HOME}/.local/lib"
    )
endif()
if(OX_LINK_LUA AND NOT LUA_LIBRARY)
    message(FATAL_ERROR "OX_LINK_LUA is ON but the Lua library was not found - pass -DLUA_LIBRARY=/path/to/lua/lib")
endif()

# ---------------------------------------------------------------------------
# OpenXLSX (built by scripts/bootstrap.sh into .deps/)
# ---------------------------------------------------------------------------
set(OX_DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.deps" CACHE PATH "Prefix of the OpenXLSX dependency build")
list(APPEND CMAKE_PREFIX_PATH "${OX_DEPS_DIR}")
find_package(OpenXLSX CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# The module
# ---------------------------------------------------------------------------
set(OX_LUA_ABI "5.5" CACHE STRING "Lua ABI version used for install paths (e.g. 5.5)")

file(GLOB OX_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(openxlsx MODULE ${OX_SOURCES})
target_include_directories(openxlsx PRIVATE "${LUA_INCLUDE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(openxlsx PRIVATE OpenXLSX::OpenXLSX)

if(OX_LINK_LUA)
    target_link_libraries(openxlsx PRIVATE "${LUA_LIBRARY}")
elseif(APPLE)
    # Let the linker leave the Lua symbols undefined; the interpreter resolves
    # them at load time (standard for Lua modules on macOS).
    target_link_options(openxlsx PRIVATE "LINKER:-undefined,dynamic_lookup")
endif()

if(MSVC)
    target_compile_options(openxlsx PRIVATE /W3 /wd4100)  # 4100: unreferenced formal parameter
else()
    target_compile_options(openxlsx PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

if(MINGW)
    # Ease deployment: without this the DLL also requires libgcc_s_*.dll and
    # libstdc++-6.dll from the MinGW distribution.
    option(OX_MINGW_STATIC_RUNTIME "Statically link the MinGW runtime (libgcc/libstdc++)" ON)
    if(OX_MINGW_STATIC_RUNTIME)
        target_link_options(openxlsx PRIVATE -static-libgcc -static-libstdc++)
    endif()
endif()

if(WIN32)
    set(_ox_module_suffix ".dll")
    set(_ox_module_name "openxlsx.dll")
else()
    set(_ox_module_suffix ".so")
    set(_ox_module_name "openxlsx.so")
endif()
set_target_properties(openxlsx PROPERTIES
    PREFIX ""
    OUTPUT_NAME "openxlsx"
    SUFFIX "${_ox_module_suffix}"
)

# Default install prefix: ~/.local (no root required)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(WIN32)
        if(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
            set(_ox_default_prefix "$ENV{USERPROFILE}/.local")
        elseif(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
            set(_ox_default_prefix "$ENV{HOME}/.local")
        else()
            set(_ox_default_prefix "${CMAKE_CURRENT_SOURCE_DIR}/install")
        endif()
    else()
        set(_ox_default_prefix "$ENV{HOME}/.local")
    endif()
    set(CMAKE_INSTALL_PREFIX "${_ox_default_prefix}" CACHE PATH "Install prefix" FORCE)
endif()

install(TARGETS openxlsx LIBRARY DESTINATION "lib/lua/${OX_LUA_ABI}"
                       RUNTIME DESTINATION "lib/lua/${OX_LUA_ABI}")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lua/openxlsx/init.lua")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/lua/openxlsx/init.lua"
            DESTINATION "share/lua/${OX_LUA_ABI}/openxlsx")
endif()

# Helpful summary
message(STATUS "lua-openxlsx: Lua headers  = ${LUA_INCLUDE_DIR}")
message(STATUS "lua-openxlsx: link Lua     = ${OX_LINK_LUA}${LUA_LIBRARY}")
message(STATUS "lua-openxlsx: install prefix = ${CMAKE_INSTALL_PREFIX}")
message(STATUS "lua-openxlsx: install path   = lib/lua/${OX_LUA_ABI}/${_ox_module_name}")
