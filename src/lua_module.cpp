// =============================================================================
// lua_module.cpp — module entry point (luaopen_openxlsx), utilities, constants
// =============================================================================
#include "lua_bind.hpp"

#include <cstdio>

using namespace OpenXLSX;
using namespace ox;

namespace {

// ============================================================================
// module-level utility functions
// ============================================================================
int l_version(lua_State *L) {
    push_string(L, "lua-openxlsx 0.1.0 (OpenXLSX 0.5.1)");
    return 1;
}

int l_zip_library_name(lua_State *L) {
    lua_pushstring(L, OpenXLSX::ZipLibraryName());
    return 1;
}

int l_zip_library_version(lua_State *L) {
    lua_pushstring(L, OpenXLSX::ZipLibraryVersion());
    return 1;
}

int l_column_as_string(lua_State *L) {
    OX_TRY
    const lua_Integer col = luaL_checkinteger(L, 1);
    if (col < 1 || col > 16384) luaL_error(L, "column out of range: %d (valid: 1..16384)", int(col));
    push_string(L, XLCellReference::columnAsString(uint16_t(col)));
    return 1;
    OX_CATCH_ALL
}

int l_column_as_number(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(XLCellReference::columnAsNumber(check_column_letters(L, 1))));
    return 1;
    OX_CATCH_ALL
}

int l_row_as_string(lua_State *L) {
    OX_TRY
    const lua_Integer row = luaL_checkinteger(L, 1);
    if (row < 1 || row > 1048576) luaL_error(L, "row out of range: %d (valid: 1..1048576)", int(row));
    push_string(L, XLCellReference::rowAsString(uint32_t(row)));
    return 1;
    OX_CATCH_ALL
}

int l_row_as_number(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(XLCellReference::rowAsNumber(check_row_digits(L, 1))));
    return 1;
    OX_CATCH_ALL
}

int l_coordinates_from_address(lua_State *L) {
    OX_TRY
    const XLCoordinates coords = XLCellReference::coordinatesFromAddress(check_cell_address(L, 1));
    lua_pushinteger(L, lua_Integer(coords.first));   // row
    lua_pushinteger(L, lua_Integer(coords.second));  // column
    return 2;
    OX_CATCH_ALL
}

int l_excel_password_hash(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(OpenXLSX::ExcelPasswordHash(check_string(L, 1))));
    return 1;
    OX_CATCH_ALL
}

int l_excel_password_hash_string(lua_State *L) {
    OX_TRY
    push_string(L, OpenXLSX::ExcelPasswordHashAsString(check_string(L, 1)));
    return 1;
    OX_CATCH_ALL
}

int l_binary_as_hex_string(lua_State *L) {
    OX_TRY
    size_t n = 0;
    const char *data = luaL_checklstring(L, 1, &n);
    push_string(L, OpenXLSX::BinaryAsHexString(data, n));
    return 1;
    OX_CATCH_ALL
}

// ============================================================================
// constants
// ============================================================================
void set_int(lua_State *L, int table, const char *key, lua_Integer value) {
    const int t = lua_absindex(L, table);
    lua_pushinteger(L, value);
    lua_setfield(L, t, key);
}

void set_bool(lua_State *L, int table, const char *key, bool value) {
    const int t = lua_absindex(L, table);
    lua_pushboolean(L, value);
    lua_setfield(L, t, key);
}

const char *const PROPERTY_KEYS[] = {
    "TITLE",        "SUBJECT",       "CREATOR",         "KEYWORDS",     "DESCRIPTION",
    "LAST_MODIFIED_BY", "LAST_PRINTED", "CREATION_DATE", "MODIFICATION_DATE", "CATEGORY",
    "APPLICATION",  "DOC_SECURITY",  "SCALE_CROP",      "MANAGER",      "COMPANY",
    "LINKS_UP_TO_DATE", "SHARED_DOC", "HYPERLINK_BASE", "HYPERLINKS_CHANGED", "APP_VERSION",
};

const char *const VALUE_TYPE_KEYS[] = { "EMPTY", "BOOLEAN", "INTEGER", "FLOAT", "ERROR", "STRING" };

const char *const SHEET_TYPE_KEYS[] = { "WORKSHEET", "CHARTSHEET", "DIALOGSHEET", "MACROSHEET" };

const char *const SHEET_STATE_KEYS[] = { "VISIBLE", "HIDDEN", "VERY_HIDDEN" };

const char *const CF_TYPE_KEYS[] = {
    "EXPRESSION", "CELL_IS", "COLOR_SCALE", "DATA_BAR", "ICON_SET", "TOP10", "UNIQUE_VALUES", "DUPLICATE_VALUES",
    "CONTAINS_TEXT", "NOT_CONTAINS_TEXT", "BEGINS_WITH", "ENDS_WITH", "CONTAINS_BLANKS", "NOT_CONTAINS_BLANKS",
    "CONTAINS_ERRORS", "NOT_CONTAINS_ERRORS", "TIME_PERIOD", "ABOVE_AVERAGE", "INVALID",
};

const char *const CF_OPERATOR_KEYS[] = {
    "LESS_THAN", "LESS_THAN_OR_EQUAL", "EQUAL", "NOT_EQUAL", "GREATER_THAN_OR_EQUAL", "GREATER_THAN",
    "BETWEEN", "NOT_BETWEEN", "CONTAINS_TEXT", "NOT_CONTAINS", "BEGINS_WITH", "ENDS_WITH", "INVALID",
};

const char *const CF_TIME_PERIOD_KEYS[] = {
    "TODAY", "YESTERDAY", "TOMORROW", "LAST_7_DAYS", "THIS_MONTH", "LAST_MONTH", "NEXT_MONTH",
    "THIS_WEEK", "LAST_WEEK", "NEXT_WEEK", "INVALID",
};

template <size_t N>
void set_keys(lua_State *L, const char *const (&keys)[N], const int *values) {
    for (size_t i = 0; i < N; ++i) set_int(L, -1, keys[i], values != nullptr ? values[i] : lua_Integer(i));
}

void register_constants(lua_State *L) {
    // scalar constants
    set_bool(L, 1, "FORCE_OVERWRITE", true);
    set_bool(L, 1, "DO_NOT_OVERWRITE", false);
    set_bool(L, 1, "RESET_VALUE", true);
    set_bool(L, 1, "PRESERVE_VALUE", false);
    set_bool(L, 1, "EMPTY_HIDDEN_CELLS", true);
    set_bool(L, 1, "CALCULATE_LAST_CELL", true);
    set_bool(L, 1, "DO_NOT_CALCULATE_LAST_CELL", false);
    set_int(L, 1, "MERGE_NOT_FOUND", XLMergeNotFound);
    set_int(L, 1, "MAX_COLS", OpenXLSX::MAX_COLS);
    set_int(L, 1, "MAX_ROWS", OpenXLSX::MAX_ROWS);
    set_int(L, 1, "MAX_SHAPE_ANCHOR_COLUMN", OpenXLSX::MAX_SHAPE_ANCHOR_COLUMN);
    set_int(L, 1, "MAX_SHAPE_ANCHOR_ROW", OpenXLSX::MAX_SHAPE_ANCHOR_ROW);

    // keep flags for cell:clear()
    lua_newtable(L);
    set_int(L, -1, "STYLE", XLKeepCellStyle);
    set_int(L, -1, "TYPE", XLKeepCellType);
    set_int(L, -1, "VALUE", XLKeepCellValue);
    set_int(L, -1, "FORMULA", XLKeepCellFormula);
    lua_setfield(L, 1, "keep");

    // XLDocument::property enumeration
    lua_newtable(L);
    for (size_t i = 0; i < XL_PROPERTY_COUNT; ++i) set_int(L, -1, PROPERTY_KEYS[i], lua_Integer(i));
    lua_setfield(L, 1, "property");

    // XLValueType
    lua_newtable(L);
    set_keys(L, VALUE_TYPE_KEYS, nullptr);
    lua_setfield(L, 1, "value_type");

    // XLSheetType
    lua_newtable(L);
    set_keys(L, SHEET_TYPE_KEYS, nullptr);
    lua_setfield(L, 1, "sheet_type");

    // XLSheetState
    lua_newtable(L);
    set_keys(L, SHEET_STATE_KEYS, nullptr);
    lua_setfield(L, 1, "sheet_state");

    // XLIteratorDirection
    lua_newtable(L);
    set_int(L, -1, "FORWARD", 0);
    set_int(L, -1, "REVERSE", 1);
    lua_setfield(L, 1, "iter_direction");

    // conditional formatting enums (values differ from declaration order)
    {
        int values[XL_CF_TYPE_COUNT];
        for (int i = 0; i < XL_CF_TYPE_COUNT; ++i) values[i] = xl_cf_type_value(i);
        lua_newtable(L);
        set_keys(L, CF_TYPE_KEYS, values);
        lua_setfield(L, 1, "cf_type");

        int op_values[XL_CF_OPERATOR_COUNT];
        for (int i = 0; i < XL_CF_OPERATOR_COUNT; ++i) op_values[i] = xl_cf_operator_value(i);
        lua_newtable(L);
        set_keys(L, CF_OPERATOR_KEYS, op_values);
        lua_setfield(L, 1, "cf_operator");

        int tp_values[XL_CF_TIME_PERIOD_COUNT];
        for (int i = 0; i < XL_CF_TIME_PERIOD_COUNT; ++i) tp_values[i] = xl_cf_time_period_value(i);
        lua_newtable(L);
        set_keys(L, CF_TIME_PERIOD_KEYS, tp_values);
        lua_setfield(L, 1, "cf_time_period");
    }
}

}  // namespace

namespace ox {

void bind_document(lua_State *L);
void bind_sheet(lua_State *L);
void bind_data(lua_State *L);
void bind_styles(lua_State *L);
void bind_p4(lua_State *L);

}  // namespace ox

extern "C" int luaopen_openxlsx(lua_State *L) {
    lua_settop(L, 0);  // require passes the module name as argument - drop it
    lua_newtable(L);   // module table at index 1

    ox::bind_document(L);  // XLDocument, XLWorkbook, XLSheet, XLChartsheet
    ox::bind_sheet(L);     // XLWorksheet, conditional formatting
    ox::bind_data(L);      // cells, values, references, rows, columns, ranges, ...
    ox::bind_styles(L);    // XLStyles family
    ox::bind_p4(L);        // comments, tables, drawings, relationships

    // module-level functions
    lua_pushcfunction(L, l_version);
    lua_setfield(L, 1, "version");
    lua_pushcfunction(L, l_zip_library_name);
    lua_setfield(L, 1, "zip_library_name");
    lua_pushcfunction(L, l_zip_library_version);
    lua_setfield(L, 1, "zip_library_version");
    lua_pushcfunction(L, l_column_as_string);
    lua_setfield(L, 1, "column_as_string");
    lua_pushcfunction(L, l_column_as_number);
    lua_setfield(L, 1, "column_as_number");
    lua_pushcfunction(L, l_row_as_string);
    lua_setfield(L, 1, "row_as_string");
    lua_pushcfunction(L, l_row_as_number);
    lua_setfield(L, 1, "row_as_number");
    lua_pushcfunction(L, l_coordinates_from_address);
    lua_setfield(L, 1, "coordinates_from_address");
    lua_pushcfunction(L, l_excel_password_hash);
    lua_setfield(L, 1, "excel_password_hash");
    lua_pushcfunction(L, l_excel_password_hash_string);
    lua_setfield(L, 1, "excel_password_hash_string");
    lua_pushcfunction(L, l_binary_as_hex_string);
    lua_setfield(L, 1, "binary_as_hex_string");

    register_constants(L);

    return 1;
}
