rockspec_format = "3.0"
package = "lua-openxlsx"
version = "0.1.2-1"

source = {
   url = "git+https://github.com/yangqingbzhp-commits/lua-openxlsx.git",
   tag = "v0.1.2",
}

description = {
   summary = "Bindings for OpenXLSX: read/write .xlsx files from Lua",
   detailed = [[
      Lua 5.5/5.4 binding for the OpenXLSX v0.5.1 C++ library with full public
      API coverage: typed cell values, formulas, dates, batch row/column
      access, ranges and iterators, merged cells, worksheet management,
      conditional formatting, sheet protection, document properties, the
      complete style family (fonts, fills, borders, number formats,
      alignment, named styles, dxf) and comments / VML shapes / relationships.

      OpenXLSX itself is fetched and built automatically (pugixml, miniz;
      Boost.Nowide on Windows), or a locally built copy can be reused via
      scripts/bootstrap.sh and the .deps prefix.
   ]],
   homepage = "https://github.com/yangqingbzhp-commits/lua-openxlsx",
   license = "MIT",
}

dependencies = {
   "lua >= 5.4",
}

build = {
   type = "command",
   -- $(LUA_INCDIR)/$(LIBDIR) are substituted by LuaRocks.
   -- OX_LUA_MODULE_DIR places the module exactly where LuaRocks deploys from.
   -- OX_FETCH_OPENXLSX makes the build standalone (fetches OpenXLSX v0.5.1);
   -- a prebuilt copy (.deps via scripts/bootstrap.sh) is reused when present.
   build_command = [[
      cmake -S . -B build.luarocks -DCMAKE_BUILD_TYPE=Release \
        -DLUA_INCLUDE_DIR="$(LUA_INCDIR)" \
        -DOX_FETCH_OPENXLSX=ON \
        -DOX_LUA_MODULE_DIR="$(LIBDIR)" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
      && cmake --build build.luarocks --config Release --parallel
   ]],
   -- only the module target is installed (OpenXLSX keeps its own install rules)
   install_command = "cmake --install build.luarocks --config Release --component ox-module",
}
