// =============================================================================
// bind_p5.cpp — P5 odds and ends:
//   * cell:equals / row:equals (upstream operator==)
//   * value:variant()
//   * path helpers (getPathARelativeToPathB, eliminateDotAndDotDotFromPath)
//   * XLXmlSavingDeclaration + doc:set_saving_declaration
// =============================================================================
#include "lua_bind.hpp"
#include "style_machinery.hpp"

#include <cstring>
#include <iterator>

using namespace OpenXLSX;
using namespace ox;

namespace {

// ============================================================================
// member table for XLXmlSavingDeclaration
// ============================================================================
const StyleMember saving_declaration_members[] = {
    OX_PROP_RO(XLXmlSavingDeclaration, version, version),
    OX_PROP_RO(XLXmlSavingDeclaration, encoding, encoding),
    OX_PROP_RO(XLXmlSavingDeclaration, standalone, standalone),
    OX_PROP_RO(XLXmlSavingDeclaration, standalone_as_bool, standalone_as_bool),
};

// ============================================================================
// equality helpers
// ============================================================================
int cell_equals(lua_State *L) {
    OX_TRY
    auto *a = check<XLCell>(L, 1, MT_CELL);
    auto *b = check<XLCell>(L, 2, MT_CELL);
    lua_pushboolean(L, *a == *b);
    return 1;
    OX_CATCH_ALL
}

int row_equals(lua_State *L) {
    OX_TRY
    auto *a = check<XLRow>(L, 1, MT_ROW);
    auto *b = check<XLRow>(L, 2, MT_ROW);
    lua_pushboolean(L, *a == *b);
    return 1;
    OX_CATCH_ALL
}

// ============================================================================
// XLCellValue::getVariant()
// ============================================================================
int cellvalue_variant(lua_State *L) {
    OX_TRY
    auto *v = check<XLCellValue>(L, 1, MT_CELLVALUE);
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, int(v->type()));
    lua_setfield(L, -2, "type");
    push_cellvalue(L, *v);
    lua_setfield(L, -2, "value");
    return 1;
    OX_CATCH_ALL
}

// ============================================================================
// path helpers
// ============================================================================
int l_path_relative_to(lua_State *L) {
    OX_TRY
    push_string(L, OpenXLSX::getPathARelativeToPathB(check_string(L, 1), check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}

int l_normalize_path(lua_State *L) {
    OX_TRY
    push_string(L, OpenXLSX::eliminateDotAndDotDotFromPath(check_string(L, 1)));
    return 1;
    OX_CATCH_ALL
}

// ============================================================================
// XLXmlSavingDeclaration
// ============================================================================
int saving_declaration_new(lua_State *L) {
    OX_TRY
    const int n = lua_gettop(L);
    if (n == 0) {
        push<XLXmlSavingDeclaration>(L, MT_XML_SAVING_DECLARATION, XLXmlSavingDeclaration());
    } else if (n == 2) {
        push<XLXmlSavingDeclaration>(L, MT_XML_SAVING_DECLARATION,
                                     XLXmlSavingDeclaration(check_string(L, 1), check_string(L, 2)));
    } else {
        push<XLXmlSavingDeclaration>(L, MT_XML_SAVING_DECLARATION,
                                     XLXmlSavingDeclaration(check_string(L, 1), check_string(L, 2),
                                                            opt_bool(L, 3, false)));
    }
    return 1;
    OX_CATCH_ALL
}

int doc_set_saving_declaration(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    auto *decl = check<XLXmlSavingDeclaration>(L, 2, MT_XML_SAVING_DECLARATION);
    doc->setSavingDeclaration(*decl);
    return 0;
    OX_CATCH_ALL
}

}  // namespace

namespace ox {

void bind_p5(lua_State *L) {
    // ---- cell / row equality: add to existing metatable method tables ----
    {
        const luaL_Reg cell_extra[] = { { "equals", cell_equals }, { nullptr, nullptr } };
        add_style_methods(L, MT_CELL, cell_extra);
    }
    {
        const luaL_Reg row_extra[] = { { "equals", row_equals }, { nullptr, nullptr } };
        add_style_methods(L, MT_ROW, row_extra);
    }
    {
        const luaL_Reg value_extra[] = { { "variant", cellvalue_variant }, { nullptr, nullptr } };
        add_style_methods(L, MT_CELLVALUE, value_extra);
    }
    {
        const luaL_Reg doc_extra[] = { { "set_saving_declaration", doc_set_saving_declaration },
                                       { nullptr, nullptr } };
        add_style_methods(L, MT_DOCUMENT, doc_extra);
    }

    // ---- XLXmlSavingDeclaration ----
    register_style_type<XLXmlSavingDeclaration>(L, MT_XML_SAVING_DECLARATION, saving_declaration_members,
                                                std::size(saving_declaration_members));
    lua_pushcfunction(L, saving_declaration_new);
    lua_setfield(L, 1, "saving_declaration");

    // ---- path helpers ----
    lua_pushcfunction(L, l_path_relative_to);
    lua_setfield(L, 1, "path_relative_to");
    lua_pushcfunction(L, l_normalize_path);
    lua_setfield(L, 1, "normalize_path");
}

}  // namespace ox
