// =============================================================================
// bind_data.cpp — XLCell, XLCellValue, XLCellReference, XLDateTime, XLColor,
//                 XLFormula, XLRow, XLColumn, XLCellRange, XLRowRange,
//                 XLRowDataRange, iterators, XLMergeCells
// =============================================================================
#include "lua_bind.hpp"

#include <cstring>

using namespace OpenXLSX;
using namespace ox;

namespace {

// ============================================================================
// XLCellReference
// ============================================================================
int ref_new(lua_State *L) {
    OX_TRY
    if (lua_type(L, 1) == LUA_TSTRING) {
        push<XLCellReference>(L, MT_REFERENCE, XLCellReference(check_cell_address(L, 1)));
        return 1;
    }
    if (lua_isinteger(L, 1) && lua_isinteger(L, 2)) {
        const lua_Integer row = luaL_checkinteger(L, 1);
        const lua_Integer col = luaL_checkinteger(L, 2);
        check_row_col_range(L, row, col);
        push<XLCellReference>(L, MT_REFERENCE, XLCellReference(uint32_t(row), uint16_t(col)));
        return 1;
    }
    if (lua_isinteger(L, 1) && lua_type(L, 2) == LUA_TSTRING) {
        const lua_Integer row = luaL_checkinteger(L, 1);
        check_row_col_range(L, row, 1);
        push<XLCellReference>(L, MT_REFERENCE, XLCellReference(uint32_t(row), check_column_letters(L, 2)));
        return 1;
    }
    return luaL_error(L, "reference: expected ('A1') or (row, col) or (row, 'A')");
    OX_CATCH_ALL
}

int ref_address(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCellReference>(L, 1, MT_REFERENCE)->address());
    return 1;
    OX_CATCH_ALL
}

int ref_row(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCellReference>(L, 1, MT_REFERENCE)->row()));
    return 1;
    OX_CATCH_ALL
}

int ref_column(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCellReference>(L, 1, MT_REFERENCE)->column()));
    return 1;
    OX_CATCH_ALL
}

int ref_set_row(lua_State *L) {
    OX_TRY
    check<XLCellReference>(L, 1, MT_REFERENCE)->setRow(uint32_t(luaL_checkinteger(L, 2)));
    return 0;
    OX_CATCH_ALL
}

int ref_set_column(lua_State *L) {
    OX_TRY
    check<XLCellReference>(L, 1, MT_REFERENCE)->setColumn(uint16_t(luaL_checkinteger(L, 2)));
    return 0;
    OX_CATCH_ALL
}

int ref_set_row_and_column(lua_State *L) {
    OX_TRY
    check<XLCellReference>(L, 1, MT_REFERENCE)
        ->setRowAndColumn(uint32_t(luaL_checkinteger(L, 2)), uint16_t(luaL_checkinteger(L, 3)));
    return 0;
    OX_CATCH_ALL
}

int ref_set_address(lua_State *L) {
    OX_TRY
    check<XLCellReference>(L, 1, MT_REFERENCE)->setAddress(check_cell_address(L, 2));
    return 0;
    OX_CATCH_ALL
}

int ref_tostring(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCellReference>(L, 1, MT_REFERENCE)->address());
    return 1;
    OX_CATCH_ALL
}

int ref_eq(lua_State *L) {
    OX_TRY
    auto *a = maybe<XLCellReference>(L, 1, MT_REFERENCE);
    auto *b = maybe<XLCellReference>(L, 2, MT_REFERENCE);
    lua_pushboolean(L, a != nullptr && b != nullptr && *a == *b);
    return 1;
    OX_CATCH_ALL
}

int ref_lt(lua_State *L) {
    OX_TRY
    auto *a = check<XLCellReference>(L, 1, MT_REFERENCE);
    auto *b = check<XLCellReference>(L, 2, MT_REFERENCE);
    lua_pushboolean(L, *a < *b);
    return 1;
    OX_CATCH_ALL
}

int ref_le(lua_State *L) {
    OX_TRY
    auto *a = check<XLCellReference>(L, 1, MT_REFERENCE);
    auto *b = check<XLCellReference>(L, 2, MT_REFERENCE);
    lua_pushboolean(L, *a <= *b);
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg ref_methods[] = {
    { "address", ref_address },
    { "row", ref_row },
    { "column", ref_column },
    { "set_row", ref_set_row },
    { "set_column", ref_set_column },
    { "set_row_and_column", ref_set_row_and_column },
    { "set_address", ref_set_address },
    { "to_string", ref_tostring },
    { nullptr, nullptr },
};

// ============================================================================
// XLDateTime
// ============================================================================
int datetime_new(lua_State *L) {
    OX_TRY
    XLDateTime dt = lua_isnoneornil(L, 1) ? XLDateTime() : check_datetime(L, 1);
    push<XLDateTime>(L, MT_DATETIME, std::move(dt));
    return 1;
    OX_CATCH_ALL
}

int datetime_from_unix(lua_State *L) {
    OX_TRY
    const time_t t = time_t(luaL_checkinteger(L, 1));
    push<XLDateTime>(L, MT_DATETIME, XLDateTime(t));
    return 1;
    OX_CATCH_ALL
}

int datetime_serial(lua_State *L) {
    OX_TRY
    lua_pushnumber(L, check<XLDateTime>(L, 1, MT_DATETIME)->serial());
    return 1;
    OX_CATCH_ALL
}

int datetime_tm(lua_State *L) {
    OX_TRY
    push_datetime_table(L, *check<XLDateTime>(L, 1, MT_DATETIME));
    return 1;
    OX_CATCH_ALL
}

int datetime_set_serial(lua_State *L) {
    OX_TRY
    *check<XLDateTime>(L, 1, MT_DATETIME) = double(luaL_checknumber(L, 2));
    return 0;
    OX_CATCH_ALL
}

int datetime_set_tm(lua_State *L) {
    OX_TRY
    const XLDateTime other = check_datetime(L, 2);
    *check<XLDateTime>(L, 1, MT_DATETIME) = other.tm();
    return 0;
    OX_CATCH_ALL
}

int datetime_tostring(lua_State *L) {
    OX_TRY
    const std::tm t = check<XLDateTime>(L, 1, MT_DATETIME)->tm();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    lua_pushstring(L, buf);
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg datetime_methods[] = {
    { "serial", datetime_serial },
    { "tm", datetime_tm },
    { "set_serial", datetime_set_serial },
    { "set_tm", datetime_set_tm },
    { "to_string", datetime_tostring },
    { nullptr, nullptr },
};

// ============================================================================
// XLColor
// ============================================================================
int color_new(lua_State *L) {
    OX_TRY
    if (lua_type(L, 1) == LUA_TSTRING) {
        push<XLColor>(L, MT_COLOR, XLColor(check_string(L, 1)));
        return 1;
    }
    if (lua_gettop(L) >= 4) {
        push<XLColor>(L, MT_COLOR, XLColor(uint8_t(luaL_checkinteger(L, 1)), uint8_t(luaL_checkinteger(L, 2)),
                                           uint8_t(luaL_checkinteger(L, 3)), uint8_t(luaL_checkinteger(L, 4))));
        return 1;
    }
    if (lua_gettop(L) >= 3) {
        push<XLColor>(L, MT_COLOR, XLColor(uint8_t(luaL_checkinteger(L, 1)), uint8_t(luaL_checkinteger(L, 2)),
                                           uint8_t(luaL_checkinteger(L, 3))));
        return 1;
    }
    return luaL_error(L, "color: expected ('RRGGBB'[, 'AARRGGBB']) or (r,g,b) or (a,r,g,b)");
    OX_CATCH_ALL
}

int color_set(lua_State *L) {
    OX_TRY
    auto *c = check<XLColor>(L, 1, MT_COLOR);
    if (lua_type(L, 2) == LUA_TSTRING) {
        c->set(check_string(L, 2));
    } else if (lua_gettop(L) >= 5) {
        c->set(uint8_t(luaL_checkinteger(L, 2)), uint8_t(luaL_checkinteger(L, 3)),
               uint8_t(luaL_checkinteger(L, 4)), uint8_t(luaL_checkinteger(L, 5)));
    } else {
        c->set(uint8_t(luaL_checkinteger(L, 2)), uint8_t(luaL_checkinteger(L, 3)), uint8_t(luaL_checkinteger(L, 4)));
    }
    return 0;
    OX_CATCH_ALL
}

int color_alpha(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, check<XLColor>(L, 1, MT_COLOR)->alpha());
    return 1;
    OX_CATCH_ALL
}
int color_red(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, check<XLColor>(L, 1, MT_COLOR)->red());
    return 1;
    OX_CATCH_ALL
}
int color_green(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, check<XLColor>(L, 1, MT_COLOR)->green());
    return 1;
    OX_CATCH_ALL
}
int color_blue(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, check<XLColor>(L, 1, MT_COLOR)->blue());
    return 1;
    OX_CATCH_ALL
}
int color_hex(lua_State *L) {
    OX_TRY
    push_string(L, check<XLColor>(L, 1, MT_COLOR)->hex());
    return 1;
    OX_CATCH_ALL
}
int color_eq(lua_State *L) {
    OX_TRY
    auto *a = maybe<XLColor>(L, 1, MT_COLOR);
    auto *b = maybe<XLColor>(L, 2, MT_COLOR);
    lua_pushboolean(L, a != nullptr && b != nullptr && *a == *b);
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg color_methods[] = {
    { "set", color_set },      { "alpha", color_alpha }, { "red", color_red },
    { "green", color_green },  { "blue", color_blue },   { "hex", color_hex },
    { "to_string", color_hex }, { nullptr, nullptr },
};

// ============================================================================
// XLFormula
// ============================================================================
int formula_new(lua_State *L) {
    OX_TRY
    push<XLFormula>(L, MT_FORMULA, XLFormula(opt_string(L, 1)));
    return 1;
    OX_CATCH_ALL
}

int formula_get(lua_State *L) {
    OX_TRY
    push_string(L, check<XLFormula>(L, 1, MT_FORMULA)->get());
    return 1;
    OX_CATCH_ALL
}

int formula_set(lua_State *L) {
    OX_TRY
    check<XLFormula>(L, 1, MT_FORMULA)->set(check_string(L, 2));
    return 0;
    OX_CATCH_ALL
}

int formula_clear(lua_State *L) {
    OX_TRY
    check<XLFormula>(L, 1, MT_FORMULA)->clear();
    return 0;
    OX_CATCH_ALL
}

int formula_tostring(lua_State *L) {
    OX_TRY
    push_string(L, check<XLFormula>(L, 1, MT_FORMULA)->get());
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg formula_methods[] = {
    { "get", formula_get }, { "set", formula_set }, { "clear", formula_clear },
    { "to_string", formula_tostring }, { nullptr, nullptr },
};

// ============================================================================
// XLCellValue
// ============================================================================
int cellvalue_new(lua_State *L) {
    OX_TRY
    push<XLCellValue>(L, MT_CELLVALUE, check_cellvalue(L, 1));  // no argument -> Empty
    return 1;
    OX_CATCH_ALL
}

int cellvalue_get(lua_State *L) {
    OX_TRY
    push_cellvalue(L, *check<XLCellValue>(L, 1, MT_CELLVALUE));
    return 1;
    OX_CATCH_ALL
}

int cellvalue_set(lua_State *L) {
    OX_TRY
    *check<XLCellValue>(L, 1, MT_CELLVALUE) = check_cellvalue(L, 2);
    return 0;
    OX_CATCH_ALL
}

int cellvalue_type(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, int(check<XLCellValue>(L, 1, MT_CELLVALUE)->type()));
    return 1;
    OX_CATCH_ALL
}

int cellvalue_type_as_string(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCellValue>(L, 1, MT_CELLVALUE)->typeAsString());
    return 1;
    OX_CATCH_ALL
}

int cellvalue_get_double(lua_State *L) {
    OX_TRY
    lua_pushnumber(L, check<XLCellValue>(L, 1, MT_CELLVALUE)->getDouble());
    return 1;
    OX_CATCH_ALL
}

int cellvalue_get_string(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCellValue>(L, 1, MT_CELLVALUE)->getString());
    return 1;
    OX_CATCH_ALL
}

int cellvalue_get_integer(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCellValue>(L, 1, MT_CELLVALUE)->get<int64_t>()));
    return 1;
    OX_CATCH_ALL
}

int cellvalue_get_boolean(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCellValue>(L, 1, MT_CELLVALUE)->get<bool>());
    return 1;
    OX_CATCH_ALL
}

int cellvalue_get_datetime(lua_State *L) {
    OX_TRY
    push<XLDateTime>(L, MT_DATETIME, check<XLCellValue>(L, 1, MT_CELLVALUE)->get<XLDateTime>());
    return 1;
    OX_CATCH_ALL
}

int cellvalue_clear(lua_State *L) {
    OX_TRY
    check<XLCellValue>(L, 1, MT_CELLVALUE)->clear();
    return 0;
    OX_CATCH_ALL
}

int cellvalue_set_error(lua_State *L) {
    OX_TRY
    check<XLCellValue>(L, 1, MT_CELLVALUE)->setError(check_string(L, 2));
    return 0;
    OX_CATCH_ALL
}

int cellvalue_eq(lua_State *L) {
    OX_TRY
    auto *a = maybe<XLCellValue>(L, 1, MT_CELLVALUE);
    auto *b = maybe<XLCellValue>(L, 2, MT_CELLVALUE);
    lua_pushboolean(L, a != nullptr && b != nullptr && *a == *b);
    return 1;
    OX_CATCH_ALL
}

int cellvalue_lt(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, *check<XLCellValue>(L, 1, MT_CELLVALUE) < *check<XLCellValue>(L, 2, MT_CELLVALUE));
    return 1;
    OX_CATCH_ALL
}

int cellvalue_le(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, *check<XLCellValue>(L, 1, MT_CELLVALUE) <= *check<XLCellValue>(L, 2, MT_CELLVALUE));
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg cellvalue_methods[] = {
    { "get", cellvalue_get },
    { "set", cellvalue_set },
    { "type", cellvalue_type },
    { "type_as_string", cellvalue_type_as_string },
    { "get_double", cellvalue_get_double },
    { "get_string", cellvalue_get_string },
    { "get_integer", cellvalue_get_integer },
    { "get_boolean", cellvalue_get_boolean },
    { "get_datetime", cellvalue_get_datetime },
    { "clear", cellvalue_clear },
    { "set_error", cellvalue_set_error },
    { nullptr, nullptr },
};

// ============================================================================
// XLCell (with property sugar: cell.value / cell.formula / ...)
// ============================================================================
int cell_property_get(lua_State *L) {
    OX_TRY
    auto *cell = check<XLCell>(L, 1, MT_CELL);
    const char *key = lua_tostring(L, 2);
    if (key == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    if (std::strcmp(key, "value") == 0) {
        push_cellvalue_proxy(L, cell->value());
        return 1;
    }
    if (std::strcmp(key, "formula") == 0) {
        push_string(L, cell->formula().get());
        return 1;
    }
    if (std::strcmp(key, "address") == 0) {
        push_string(L, cell->cellReference().address());
        return 1;
    }
    if (std::strcmp(key, "has_formula") == 0) {
        lua_pushboolean(L, cell->hasFormula());
        return 1;
    }
    if (std::strcmp(key, "empty") == 0) {
        lua_pushboolean(L, cell->empty());
        return 1;
    }
    if (std::strcmp(key, "format") == 0) {
        lua_pushinteger(L, lua_Integer(cell->cellFormat()));
        return 1;
    }
    if (std::strcmp(key, "reference") == 0) {
        push<XLCellReference>(L, MT_REFERENCE, cell->cellReference());
        return 1;
    }
    if (std::strcmp(key, "value_type") == 0) {
        lua_pushinteger(L, int(cell->value().type()));
        return 1;
    }
    lua_pushnil(L);
    return 1;
    OX_CATCH_ALL
}

int cell_index(lua_State *L) {
    if (lua_type(L, 2) == LUA_TSTRING) {
        lua_pushvalue(L, lua_upvalueindex(1));  // methods table
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (!lua_isnil(L, -1)) {
            lua_remove(L, -2);
            return 1;
        }
        lua_pop(L, 2);
    }
    return cell_property_get(L);
}

int cell_newindex(lua_State *L) {
    OX_TRY
    auto *cell = check<XLCell>(L, 1, MT_CELL);
    const char *key = luaL_checkstring(L, 2);
    if (std::strcmp(key, "value") == 0) {
        if (lua_isnoneornil(L, 3)) {
            cell->value().clear();
        } else if (auto *dt = maybe<XLDateTime>(L, 3, MT_DATETIME)) {
            cell->value() = *dt;
        } else if (auto *cv = maybe<XLCellValue>(L, 3, MT_CELLVALUE)) {
            cell->value() = *cv;
        } else {
            switch (lua_type(L, 3)) {
                case LUA_TBOOLEAN:
                    cell->value() = (lua_toboolean(L, 3) != 0);
                    break;
                case LUA_TNUMBER:
                    if (lua_isinteger(L, 3)) cell->value() = int64_t(lua_tointeger(L, 3));
                    else cell->value() = double(lua_tonumber(L, 3));
                    break;
                case LUA_TSTRING: {
                    size_t n = 0;
                    const char *s = lua_tolstring(L, 3, &n);
                    cell->value() = std::string(s, n);
                    break;
                }
                default:
                    return luaL_error(L, "cell.value: unsupported value type '%s'", luaL_typename(L, 3));
            }
        }
        return 0;
    }
    if (std::strcmp(key, "formula") == 0) {
        if (lua_isnoneornil(L, 3)) cell->formula().clear();
        else cell->formula() = check_string(L, 3);
        return 0;
    }
    if (std::strcmp(key, "format") == 0) {
        cell->setCellFormat(XLStyleIndex(luaL_checkinteger(L, 3)));
        return 0;
    }
    return luaL_error(L, "cell property '%s' is read-only or does not exist", key);
    OX_CATCH_ALL
}

int cell_get_value(lua_State *L) {
    OX_TRY
    push_cellvalue_proxy(L, check<XLCell>(L, 1, MT_CELL)->value());
    return 1;
    OX_CATCH_ALL
}

int cell_set_value(lua_State *L) {
    OX_TRY
    auto *cell = check<XLCell>(L, 1, MT_CELL);
    cell->value() = check_cellvalue(L, 2);
    return 0;
    OX_CATCH_ALL
}

int cell_get_formula(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCell>(L, 1, MT_CELL)->formula().get());
    return 1;
    OX_CATCH_ALL
}

int cell_set_formula(lua_State *L) {
    OX_TRY
    check<XLCell>(L, 1, MT_CELL)->formula() = check_string(L, 2);
    return 0;
    OX_CATCH_ALL
}

int cell_clear_formula(lua_State *L) {
    OX_TRY
    check<XLCell>(L, 1, MT_CELL)->formula().clear();
    return 0;
    OX_CATCH_ALL
}

int cell_has_formula(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCell>(L, 1, MT_CELL)->hasFormula());
    return 1;
    OX_CATCH_ALL
}

int cell_value_type(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, int(check<XLCell>(L, 1, MT_CELL)->value().type()));
    return 1;
    OX_CATCH_ALL
}

int cell_value_type_name(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCell>(L, 1, MT_CELL)->value().typeAsString());
    return 1;
    OX_CATCH_ALL
}

int cell_empty(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCell>(L, 1, MT_CELL)->empty());
    return 1;
    OX_CATCH_ALL
}

int cell_clear(lua_State *L) {
    OX_TRY
    check<XLCell>(L, 1, MT_CELL)->clear(uint32_t(opt_integer(L, 2, 0)));
    return 0;
    OX_CATCH_ALL
}

int cell_reference(lua_State *L) {
    OX_TRY
    push<XLCellReference>(L, MT_REFERENCE, check<XLCell>(L, 1, MT_CELL)->cellReference());
    return 1;
    OX_CATCH_ALL
}

int cell_address(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCell>(L, 1, MT_CELL)->cellReference().address());
    return 1;
    OX_CATCH_ALL
}

int cell_offset(lua_State *L) {
    OX_TRY
    auto *cell = check<XLCell>(L, 1, MT_CELL);
    push<XLCell>(L, MT_CELL, cell->offset(uint16_t(luaL_checkinteger(L, 2)), uint16_t(luaL_checkinteger(L, 3))));
    return 1;
    OX_CATCH_ALL
}

int cell_format(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCell>(L, 1, MT_CELL)->cellFormat()));
    return 1;
    OX_CATCH_ALL
}

int cell_set_format(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCell>(L, 1, MT_CELL)->setCellFormat(XLStyleIndex(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

int cell_get_string(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCell>(L, 1, MT_CELL)->getString());
    return 1;
    OX_CATCH_ALL
}

int cell_copy_from(lua_State *L) {
    OX_TRY
    check<XLCell>(L, 1, MT_CELL)->copyFrom(*check<XLCell>(L, 2, MT_CELL));
    return 0;
    OX_CATCH_ALL
}

const luaL_Reg cell_methods[] = {
    { "get_value", cell_get_value },
    { "set_value", cell_set_value },
    { "get_formula", cell_get_formula },
    { "set_formula", cell_set_formula },
    { "clear_formula", cell_clear_formula },
    { "has_formula", cell_has_formula },
    { "value_type", cell_value_type },
    { "value_type_name", cell_value_type_name },
    { "empty", cell_empty },
    { "clear", cell_clear },
    { "reference", cell_reference },
    { "address", cell_address },
    { "offset", cell_offset },
    { "format", cell_format },
    { "set_format", cell_set_format },
    { "get_string", cell_get_string },
    { "copy_from", cell_copy_from },
    { nullptr, nullptr },
};

// ============================================================================
// XLRow (with property sugar: row.values)
// ============================================================================
int row_property_get(lua_State *L) {
    OX_TRY
    auto *row = check<XLRow>(L, 1, MT_ROW);
    const char *key = lua_tostring(L, 2);
    if (key == nullptr) {
        lua_pushnil(L);
        return 1;
    }
    if (std::strcmp(key, "values") == 0) {
        const std::vector<XLCellValue> values = row->values();
        push_value_array(L, values);
        return 1;
    }
    lua_pushnil(L);
    return 1;
    OX_CATCH_ALL
}

int row_index(lua_State *L) {
    if (lua_type(L, 2) == LUA_TSTRING) {
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (!lua_isnil(L, -1)) {
            lua_remove(L, -2);
            return 1;
        }
        lua_pop(L, 2);
    }
    return row_property_get(L);
}

int row_newindex(lua_State *L) {
    OX_TRY
    auto *row = check<XLRow>(L, 1, MT_ROW);
    const char *key = luaL_checkstring(L, 2);
    if (std::strcmp(key, "values") == 0) {
        std::vector<XLCellValue> values = check_value_array(L, 3);
        row->values() = values;
        return 0;
    }
    return luaL_error(L, "row property '%s' is read-only or does not exist", key);
    OX_CATCH_ALL
}

int row_height(lua_State *L) {
    OX_TRY
    lua_pushnumber(L, check<XLRow>(L, 1, MT_ROW)->height());
    return 1;
    OX_CATCH_ALL
}
int row_set_height(lua_State *L) {
    OX_TRY
    check<XLRow>(L, 1, MT_ROW)->setHeight(float(luaL_checknumber(L, 2)));
    return 0;
    OX_CATCH_ALL
}
int row_descent(lua_State *L) {
    OX_TRY
    lua_pushnumber(L, check<XLRow>(L, 1, MT_ROW)->descent());
    return 1;
    OX_CATCH_ALL
}
int row_set_descent(lua_State *L) {
    OX_TRY
    check<XLRow>(L, 1, MT_ROW)->setDescent(float(luaL_checknumber(L, 2)));
    return 0;
    OX_CATCH_ALL
}
int row_hidden(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLRow>(L, 1, MT_ROW)->isHidden());
    return 1;
    OX_CATCH_ALL
}
int row_set_hidden(lua_State *L) {
    OX_TRY
    check<XLRow>(L, 1, MT_ROW)->setHidden(lua_toboolean(L, 2) != 0);
    return 0;
    OX_CATCH_ALL
}
int row_row_number(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLRow>(L, 1, MT_ROW)->rowNumber()));
    return 1;
    OX_CATCH_ALL
}
int row_cell_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLRow>(L, 1, MT_ROW)->cellCount()));
    return 1;
    OX_CATCH_ALL
}
int row_empty(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLRow>(L, 1, MT_ROW)->empty());
    return 1;
    OX_CATCH_ALL
}
int row_get_values(lua_State *L) {
    OX_TRY
    const std::vector<XLCellValue> values = check<XLRow>(L, 1, MT_ROW)->values();
    push_value_array(L, values);
    return 1;
    OX_CATCH_ALL
}
int row_set_values(lua_State *L) {
    OX_TRY
    auto *row = check<XLRow>(L, 1, MT_ROW);
    std::vector<XLCellValue> values = check_value_array(L, 2);
    row->values() = values;
    return 0;
    OX_CATCH_ALL
}
int row_clear_values(lua_State *L) {
    OX_TRY
    check<XLRow>(L, 1, MT_ROW)->values().clear();
    return 0;
    OX_CATCH_ALL
}
int row_find_cell(lua_State *L) {
    OX_TRY
    auto *row = check<XLRow>(L, 1, MT_ROW);
    const lua_Integer col = luaL_checkinteger(L, 2);
    check_row_col_range(L, 1, col);
    push<XLCell>(L, MT_CELL, row->findCell(uint16_t(col)));
    return 1;
    OX_CATCH_ALL
}
int row_format(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLRow>(L, 1, MT_ROW)->format()));
    return 1;
    OX_CATCH_ALL
}
int row_set_format(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLRow>(L, 1, MT_ROW)->setFormat(XLStyleIndex(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

// forward declaration (definition is further below)
int row_cells(lua_State *L);

const luaL_Reg row_methods[] = {
    { "height", row_height },
    { "set_height", row_set_height },
    { "descent", row_descent },
    { "set_descent", row_set_descent },
    { "is_hidden", row_hidden },
    { "set_hidden", row_set_hidden },
    { "row_number", row_row_number },
    { "cell_count", row_cell_count },
    { "empty", row_empty },
    { "get_values", row_get_values },
    { "set_values", row_set_values },
    { "clear_values", row_clear_values },
    { "cells", row_cells },
    { "find_cell", row_find_cell },
    { "format", row_format },
    { "set_format", row_set_format },
    { nullptr, nullptr },
};

// ============================================================================
// XLColumn
// ============================================================================
int column_width(lua_State *L) {
    OX_TRY
    lua_pushnumber(L, check<XLColumn>(L, 1, MT_COLUMN)->width());
    return 1;
    OX_CATCH_ALL
}
int column_set_width(lua_State *L) {
    OX_TRY
    check<XLColumn>(L, 1, MT_COLUMN)->setWidth(float(luaL_checknumber(L, 2)));
    return 0;
    OX_CATCH_ALL
}
int column_hidden(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLColumn>(L, 1, MT_COLUMN)->isHidden());
    return 1;
    OX_CATCH_ALL
}
int column_set_hidden(lua_State *L) {
    OX_TRY
    check<XLColumn>(L, 1, MT_COLUMN)->setHidden(lua_toboolean(L, 2) != 0);
    return 0;
    OX_CATCH_ALL
}
int column_format(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLColumn>(L, 1, MT_COLUMN)->format()));
    return 1;
    OX_CATCH_ALL
}
int column_set_format(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLColumn>(L, 1, MT_COLUMN)->setFormat(XLStyleIndex(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg column_methods[] = {
    { "width", column_width },        { "set_width", column_set_width },
    { "is_hidden", column_hidden },   { "set_hidden", column_set_hidden },
    { "format", column_format },      { "set_format", column_set_format },
    { nullptr, nullptr },
};

// ============================================================================
// Iterators
// ============================================================================
int cell_iterator_next(lua_State *L) {
    OX_TRY
    ++(*check<XLCellIterator>(L, 1, MT_CELL_ITERATOR));
    return 0;
    OX_CATCH_ALL
}
int cell_iterator_cell(lua_State *L) {
    OX_TRY
    push_live<XLCell>(L, MT_CELL, token_of(L, 1), XLCell(**check<XLCellIterator>(L, 1, MT_CELL_ITERATOR)));
    return 1;
    OX_CATCH_ALL
}
int cell_iterator_address(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCellIterator>(L, 1, MT_CELL_ITERATOR)->address());
    return 1;
    OX_CATCH_ALL
}
int cell_iterator_at_end(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCellIterator>(L, 1, MT_CELL_ITERATOR)->endReached());
    return 1;
    OX_CATCH_ALL
}
int cell_iterator_cell_exists(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCellIterator>(L, 1, MT_CELL_ITERATOR)->cellExists());
    return 1;
    OX_CATCH_ALL
}
int cell_iterator_distance(lua_State *L) {
    OX_TRY
    auto *it = check<XLCellIterator>(L, 1, MT_CELL_ITERATOR);
    auto *other = check<XLCellIterator>(L, 2, MT_CELL_ITERATOR);
    lua_pushinteger(L, lua_Integer(it->distance(*other)));
    return 1;
    OX_CATCH_ALL
}
int cell_iterator_equals(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, *check<XLCellIterator>(L, 1, MT_CELL_ITERATOR) ==
                           *check<XLCellIterator>(L, 2, MT_CELL_ITERATOR));
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg cell_iterator_methods[] = {
    { "next", cell_iterator_next },       { "cell", cell_iterator_cell },
    { "address", cell_iterator_address }, { "at_end", cell_iterator_at_end },
    { "cell_exists", cell_iterator_cell_exists },
    { "distance", cell_iterator_distance }, { "equals", cell_iterator_equals },
    { nullptr, nullptr },
};

// ---- closure producers ------------------------------------------------------
int cell_range_next(lua_State *L) {
    OX_TRY
    if (iter_at_end<XLCellIterator>(L, MT_CELL_ITERATOR)) {
        lua_pushnil(L);
        return 1;
    }
    auto *it = check<XLCellIterator>(L, lua_upvalueindex(1), MT_CELL_ITERATOR);
    push_live<XLCell>(L, MT_CELL, token_of(L, lua_upvalueindex(1)), XLCell(**it));
    ++(*it);
    return 1;
    OX_CATCH_ALL
}

int row_iterator_next(lua_State *L) {
    OX_TRY
    ++(*check<XLRowIterator>(L, 1, MT_ROW_ITERATOR));
    return 0;
    OX_CATCH_ALL
}
int row_iterator_row(lua_State *L) {
    OX_TRY
    push_live<XLRow>(L, MT_ROW, token_of(L, 1), XLRow(**check<XLRowIterator>(L, 1, MT_ROW_ITERATOR)));
    return 1;
    OX_CATCH_ALL
}
int row_iterator_at_end(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLRowIterator>(L, 1, MT_ROW_ITERATOR)->endReached());
    return 1;
    OX_CATCH_ALL
}
int row_iterator_row_exists(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLRowIterator>(L, 1, MT_ROW_ITERATOR)->rowExists());
    return 1;
    OX_CATCH_ALL
}
int row_iterator_row_number(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLRowIterator>(L, 1, MT_ROW_ITERATOR)->rowNumber()));
    return 1;
    OX_CATCH_ALL
}
int row_iterator_equals(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, *check<XLRowIterator>(L, 1, MT_ROW_ITERATOR) ==
                           *check<XLRowIterator>(L, 2, MT_ROW_ITERATOR));
    return 1;
    OX_CATCH_ALL
}
int row_iterator_direction(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, int(check<XLRowIterator>(L, 1, MT_ROW_ITERATOR)->direction()));
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg row_iterator_methods[] = {
    { "next", row_iterator_next },       { "row", row_iterator_row },
    { "at_end", row_iterator_at_end },   { "row_exists", row_iterator_row_exists },
    { "row_number", row_iterator_row_number }, { "equals", row_iterator_equals },
    { "direction", row_iterator_direction },
    { nullptr, nullptr },
};

int row_reverse_iterator_next(lua_State *L) {
    OX_TRY
    ++(*check<XLRowReverseIterator>(L, 1, MT_ROW_REVERSE_ITERATOR));
    return 0;
    OX_CATCH_ALL
}
int row_reverse_iterator_row(lua_State *L) {
    OX_TRY
    push_live<XLRow>(L, MT_ROW, token_of(L, 1),
                     XLRow(**check<XLRowReverseIterator>(L, 1, MT_ROW_REVERSE_ITERATOR)));
    return 1;
    OX_CATCH_ALL
}
int row_reverse_iterator_at_end(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLRowReverseIterator>(L, 1, MT_ROW_REVERSE_ITERATOR)->endReached());
    return 1;
    OX_CATCH_ALL
}
int row_reverse_iterator_row_exists(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLRowReverseIterator>(L, 1, MT_ROW_REVERSE_ITERATOR)->rowExists());
    return 1;
    OX_CATCH_ALL
}
int row_reverse_iterator_row_number(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLRowReverseIterator>(L, 1, MT_ROW_REVERSE_ITERATOR)->rowNumber()));
    return 1;
    OX_CATCH_ALL
}
int row_reverse_iterator_equals(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, *check<XLRowReverseIterator>(L, 1, MT_ROW_REVERSE_ITERATOR) ==
                           *check<XLRowReverseIterator>(L, 2, MT_ROW_REVERSE_ITERATOR));
    return 1;
    OX_CATCH_ALL
}
int row_reverse_iterator_direction(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, int(check<XLRowReverseIterator>(L, 1, MT_ROW_REVERSE_ITERATOR)->direction()));
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg row_reverse_iterator_methods[] = {
    { "next", row_reverse_iterator_next },
    { "row", row_reverse_iterator_row },
    { "at_end", row_reverse_iterator_at_end },
    { "row_exists", row_reverse_iterator_row_exists },
    { "row_number", row_reverse_iterator_row_number },
    { "equals", row_reverse_iterator_equals },
    { "direction", row_reverse_iterator_direction },
    { nullptr, nullptr },
};

int row_range_forward_next(lua_State *L) {
    OX_TRY
    if (iter_at_end<XLRowIterator>(L, MT_ROW_ITERATOR)) {
        lua_pushnil(L);
        return 1;
    }
    auto *it = check<XLRowIterator>(L, lua_upvalueindex(1), MT_ROW_ITERATOR);
    push_live<XLRow>(L, MT_ROW, token_of(L, lua_upvalueindex(1)), XLRow(**it));
    ++(*it);
    return 1;
    OX_CATCH_ALL
}

int row_range_reverse_next(lua_State *L) {
    OX_TRY
    if (iter_at_end<XLRowReverseIterator>(L, MT_ROW_REVERSE_ITERATOR)) {
        lua_pushnil(L);
        return 1;
    }
    auto *it = check<XLRowReverseIterator>(L, lua_upvalueindex(1), MT_ROW_REVERSE_ITERATOR);
    push_live<XLRow>(L, MT_ROW, token_of(L, lua_upvalueindex(1)), XLRow(**it));
    ++(*it);
    return 1;
    OX_CATCH_ALL
}

int row_data_iterator_next(lua_State *L) {
    OX_TRY
    ++(*check<XLRowDataIterator>(L, 1, MT_ROW_DATA_ITERATOR));
    return 0;
    OX_CATCH_ALL
}
int row_data_iterator_cell(lua_State *L) {
    OX_TRY
    push_live<XLCell>(L, MT_CELL, token_of(L, 1), XLCell(**check<XLRowDataIterator>(L, 1, MT_ROW_DATA_ITERATOR)));
    return 1;
    OX_CATCH_ALL
}
int row_data_iterator_equals(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, *check<XLRowDataIterator>(L, 1, MT_ROW_DATA_ITERATOR) ==
                           *check<XLRowDataIterator>(L, 2, MT_ROW_DATA_ITERATOR));
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg row_data_iterator_methods[] = {
    { "next", row_data_iterator_next },
    { "cell", row_data_iterator_cell },
    { "equals", row_data_iterator_equals },
    { nullptr, nullptr },
};

int row_data_range_forward_next(lua_State *L) {
    OX_TRY
    if (iter_at_end<XLRowDataIterator>(L, MT_ROW_DATA_ITERATOR)) {
        lua_pushnil(L);
        return 1;
    }
    auto *it = check<XLRowDataIterator>(L, lua_upvalueindex(1), MT_ROW_DATA_ITERATOR);
    push_live<XLCell>(L, MT_CELL, token_of(L, lua_upvalueindex(1)), XLCell(**it));
    ++(*it);
    return 1;
    OX_CATCH_ALL
}

// ---- XLRowDataRange --------------------------------------------------------
/** Convenience: build a row data range for the given row/args and push it. */
int row_cells(lua_State *L) {
    OX_TRY
    auto *row = check<XLRow>(L, 1, MT_ROW);
    const auto token = token_of(L, 1);
    if (lua_gettop(L) >= 3) {
        push_live<XLRowDataRange>(L, MT_ROW_DATA_RANGE, token,
                                  row->cells(uint16_t(luaL_checkinteger(L, 2)),
                                             uint16_t(luaL_checkinteger(L, 3))));
    } else if (lua_gettop(L) == 2) {
        push_live<XLRowDataRange>(L, MT_ROW_DATA_RANGE, token,
                                  row->cells(uint16_t(luaL_checkinteger(L, 2))));
    } else {
        push_live<XLRowDataRange>(L, MT_ROW_DATA_RANGE, token, row->cells());
    }
    return 1;
    OX_CATCH_ALL
}

int row_data_range_size(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLRowDataRange>(L, 1, MT_ROW_DATA_RANGE)->size()));
    return 1;
    OX_CATCH_ALL
}

int row_data_range_each(lua_State *L) {
    OX_TRY
    auto *range = check<XLRowDataRange>(L, 1, MT_ROW_DATA_RANGE);
    XLRowDataIterator begin = range->begin();
    XLRowDataIterator end = range->end();
    push_closure_iterator(L, MT_ROW_DATA_ITERATOR, token_of(L, 1), begin, end, row_data_range_forward_next);
    return 1;
    OX_CATCH_ALL
}

int row_data_range_set_value(lua_State *L) {
    OX_TRY
    auto *range = check<XLRowDataRange>(L, 1, MT_ROW_DATA_RANGE);
    if (lua_isnoneornil(L, 2)) {
        for (auto it = range->begin(); it != range->end(); ++it) it->value().clear();
    } else if (auto *dt = maybe<XLDateTime>(L, 2, MT_DATETIME)) {
        *range = *dt;
    } else {
        switch (lua_type(L, 2)) {
            case LUA_TBOOLEAN: *range = (lua_toboolean(L, 2) != 0); break;
            case LUA_TNUMBER:
                if (lua_isinteger(L, 2)) *range = int64_t(lua_tointeger(L, 2));
                else *range = double(lua_tonumber(L, 2));
                break;
            case LUA_TSTRING: *range = check_string(L, 2); break;
            default: return luaL_error(L, "row cells:set_value: unsupported value type '%s'", luaL_typename(L, 2));
        }
    }
    return 0;
    OX_CATCH_ALL
}

int row_data_range_begin(lua_State *L) {
    OX_TRY
    push_live<XLRowDataIterator>(L, MT_ROW_DATA_ITERATOR, token_of(L, 1),
                                 check<XLRowDataRange>(L, 1, MT_ROW_DATA_RANGE)->begin());
    return 1;
    OX_CATCH_ALL
}

int row_data_range_end(lua_State *L) {
    OX_TRY
    push_live<XLRowDataIterator>(L, MT_ROW_DATA_ITERATOR, token_of(L, 1),
                                 check<XLRowDataRange>(L, 1, MT_ROW_DATA_RANGE)->end());
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg row_data_range_methods[] = {
    { "size", row_data_range_size },
    { "each", row_data_range_each },
    { "set_value", row_data_range_set_value },
    { "begin", row_data_range_begin },
    { "end", row_data_range_end },
    { nullptr, nullptr },
};

// ============================================================================
// XLRowRange
// ============================================================================
int row_range_row_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLRowRange>(L, 1, MT_ROW_RANGE)->rowCount()));
    return 1;
    OX_CATCH_ALL
}

int row_range_len(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLRowRange>(L, 1, MT_ROW_RANGE)->rowCount()));
    return 1;
    OX_CATCH_ALL
}

int row_range_each(lua_State *L) {
    OX_TRY
    auto *range = check<XLRowRange>(L, 1, MT_ROW_RANGE);
    XLRowIterator begin = range->begin();
    XLRowIterator end = range->end();
    push_closure_iterator(L, MT_ROW_ITERATOR, token_of(L, 1), begin, end, row_range_forward_next);
    return 1;
    OX_CATCH_ALL
}

int row_range_each_reverse(lua_State *L) {
    OX_TRY
    auto *range = check<XLRowRange>(L, 1, MT_ROW_RANGE);
    XLRowReverseIterator begin = range->rbegin();
    XLRowReverseIterator end = range->rend();
    push_closure_iterator(L, MT_ROW_REVERSE_ITERATOR, token_of(L, 1), begin, end, row_range_reverse_next);
    return 1;
    OX_CATCH_ALL
}

int row_range_begin(lua_State *L) {
    OX_TRY
    push_live<XLRowIterator>(L, MT_ROW_ITERATOR, token_of(L, 1),
                             check<XLRowRange>(L, 1, MT_ROW_RANGE)->begin());
    return 1;
    OX_CATCH_ALL
}

int row_range_end(lua_State *L) {
    OX_TRY
    push_live<XLRowIterator>(L, MT_ROW_ITERATOR, token_of(L, 1),
                             check<XLRowRange>(L, 1, MT_ROW_RANGE)->end());
    return 1;
    OX_CATCH_ALL
}

int row_range_rbegin(lua_State *L) {
    OX_TRY
    push_live<XLRowReverseIterator>(L, MT_ROW_REVERSE_ITERATOR, token_of(L, 1),
                                    check<XLRowRange>(L, 1, MT_ROW_RANGE)->rbegin());
    return 1;
    OX_CATCH_ALL
}

int row_range_rend(lua_State *L) {
    OX_TRY
    push_live<XLRowReverseIterator>(L, MT_ROW_REVERSE_ITERATOR, token_of(L, 1),
                                    check<XLRowRange>(L, 1, MT_ROW_RANGE)->rend());
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg row_range_methods[] = {
    { "row_count", row_range_row_count },
    { "each", row_range_each },
    { "each_reverse", row_range_each_reverse },
    { "begin", row_range_begin },
    { "end", row_range_end },
    { "rbegin", row_range_rbegin },
    { "rend", row_range_rend },
    { nullptr, nullptr },
};

// ============================================================================
// XLCellRange
// ============================================================================
int range_top_left(lua_State *L) {
    OX_TRY
    push<XLCellReference>(L, MT_REFERENCE, check<XLCellRange>(L, 1, MT_RANGE)->topLeft());
    return 1;
    OX_CATCH_ALL
}

int range_bottom_right(lua_State *L) {
    OX_TRY
    push<XLCellReference>(L, MT_REFERENCE, check<XLCellRange>(L, 1, MT_RANGE)->bottomRight());
    return 1;
    OX_CATCH_ALL
}

int range_address(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCellRange>(L, 1, MT_RANGE)->address());
    return 1;
    OX_CATCH_ALL
}

int range_num_rows(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCellRange>(L, 1, MT_RANGE)->numRows()));
    return 1;
    OX_CATCH_ALL
}

int range_num_columns(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCellRange>(L, 1, MT_RANGE)->numColumns()));
    return 1;
    OX_CATCH_ALL
}

int range_clear(lua_State *L) {
    OX_TRY
    check<XLCellRange>(L, 1, MT_RANGE)->clear();
    return 0;
    OX_CATCH_ALL
}

int range_set_format(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCellRange>(L, 1, MT_RANGE)->setFormat(XLStyleIndex(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

int range_fetch_column_styles(lua_State *L) {
    OX_TRY
    check<XLCellRange>(L, 1, MT_RANGE)->fetchColumnStyles();
    return 0;
    OX_CATCH_ALL
}

int range_set_value(lua_State *L) {
    OX_TRY
    auto *range = check<XLCellRange>(L, 1, MT_RANGE);
    if (lua_isnoneornil(L, 2)) {
        for (auto it = range->begin(); it != range->end(); ++it) it->value().clear();
    } else if (auto *dt = maybe<XLDateTime>(L, 2, MT_DATETIME)) {
        *range = *dt;
    } else {
        switch (lua_type(L, 2)) {
            case LUA_TBOOLEAN: *range = (lua_toboolean(L, 2) != 0); break;
            case LUA_TNUMBER:
                if (lua_isinteger(L, 2)) *range = int64_t(lua_tointeger(L, 2));
                else *range = double(lua_tonumber(L, 2));
                break;
            case LUA_TSTRING: *range = check_string(L, 2); break;
            default: return luaL_error(L, "range:set_value: unsupported value type '%s'", luaL_typename(L, 2));
        }
    }
    return 0;
    OX_CATCH_ALL
}

int range_each(lua_State *L) {
    OX_TRY
    auto *range = check<XLCellRange>(L, 1, MT_RANGE);
    XLCellIterator begin = range->begin();
    XLCellIterator end = range->end();
    push_closure_iterator(L, MT_CELL_ITERATOR, token_of(L, 1), begin, end, cell_range_next);
    return 1;
    OX_CATCH_ALL
}

int range_begin(lua_State *L) {
    OX_TRY
    push_live<XLCellIterator>(L, MT_CELL_ITERATOR, token_of(L, 1),
                              check<XLCellRange>(L, 1, MT_RANGE)->begin());
    return 1;
    OX_CATCH_ALL
}

int range_end(lua_State *L) {
    OX_TRY
    push_live<XLCellIterator>(L, MT_CELL_ITERATOR, token_of(L, 1),
                              check<XLCellRange>(L, 1, MT_RANGE)->end());
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg range_methods[] = {
    { "top_left", range_top_left },
    { "bottom_right", range_bottom_right },
    { "address", range_address },
    { "num_rows", range_num_rows },
    { "num_columns", range_num_columns },
    { "clear", range_clear },
    { "set_format", range_set_format },
    { "fetch_column_styles", range_fetch_column_styles },
    { "set_value", range_set_value },
    { "each", range_each },
    { "begin", range_begin },
    { "end", range_end },
    { nullptr, nullptr },
};

// ============================================================================
// XLMergeCells
// ============================================================================
int merges_valid(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLMergeCells>(L, 1, MT_MERGES)->valid());
    return 1;
    OX_CATCH_ALL
}
int merges_find_merge(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLMergeCells>(L, 1, MT_MERGES)->findMerge(check_range_address(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int merges_merge_exists(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLMergeCells>(L, 1, MT_MERGES)->mergeExists(check_range_address(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int merges_find_merge_by_cell(lua_State *L) {
    OX_TRY
    auto *merges = check<XLMergeCells>(L, 1, MT_MERGES);
    if (auto *ref = maybe<XLCellReference>(L, 2, MT_REFERENCE)) {
        lua_pushinteger(L, lua_Integer(merges->findMergeByCell(*ref)));
    } else {
        lua_pushinteger(L, lua_Integer(merges->findMergeByCell(check_cell_address(L, 2))));
    }
    return 1;
    OX_CATCH_ALL
}
int merges_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLMergeCells>(L, 1, MT_MERGES)->count()));
    return 1;
    OX_CATCH_ALL
}
int merges_len(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLMergeCells>(L, 1, MT_MERGES)->count()));
    return 1;
    OX_CATCH_ALL
}
int merges_merge(lua_State *L) {
    OX_TRY
    const char *ref = check<XLMergeCells>(L, 1, MT_MERGES)->merge(XLMergeIndex(luaL_checkinteger(L, 2)));
    lua_pushstring(L, ref != nullptr ? ref : "");
    return 1;
    OX_CATCH_ALL
}
int merges_append_merge(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLMergeCells>(L, 1, MT_MERGES)->appendMerge(check_range_address(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int merges_delete_merge(lua_State *L) {
    OX_TRY
    check<XLMergeCells>(L, 1, MT_MERGES)->deleteMerge(XLMergeIndex(luaL_checkinteger(L, 2)));
    return 0;
    OX_CATCH_ALL
}
int merges_delete_all(lua_State *L) {
    OX_TRY
    check<XLMergeCells>(L, 1, MT_MERGES)->deleteAll();
    return 0;
    OX_CATCH_ALL
}

const luaL_Reg merges_methods[] = {
    { "valid", merges_valid },
    { "find_merge", merges_find_merge },
    { "merge_exists", merges_merge_exists },
    { "find_merge_by_cell", merges_find_merge_by_cell },
    { "count", merges_count },
    { "merge", merges_merge },
    { "append_merge", merges_append_merge },
    { "delete_merge", merges_delete_merge },
    { "delete_all", merges_delete_all },
    { nullptr, nullptr },
};

}  // namespace

// ============================================================================
// Module-level functions & type registration
// ============================================================================
namespace ox {

void bind_data(lua_State *L) {
    // ---- register metatables ----
    register_type<XLCellReference>(L, MT_REFERENCE, ref_methods);
    register_type<XLDateTime>(L, MT_DATETIME, datetime_methods);
    register_type<XLColor>(L, MT_COLOR, color_methods);
    register_type<XLFormula>(L, MT_FORMULA, formula_methods);
    register_type<XLCellValue>(L, MT_CELLVALUE, cellvalue_methods);
    register_type<XLCell>(L, MT_CELL, cell_methods, cell_index, cell_newindex);
    register_type<XLRow>(L, MT_ROW, row_methods, row_index, row_newindex);
    register_type<XLColumn>(L, MT_COLUMN, column_methods);
    register_type<XLCellRange>(L, MT_RANGE, range_methods);
    register_type<XLRowRange>(L, MT_ROW_RANGE, row_range_methods);
    register_type<XLRowDataRange>(L, MT_ROW_DATA_RANGE, row_data_range_methods);
    register_type<XLMergeCells>(L, MT_MERGES, merges_methods);
    register_type<XLCellIterator>(L, MT_CELL_ITERATOR, cell_iterator_methods);
    register_type<XLRowIterator>(L, MT_ROW_ITERATOR, row_iterator_methods);
    register_type<XLRowReverseIterator>(L, MT_ROW_REVERSE_ITERATOR, row_reverse_iterator_methods);
    register_type<XLRowDataIterator>(L, MT_ROW_DATA_ITERATOR, row_data_iterator_methods);

    // ---- __len metamethods ----
    // (set on the metatables created above)
    luaL_getmetatable(L, MT_ROW_RANGE);
    lua_pushcfunction(L, row_range_len);
    lua_setfield(L, -2, "__len");
    lua_pop(L, 1);
    luaL_getmetatable(L, MT_MERGES);
    lua_pushcfunction(L, merges_len);
    lua_setfield(L, -2, "__len");
    lua_pop(L, 1);

    // ---- __eq / __lt / __le for reference & value ----
    luaL_getmetatable(L, MT_REFERENCE);
    lua_pushcfunction(L, ref_eq);
    lua_setfield(L, -2, "__eq");
    lua_pushcfunction(L, ref_lt);
    lua_setfield(L, -2, "__lt");
    lua_pushcfunction(L, ref_le);
    lua_setfield(L, -2, "__le");
    lua_pushcfunction(L, ref_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pop(L, 1);

    luaL_getmetatable(L, MT_CELLVALUE);
    lua_pushcfunction(L, cellvalue_eq);
    lua_setfield(L, -2, "__eq");
    lua_pushcfunction(L, cellvalue_lt);
    lua_setfield(L, -2, "__lt");
    lua_pushcfunction(L, cellvalue_le);
    lua_setfield(L, -2, "__le");
    lua_pop(L, 1);

    luaL_getmetatable(L, MT_COLOR);
    lua_pushcfunction(L, color_eq);
    lua_setfield(L, -2, "__eq");
    lua_pushcfunction(L, color_hex);
    lua_setfield(L, -2, "__tostring");
    lua_pop(L, 1);

    luaL_getmetatable(L, MT_DATETIME);
    lua_pushcfunction(L, datetime_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pop(L, 1);

    luaL_getmetatable(L, MT_FORMULA);
    lua_pushcfunction(L, formula_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pop(L, 1);

    luaL_getmetatable(L, MT_CELL);
    lua_pushcfunction(L, cell_address);
    lua_setfield(L, -2, "__tostring");
    lua_pop(L, 1);

    // ---- module-level constructors ----
    lua_pushcfunction(L, ref_new);
    lua_setfield(L, 1, "reference");
    lua_pushcfunction(L, datetime_new);
    lua_setfield(L, 1, "datetime");
    lua_pushcfunction(L, datetime_from_unix);
    lua_setfield(L, 1, "datetime_from_unix");
    lua_pushcfunction(L, color_new);
    lua_setfield(L, 1, "color");
    lua_pushcfunction(L, cellvalue_new);
    lua_setfield(L, 1, "cell_value");
    lua_pushcfunction(L, formula_new);
    lua_setfield(L, 1, "formula");
}

}  // namespace ox
