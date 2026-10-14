// =============================================================================
// bind_sheet.cpp — XLWorksheet (incl. protection) and conditional formatting
// =============================================================================
#include "lua_bind.hpp"

#include <cstring>

using namespace OpenXLSX;
using namespace ox;

namespace {

// ============================================================================
// helpers
// ============================================================================
XLCellAssignable ws_get_cell(lua_State *L, XLWorksheet *ws, bool find) {
    if (auto *ref = maybe<XLCellReference>(L, 2, MT_REFERENCE)) {
        return find ? ws->findCell(*ref) : ws->cell(*ref);
    }
    if (lua_type(L, 2) == LUA_TSTRING) {
        const std::string addr = check_cell_address(L, 2);
        return find ? ws->findCell(addr) : ws->cell(addr);
    }
    const lua_Integer row = luaL_checkinteger(L, 2);
    const lua_Integer col = luaL_checkinteger(L, 3);
    check_row_col_range(L, row, col);
    return find ? ws->findCell(uint32_t(row), uint16_t(col)) : ws->cell(uint32_t(row), uint16_t(col));
}

int cf_enum(lua_State *L, int idx, const char *const names[], int count, int (*value_of_name)(int),
            const char *what) {
    if (lua_isinteger(L, idx)) return int(lua_tointeger(L, idx));
    if (lua_type(L, idx) == LUA_TSTRING) {
        const char *s = lua_tostring(L, idx);
        for (int i = 0; i < count; ++i) {
            if (iequal_loose(s, names[i])) return value_of_name(i);
        }
    }
    return luaL_error(L, "invalid %s: expected name or numeric value", what);
}

const char *cf_type_name(int value) {
    for (int i = 0; i < XL_CF_TYPE_COUNT; ++i)
        if (xl_cf_type_value(i) == value) return XL_CF_TYPE_NAMES[i];
    return "unknown";
}
const char *cf_operator_name(int value) {
    for (int i = 0; i < XL_CF_OPERATOR_COUNT; ++i)
        if (xl_cf_operator_value(i) == value) return XL_CF_OPERATOR_NAMES[i];
    return "unknown";
}
const char *cf_time_period_name(int value) {
    for (int i = 0; i < XL_CF_TIME_PERIOD_COUNT; ++i)
        if (xl_cf_time_period_value(i) == value) return XL_CF_TIME_PERIOD_NAMES[i];
    return "unknown";
}

// ============================================================================
// XLWorksheet
// ============================================================================
int ws_cell(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    const XLCellAssignable c = ws_get_cell(L, ws, false);
    push_live<XLCell>(L, MT_CELL, token_of(L, 1), XLCell(c));
    return 1;
    OX_CATCH_ALL
}

int ws_find_cell(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    const XLCellAssignable c = ws_get_cell(L, ws, true);
    push_live<XLCell>(L, MT_CELL, token_of(L, 1), XLCell(c));
    return 1;
    OX_CATCH_ALL
}

int ws_set_dimension(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    const int n = lua_gettop(L) - 1;
    if (n <= 0) {
        ws->setDimension();
    } else if (n == 1) {
        ws->setDimension(check_range_address(L, 2));
    } else if (auto *tl = maybe<XLCellReference>(L, 2, MT_REFERENCE)) {
        ws->setDimension(*tl, *check<XLCellReference>(L, 3, MT_REFERENCE), opt_bool(L, 4, true));
    } else {
        ws->setDimension(check_cell_address(L, 2), check_cell_address(L, 3));
    }
    return 0;
    OX_CATCH_ALL
}

int ws_range(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    const auto token = token_of(L, 1);
    const int n = lua_gettop(L) - 1;
    if (n <= 0) {
        push_live<XLCellRange>(L, MT_RANGE, token, ws->range());
    } else if (n == 1) {
        push_live<XLCellRange>(L, MT_RANGE, token, ws->range(check_range_address(L, 2)));
    } else if (auto *tl = maybe<XLCellReference>(L, 2, MT_REFERENCE)) {
        push_live<XLCellRange>(L, MT_RANGE, token,
                               ws->range(*tl, *check<XLCellReference>(L, 3, MT_REFERENCE)));
    } else {
        push_live<XLCellRange>(L, MT_RANGE, token,
                               ws->range(check_cell_address(L, 2), check_cell_address(L, 3)));
    }
    return 1;
    OX_CATCH_ALL
}

int ws_rows(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    const auto token = token_of(L, 1);
    const int n = lua_gettop(L) - 1;
    if (n <= 0) {
        push_live<XLRowRange>(L, MT_ROW_RANGE, token, ws->rows());
    } else if (n == 1) {
        push_live<XLRowRange>(L, MT_ROW_RANGE, token, ws->rows(uint32_t(luaL_checkinteger(L, 2))));
    } else {
        push_live<XLRowRange>(L, MT_ROW_RANGE, token,
                              ws->rows(uint32_t(luaL_checkinteger(L, 2)), uint32_t(luaL_checkinteger(L, 3))));
    }
    return 1;
    OX_CATCH_ALL
}

int ws_row(lua_State *L) {
    OX_TRY
    const lua_Integer row = luaL_checkinteger(L, 2);
    check_row_col_range(L, row, 1);
    push_live<XLRow>(L, MT_ROW, token_of(L, 1), check<XLWorksheet>(L, 1, MT_WORKSHEET)->row(uint32_t(row)));
    return 1;
    OX_CATCH_ALL
}

int ws_column(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    const auto token = token_of(L, 1);
    if (lua_isinteger(L, 2)) {
        const lua_Integer col = lua_tointeger(L, 2);
        check_row_col_range(L, 1, col);
        push_live<XLColumn>(L, MT_COLUMN, token, ws->column(uint16_t(col)));
    } else {
        push_live<XLColumn>(L, MT_COLUMN, token, ws->column(check_column_letters(L, 2)));
    }
    return 1;
    OX_CATCH_ALL
}

int ws_last_cell(lua_State *L) {
    OX_TRY
    push<XLCellReference>(L, MT_REFERENCE, check<XLWorksheet>(L, 1, MT_WORKSHEET)->lastCell());
    return 1;
    OX_CATCH_ALL
}

int ws_column_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLWorksheet>(L, 1, MT_WORKSHEET)->columnCount()));
    return 1;
    OX_CATCH_ALL
}

int ws_row_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLWorksheet>(L, 1, MT_WORKSHEET)->rowCount()));
    return 1;
    OX_CATCH_ALL
}

int ws_delete_row(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->deleteRow(uint32_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

int ws_update_sheet_name(lua_State *L) {
    OX_TRY
    check<XLWorksheet>(L, 1, MT_WORKSHEET)->updateSheetName(check_string(L, 2), check_string(L, 3));
    return 0;
    OX_CATCH_ALL
}

int ws_merges(lua_State *L) {
    OX_TRY
    push_live<XLMergeCells>(L, MT_MERGES, token_of(L, 1), check<XLWorksheet>(L, 1, MT_WORKSHEET)->merges());
    return 1;
    OX_CATCH_ALL
}

int ws_merge_cells(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    if (auto *range = maybe<XLCellRange>(L, 2, MT_RANGE)) ws->mergeCells(*range, opt_bool(L, 3, false));
    else ws->mergeCells(check_range_address(L, 2), opt_bool(L, 3, false));
    return 0;
    OX_CATCH_ALL
}

int ws_unmerge_cells(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    if (auto *range = maybe<XLCellRange>(L, 2, MT_RANGE)) ws->unmergeCells(*range);
    else ws->unmergeCells(check_range_address(L, 2));
    return 0;
    OX_CATCH_ALL
}

int ws_get_column_format(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    const auto format = lua_isinteger(L, 2) ? ws->getColumnFormat(uint16_t(lua_tointeger(L, 2)))
                                            : ws->getColumnFormat(check_string(L, 2));
    lua_pushinteger(L, lua_Integer(format));
    return 1;
    OX_CATCH_ALL
}

int ws_set_column_format(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    const auto idx = XLStyleIndex(luaL_checkinteger(L, 3));
    const bool ok = lua_isinteger(L, 2) ? ws->setColumnFormat(uint16_t(lua_tointeger(L, 2)), idx)
                                        : ws->setColumnFormat(check_string(L, 2), idx);
    lua_pushboolean(L, ok);
    return 1;
    OX_CATCH_ALL
}

int ws_get_row_format(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLWorksheet>(L, 1, MT_WORKSHEET)
                                       ->getRowFormat(uint16_t(luaL_checkinteger(L, 2)))));
    return 1;
    OX_CATCH_ALL
}

int ws_set_row_format(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)
                            ->setRowFormat(uint32_t(luaL_checkinteger(L, 2)),
                                           XLStyleIndex(luaL_checkinteger(L, 3))));
    return 1;
    OX_CATCH_ALL
}

// ---- protection ------------------------------------------------------------
int ws_protect_sheet(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->protectSheet(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}
int ws_protect_objects(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->protectObjects(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}
int ws_protect_scenarios(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->protectScenarios(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}

#define WS_ALLOW_METHODS(name_cpp, name_lua)                    \
    int ws_##name_lua(lua_State *L) {                           \
        OX_TRY                                                  \
        lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->name_cpp(opt_bool(L, 2, true))); \
        return 1;                                               \
        OX_CATCH_ALL                                            \
    }

WS_ALLOW_METHODS(allowInsertColumns, allow_insert_columns)
WS_ALLOW_METHODS(allowInsertRows, allow_insert_rows)
WS_ALLOW_METHODS(allowDeleteColumns, allow_delete_columns)
WS_ALLOW_METHODS(allowDeleteRows, allow_delete_rows)
WS_ALLOW_METHODS(allowSelectLockedCells, allow_select_locked_cells)
WS_ALLOW_METHODS(allowSelectUnlockedCells, allow_select_unlocked_cells)

#define WS_DENY_METHODS(name_cpp, name_lua)                     \
    int ws_##name_lua(lua_State *L) {                           \
        OX_TRY                                                  \
        lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->name_cpp()); \
        return 1;                                               \
        OX_CATCH_ALL                                            \
    }

WS_DENY_METHODS(denyInsertColumns, deny_insert_columns)
WS_DENY_METHODS(denyInsertRows, deny_insert_rows)
WS_DENY_METHODS(denyDeleteColumns, deny_delete_columns)
WS_DENY_METHODS(denyDeleteRows, deny_delete_rows)
WS_DENY_METHODS(denySelectLockedCells, deny_select_locked_cells)
WS_DENY_METHODS(denySelectUnlockedCells, deny_select_unlocked_cells)

int ws_set_password_hash(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->setPasswordHash(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int ws_set_password(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->setPassword(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int ws_clear_password(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->clearPassword());
    return 1;
    OX_CATCH_ALL
}
int ws_clear_sheet_protection(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->clearSheetProtection());
    return 1;
    OX_CATCH_ALL
}

#define WS_PROT_GETTER(name_cpp, name_lua)                      \
    int ws_##name_lua(lua_State *L) {                           \
        OX_TRY                                                  \
        lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->name_cpp()); \
        return 1;                                               \
        OX_CATCH_ALL                                            \
    }

WS_PROT_GETTER(sheetProtected, sheet_protected)
WS_PROT_GETTER(objectsProtected, objects_protected)
WS_PROT_GETTER(scenariosProtected, scenarios_protected)
WS_PROT_GETTER(insertColumnsAllowed, insert_columns_allowed)
WS_PROT_GETTER(insertRowsAllowed, insert_rows_allowed)
WS_PROT_GETTER(deleteColumnsAllowed, delete_columns_allowed)
WS_PROT_GETTER(deleteRowsAllowed, delete_rows_allowed)
WS_PROT_GETTER(selectLockedCellsAllowed, select_locked_cells_allowed)
WS_PROT_GETTER(selectUnlockedCellsAllowed, select_unlocked_cells_allowed)
WS_PROT_GETTER(passwordIsSet, password_is_set)

int ws_password_hash(lua_State *L) {
    OX_TRY
    push_string(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->passwordHash());
    return 1;
    OX_CATCH_ALL
}
int ws_sheet_protection_summary(lua_State *L) {
    OX_TRY
    push_string(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->sheetProtectionSummary());
    return 1;
    OX_CATCH_ALL
}
int ws_has_relationships(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->hasRelationships());
    return 1;
    OX_CATCH_ALL
}
int ws_has_vml_drawing(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->hasVmlDrawing());
    return 1;
    OX_CATCH_ALL
}
int ws_has_comments(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->hasComments());
    return 1;
    OX_CATCH_ALL
}
int ws_has_tables(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorksheet>(L, 1, MT_WORKSHEET)->hasTables());
    return 1;
    OX_CATCH_ALL
}

// ---- conditional formats ----------------------------------------------------
int cfs_empty(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLConditionalFormats>(L, 1, MT_CONDITIONAL_FORMATS)->empty());
    return 1;
    OX_CATCH_ALL
}
int cfs_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLConditionalFormats>(L, 1, MT_CONDITIONAL_FORMATS)->count()));
    return 1;
    OX_CATCH_ALL
}
int cfs_len(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLConditionalFormats>(L, 1, MT_CONDITIONAL_FORMATS)->count()));
    return 1;
    OX_CATCH_ALL
}
int cfs_get(lua_State *L) {
    OX_TRY
    push_live<XLConditionalFormat>(L, MT_CONDITIONAL_FORMAT, token_of(L, 1),
                                   check<XLConditionalFormats>(L, 1, MT_CONDITIONAL_FORMATS)
                                       ->conditionalFormatByIndex(size_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int cfs_create(lua_State *L) {
    OX_TRY
    auto *cfs = check<XLConditionalFormats>(L, 1, MT_CONDITIONAL_FORMATS);
    size_t index = 0;
    if (auto *copy_from = maybe<XLConditionalFormat>(L, 2, MT_CONDITIONAL_FORMAT)) {
        index = opt_string(L, 3).empty() ? cfs->create(*copy_from) : cfs->create(*copy_from, check_string(L, 3));
    } else if (lua_type(L, 2) == LUA_TSTRING) {
        index = cfs->create(XLConditionalFormat{}, check_string(L, 2));
    } else {
        index = cfs->create();
    }
    lua_pushinteger(L, lua_Integer(index));
    return 1;
    OX_CATCH_ALL
}
int cfs_summary(lua_State *L) {
    OX_TRY
    push_string(L, check<XLConditionalFormats>(L, 1, MT_CONDITIONAL_FORMATS)->summary());
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg cfs_methods[] = {
    { "empty", cfs_empty }, { "count", cfs_count }, { "get", cfs_get },
    { "create", cfs_create }, { "summary", cfs_summary }, { nullptr, nullptr },
};

int cf_empty(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLConditionalFormat>(L, 1, MT_CONDITIONAL_FORMAT)->empty());
    return 1;
    OX_CATCH_ALL
}
int cf_sqref(lua_State *L) {
    OX_TRY
    push_string(L, check<XLConditionalFormat>(L, 1, MT_CONDITIONAL_FORMAT)->sqref());
    return 1;
    OX_CATCH_ALL
}
int cf_set_sqref(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLConditionalFormat>(L, 1, MT_CONDITIONAL_FORMAT)->setSqref(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int cf_rules(lua_State *L) {
    OX_TRY
    push_live<XLCfRules>(L, MT_CF_RULES, token_of(L, 1),
                         check<XLConditionalFormat>(L, 1, MT_CONDITIONAL_FORMAT)->cfRules());
    return 1;
    OX_CATCH_ALL
}
int cf_summary(lua_State *L) {
    OX_TRY
    push_string(L, check<XLConditionalFormat>(L, 1, MT_CONDITIONAL_FORMAT)->summary());
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg cf_methods[] = {
    { "empty", cf_empty },     { "sqref", cf_sqref }, { "set_sqref", cf_set_sqref },
    { "cf_rules", cf_rules },  { "summary", cf_summary }, { nullptr, nullptr },
};

int cfr_empty(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRules>(L, 1, MT_CF_RULES)->empty());
    return 1;
    OX_CATCH_ALL
}
int cfr_max_priority_value(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCfRules>(L, 1, MT_CF_RULES)->maxPriorityValue()));
    return 1;
    OX_CATCH_ALL
}
int cfr_set_priority(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRules>(L, 1, MT_CF_RULES)
                            ->setPriority(size_t(luaL_checkinteger(L, 2)), uint16_t(luaL_checkinteger(L, 3))));
    return 1;
    OX_CATCH_ALL
}
int cfr_renumber_priorities(lua_State *L) {
    OX_TRY
    check<XLCfRules>(L, 1, MT_CF_RULES)->renumberPriorities(uint16_t(opt_integer(L, 2, 1)));
    return 0;
    OX_CATCH_ALL
}
int cfr_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCfRules>(L, 1, MT_CF_RULES)->count()));
    return 1;
    OX_CATCH_ALL
}
int cfr_len(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCfRules>(L, 1, MT_CF_RULES)->count()));
    return 1;
    OX_CATCH_ALL
}
int cfr_get(lua_State *L) {
    OX_TRY
    push_live<XLCfRule>(L, MT_CF_RULE, token_of(L, 1),
                        check<XLCfRules>(L, 1, MT_CF_RULES)->cfRuleByIndex(size_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int cfr_create(lua_State *L) {
    OX_TRY
    auto *rules = check<XLCfRules>(L, 1, MT_CF_RULES);
    size_t index = 0;
    if (auto *copy_from = maybe<XLCfRule>(L, 2, MT_CF_RULE)) {
        index = opt_string(L, 3).empty() ? rules->create(*copy_from) : rules->create(*copy_from, check_string(L, 3));
    } else if (lua_type(L, 2) == LUA_TSTRING) {
        index = rules->create(XLCfRule{}, check_string(L, 2));
    } else {
        index = rules->create();
    }
    lua_pushinteger(L, lua_Integer(index));
    return 1;
    OX_CATCH_ALL
}
int cfr_summary(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCfRules>(L, 1, MT_CF_RULES)->summary());
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg cfr_methods[] = {
    { "empty", cfr_empty },
    { "max_priority_value", cfr_max_priority_value },
    { "set_priority", cfr_set_priority },
    { "renumber_priorities", cfr_renumber_priorities },
    { "count", cfr_count },
    { "get", cfr_get },
    { "create", cfr_create },
    { "summary", cfr_summary },
    { nullptr, nullptr },
};

int cfrule_empty(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->empty());
    return 1;
    OX_CATCH_ALL
}
int cfrule_formula(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCfRule>(L, 1, MT_CF_RULE)->formula());
    return 1;
    OX_CATCH_ALL
}
int cfrule_type(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, int(check<XLCfRule>(L, 1, MT_CF_RULE)->type()));
    return 1;
    OX_CATCH_ALL
}
int cfrule_type_name(lua_State *L) {
    OX_TRY
    lua_pushstring(L, cf_type_name(int(check<XLCfRule>(L, 1, MT_CF_RULE)->type())));
    return 1;
    OX_CATCH_ALL
}
int cfrule_dxf_id(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCfRule>(L, 1, MT_CF_RULE)->dxfId()));
    return 1;
    OX_CATCH_ALL
}
int cfrule_priority(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCfRule>(L, 1, MT_CF_RULE)->priority()));
    return 1;
    OX_CATCH_ALL
}
int cfrule_stop_if_true(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->stopIfTrue());
    return 1;
    OX_CATCH_ALL
}
int cfrule_above_average(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->aboveAverage());
    return 1;
    OX_CATCH_ALL
}
int cfrule_percent(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->percent());
    return 1;
    OX_CATCH_ALL
}
int cfrule_bottom(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->bottom());
    return 1;
    OX_CATCH_ALL
}
int cfrule_operator(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, int(check<XLCfRule>(L, 1, MT_CF_RULE)->Operator()));
    return 1;
    OX_CATCH_ALL
}
int cfrule_operator_name(lua_State *L) {
    OX_TRY
    lua_pushstring(L, cf_operator_name(int(check<XLCfRule>(L, 1, MT_CF_RULE)->Operator())));
    return 1;
    OX_CATCH_ALL
}
int cfrule_text(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCfRule>(L, 1, MT_CF_RULE)->text());
    return 1;
    OX_CATCH_ALL
}
int cfrule_time_period(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, int(check<XLCfRule>(L, 1, MT_CF_RULE)->timePeriod()));
    return 1;
    OX_CATCH_ALL
}
int cfrule_time_period_name(lua_State *L) {
    OX_TRY
    lua_pushstring(L, cf_time_period_name(int(check<XLCfRule>(L, 1, MT_CF_RULE)->timePeriod())));
    return 1;
    OX_CATCH_ALL
}
int cfrule_rank(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCfRule>(L, 1, MT_CF_RULE)->rank()));
    return 1;
    OX_CATCH_ALL
}
int cfrule_std_dev(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLCfRule>(L, 1, MT_CF_RULE)->stdDev()));
    return 1;
    OX_CATCH_ALL
}
int cfrule_equal_average(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->equalAverage());
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_formula(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setFormula(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_type(lua_State *L) {
    OX_TRY
    const int v = cf_enum(L, 2, XL_CF_TYPE_NAMES, XL_CF_TYPE_COUNT, xl_cf_type_value, "XLCfType");
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setType(XLCfType(v)));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_dxf_id(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setDxfId(XLStyleIndex(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_stop_if_true(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setStopIfTrue(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_above_average(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setAboveAverage(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_percent(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setPercent(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_bottom(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setBottom(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_operator(lua_State *L) {
    OX_TRY
    const int v = cf_enum(L, 2, XL_CF_OPERATOR_NAMES, XL_CF_OPERATOR_COUNT, xl_cf_operator_value, "XLCfOperator");
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setOperator(XLCfOperator(v)));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_text(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setText(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_time_period(lua_State *L) {
    OX_TRY
    const int v = cf_enum(L, 2, XL_CF_TIME_PERIOD_NAMES, XL_CF_TIME_PERIOD_COUNT, xl_cf_time_period_value,
                          "XLCfTimePeriod");
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setTimePeriod(XLCfTimePeriod(v)));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_rank(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setRank(uint16_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_std_dev(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setStdDev(int16_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int cfrule_set_equal_average(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLCfRule>(L, 1, MT_CF_RULE)->setEqualAverage(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}
int cfrule_summary(lua_State *L) {
    OX_TRY
    push_string(L, check<XLCfRule>(L, 1, MT_CF_RULE)->summary());
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg cfrule_methods[] = {
    { "empty", cfrule_empty },
    { "formula", cfrule_formula },
    { "type", cfrule_type },
    { "type_name", cfrule_type_name },
    { "dxf_id", cfrule_dxf_id },
    { "priority", cfrule_priority },
    { "stop_if_true", cfrule_stop_if_true },
    { "above_average", cfrule_above_average },
    { "percent", cfrule_percent },
    { "bottom", cfrule_bottom },
    { "cf_operator", cfrule_operator },
    { "cf_operator_name", cfrule_operator_name },
    { "text", cfrule_text },
    { "time_period", cfrule_time_period },
    { "time_period_name", cfrule_time_period_name },
    { "rank", cfrule_rank },
    { "std_dev", cfrule_std_dev },
    { "equal_average", cfrule_equal_average },
    { "set_formula", cfrule_set_formula },
    { "set_type", cfrule_set_type },
    { "set_dxf_id", cfrule_set_dxf_id },
    { "set_stop_if_true", cfrule_set_stop_if_true },
    { "set_above_average", cfrule_set_above_average },
    { "set_percent", cfrule_set_percent },
    { "set_bottom", cfrule_set_bottom },
    { "set_operator", cfrule_set_operator },
    { "set_text", cfrule_set_text },
    { "set_time_period", cfrule_set_time_period },
    { "set_rank", cfrule_set_rank },
    { "set_std_dev", cfrule_set_std_dev },
    { "set_equal_average", cfrule_set_equal_average },
    { "summary", cfrule_summary },
    { nullptr, nullptr },
};

int ws_conditional_formats(lua_State *L) {
    OX_TRY
    push_live<XLConditionalFormats>(L, MT_CONDITIONAL_FORMATS, token_of(L, 1),
                                    check<XLWorksheet>(L, 1, MT_WORKSHEET)->conditionalFormats());
    return 1;
    OX_CATCH_ALL
}

// ---- the worksheet method table -------------------------------------------
const luaL_Reg worksheet_methods[] = {
    { "cell", ws_cell },
    { "find_cell", ws_find_cell },
    { "set_dimension", ws_set_dimension },
    { "range", ws_range },
    { "rows", ws_rows },
    { "row", ws_row },
    { "column", ws_column },
    { "last_cell", ws_last_cell },
    { "column_count", ws_column_count },
    { "row_count", ws_row_count },
    { "delete_row", ws_delete_row },
    { "update_sheet_name", ws_update_sheet_name },
    { "merges", ws_merges },
    { "merge_cells", ws_merge_cells },
    { "unmerge_cells", ws_unmerge_cells },
    { "get_column_format", ws_get_column_format },
    { "set_column_format", ws_set_column_format },
    { "get_row_format", ws_get_row_format },
    { "set_row_format", ws_set_row_format },
    { "conditional_formats", ws_conditional_formats },
    { "protect_sheet", ws_protect_sheet },
    { "protect_objects", ws_protect_objects },
    { "protect_scenarios", ws_protect_scenarios },
    { "allow_insert_columns", ws_allow_insert_columns },
    { "allow_insert_rows", ws_allow_insert_rows },
    { "allow_delete_columns", ws_allow_delete_columns },
    { "allow_delete_rows", ws_allow_delete_rows },
    { "allow_select_locked_cells", ws_allow_select_locked_cells },
    { "allow_select_unlocked_cells", ws_allow_select_unlocked_cells },
    { "deny_insert_columns", ws_deny_insert_columns },
    { "deny_insert_rows", ws_deny_insert_rows },
    { "deny_delete_columns", ws_deny_delete_columns },
    { "deny_delete_rows", ws_deny_delete_rows },
    { "deny_select_locked_cells", ws_deny_select_locked_cells },
    { "deny_select_unlocked_cells", ws_deny_select_unlocked_cells },
    { "set_password_hash", ws_set_password_hash },
    { "set_password", ws_set_password },
    { "clear_password", ws_clear_password },
    { "clear_sheet_protection", ws_clear_sheet_protection },
    { "sheet_protected", ws_sheet_protected },
    { "objects_protected", ws_objects_protected },
    { "scenarios_protected", ws_scenarios_protected },
    { "insert_columns_allowed", ws_insert_columns_allowed },
    { "insert_rows_allowed", ws_insert_rows_allowed },
    { "delete_columns_allowed", ws_delete_columns_allowed },
    { "delete_rows_allowed", ws_delete_rows_allowed },
    { "select_locked_cells_allowed", ws_select_locked_cells_allowed },
    { "select_unlocked_cells_allowed", ws_select_unlocked_cells_allowed },
    { "password_hash", ws_password_hash },
    { "password_is_set", ws_password_is_set },
    { "sheet_protection_summary", ws_sheet_protection_summary },
    { "has_relationships", ws_has_relationships },
    { "has_vml_drawing", ws_has_vml_drawing },
    { "has_comments", ws_has_comments },
    { "has_tables", ws_has_tables },
    { nullptr, nullptr },
};

}  // namespace

namespace ox {

void bind_sheet(lua_State *L) {
    register_type<XLWorksheet>(L, MT_WORKSHEET, worksheet_methods);
    add_sheet_base_methods(L, MT_WORKSHEET);  // adds XLSheetBase<> methods (name, index, ...)

    register_type<XLConditionalFormats>(L, MT_CONDITIONAL_FORMATS, cfs_methods);
    register_type<XLConditionalFormat>(L, MT_CONDITIONAL_FORMAT, cf_methods);
    register_type<XLCfRules>(L, MT_CF_RULES, cfr_methods);
    register_type<XLCfRule>(L, MT_CF_RULE, cfrule_methods);

    luaL_getmetatable(L, MT_CONDITIONAL_FORMATS);
    lua_pushcfunction(L, cfs_len);
    lua_setfield(L, -2, "__len");
    lua_pop(L, 1);
    luaL_getmetatable(L, MT_CF_RULES);
    lua_pushcfunction(L, cfr_len);
    lua_setfield(L, -2, "__len");
    lua_pop(L, 1);
}

}  // namespace ox
