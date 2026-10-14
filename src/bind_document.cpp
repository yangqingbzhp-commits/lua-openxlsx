// =============================================================================
// bind_document.cpp — XLDocument, XLWorkbook, XLSheet, XLChartsheet
// =============================================================================
#include "lua_bind.hpp"

#include <cstring>

using namespace OpenXLSX;
using namespace ox;

namespace {

// ============================================================================
// XLDocument
// ============================================================================
int doc_new(lua_State *L) {
    OX_TRY
    if (lua_isnoneornil(L, 1)) {
        // NOTE: XLDocument must not be moved (moved-from objects crash in their
        // destructor - upstream bug), so it is constructed in place.
        push_new<XLDocument>(L, MT_DOCUMENT);
    } else {
        push_new<XLDocument>(L, MT_DOCUMENT, check_string(L, 1));
    }
    box_set_owner_token<XLDocument>(L, -1, std::make_shared<DocToken>());
    return 1;
    OX_CATCH_ALL
}

int doc_open(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    box_new_token<XLDocument>(L, 1);  // invalidate handles from previous content
    doc->open(check_string(L, 2));
    return 0;
    OX_CATCH_ALL
}

int doc_create(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    box_new_token<XLDocument>(L, 1);  // invalidate handles from previous content
    doc->create(check_string(L, 2), opt_bool(L, 3, true));
    return 0;
    OX_CATCH_ALL
}

int doc_close(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    auto *b   = static_cast<Box *>(lua_touserdata(L, 1));
    if (b->token) b->token->alive = false;       // all derived handles become invalid
    doc->close();
    b->token = std::make_shared<DocToken>();     // fresh generation for a potential reopen
    return 0;
    OX_CATCH_ALL
}

int doc_save(lua_State *L) {
    OX_TRY
    check<XLDocument>(L, 1, MT_DOCUMENT)->save();
    return 0;
    OX_CATCH_ALL
}

int doc_save_as(lua_State *L) {
    OX_TRY
    check<XLDocument>(L, 1, MT_DOCUMENT)->saveAs(check_string(L, 2), opt_bool(L, 3, true));
    return 0;
    OX_CATCH_ALL
}

int doc_name(lua_State *L) {
    OX_TRY
    push_string(L, check<XLDocument>(L, 1, MT_DOCUMENT)->name());
    return 1;
    OX_CATCH_ALL
}

int doc_path(lua_State *L) {
    OX_TRY
    push_string(L, check<XLDocument>(L, 1, MT_DOCUMENT)->path());
    return 1;
    OX_CATCH_ALL
}

int doc_is_open(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLDocument>(L, 1, MT_DOCUMENT)->isOpen());
    return 1;
    OX_CATCH_ALL
}

int doc_workbook(lua_State *L) {
    OX_TRY
    push_live<XLWorkbook>(L, MT_WORKBOOK, token_of(L, 1), check<XLDocument>(L, 1, MT_DOCUMENT)->workbook());
    return 1;
    OX_CATCH_ALL
}

int doc_property(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    const int prop = check_enum(L, 2, XL_PROPERTY_NAMES, XL_PROPERTY_COUNT, "XLProperty");
    push_string(L, doc->property(XLProperty(prop)));
    return 1;
    OX_CATCH_ALL
}

int doc_set_property(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    const int prop = check_enum(L, 2, XL_PROPERTY_NAMES, XL_PROPERTY_COUNT, "XLProperty");
    doc->setProperty(XLProperty(prop), check_string(L, 3));
    return 0;
    OX_CATCH_ALL
}

int doc_delete_property(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    const int prop = check_enum(L, 2, XL_PROPERTY_NAMES, XL_PROPERTY_COUNT, "XLProperty");
    doc->deleteProperty(XLProperty(prop));
    return 0;
    OX_CATCH_ALL
}

int doc_validate_sheet_name(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    lua_pushboolean(L, doc->validateSheetName(check_string(L, 2), opt_bool(L, 3, false)));
    return 1;
    OX_CATCH_ALL
}

int doc_show_warnings(lua_State *L) {
    OX_TRY
    check<XLDocument>(L, 1, MT_DOCUMENT)->showWarnings();
    return 0;
    OX_CATCH_ALL
}

int doc_suppress_warnings(lua_State *L) {
    OX_TRY
    check<XLDocument>(L, 1, MT_DOCUMENT)->suppressWarnings();
    return 0;
    OX_CATCH_ALL
}

int doc_cleanup_shared_strings(lua_State *L) {
    OX_TRY
    check<XLDocument>(L, 1, MT_DOCUMENT)->cleanupSharedStrings();
    return 0;
    OX_CATCH_ALL
}

int doc_has_sheet_relationships(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLDocument>(L, 1, MT_DOCUMENT)->hasSheetRelationships(
                           uint16_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

int doc_has_sheet_vml_drawing(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLDocument>(L, 1, MT_DOCUMENT)->hasSheetVmlDrawing(
                           uint16_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

int doc_has_sheet_comments(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLDocument>(L, 1, MT_DOCUMENT)->hasSheetComments(
                           uint16_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

int doc_has_sheet_tables(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLDocument>(L, 1, MT_DOCUMENT)->hasSheetTables(
                           uint16_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg doc_methods[] = {
    { "open", doc_open },
    { "create", doc_create },
    { "close", doc_close },
    { "save", doc_save },
    { "save_as", doc_save_as },
    { "name", doc_name },
    { "path", doc_path },
    { "is_open", doc_is_open },
    { "workbook", doc_workbook },
    { "property", doc_property },
    { "set_property", doc_set_property },
    { "delete_property", doc_delete_property },
    { "validate_sheet_name", doc_validate_sheet_name },
    { "show_warnings", doc_show_warnings },
    { "suppress_warnings", doc_suppress_warnings },
    { "cleanup_shared_strings", doc_cleanup_shared_strings },
    { "has_sheet_relationships", doc_has_sheet_relationships },
    { "has_sheet_vml_drawing", doc_has_sheet_vml_drawing },
    { "has_sheet_comments", doc_has_sheet_comments },
    { "has_sheet_tables", doc_has_sheet_tables },
    { nullptr, nullptr },
};

// ============================================================================
// XLWorkbook
// ============================================================================
int wb_sheet(lua_State *L) {
    OX_TRY
    auto *wb = check<XLWorkbook>(L, 1, MT_WORKBOOK);
    const auto token = token_of(L, 1);
    if (lua_isinteger(L, 2)) push_live<XLSheet>(L, MT_SHEET, token, wb->sheet(uint16_t(lua_tointeger(L, 2))));
    else push_live<XLSheet>(L, MT_SHEET, token, wb->sheet(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}

int wb_worksheet(lua_State *L) {
    OX_TRY
    auto *wb = check<XLWorkbook>(L, 1, MT_WORKBOOK);
    const auto token = token_of(L, 1);
    if (lua_isinteger(L, 2)) push_live<XLWorksheet>(L, MT_WORKSHEET, token, wb->worksheet(uint16_t(lua_tointeger(L, 2))));
    else push_live<XLWorksheet>(L, MT_WORKSHEET, token, wb->worksheet(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}

int wb_chartsheet(lua_State *L) {
    OX_TRY
    auto *wb = check<XLWorkbook>(L, 1, MT_WORKBOOK);
    const auto token = token_of(L, 1);
    if (lua_isinteger(L, 2)) push_live<XLChartsheet>(L, MT_CHARTSHEET, token, wb->chartsheet(uint16_t(lua_tointeger(L, 2))));
    else push_live<XLChartsheet>(L, MT_CHARTSHEET, token, wb->chartsheet(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}

int wb_delete_sheet(lua_State *L) {
    OX_TRY
    check<XLWorkbook>(L, 1, MT_WORKBOOK)->deleteSheet(check_string(L, 2));
    return 0;
    OX_CATCH_ALL
}

int wb_add_worksheet(lua_State *L) {
    OX_TRY
    check<XLWorkbook>(L, 1, MT_WORKBOOK)->addWorksheet(check_string(L, 2));
    return 0;
    OX_CATCH_ALL
}

int wb_clone_sheet(lua_State *L) {
    OX_TRY
    check<XLWorkbook>(L, 1, MT_WORKBOOK)->cloneSheet(check_string(L, 2), check_string(L, 3));
    return 0;
    OX_CATCH_ALL
}

int wb_set_sheet_index(lua_State *L) {
    OX_TRY
    check<XLWorkbook>(L, 1, MT_WORKBOOK)->setSheetIndex(check_string(L, 2), unsigned(luaL_checkinteger(L, 3)));
    return 0;
    OX_CATCH_ALL
}

int wb_index_of_sheet(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLWorkbook>(L, 1, MT_WORKBOOK)->indexOfSheet(check_string(L, 2))));
    return 1;
    OX_CATCH_ALL
}

int wb_type_of_sheet(lua_State *L) {
    OX_TRY
    auto *wb = check<XLWorkbook>(L, 1, MT_WORKBOOK);
    const XLSheetType type = lua_isinteger(L, 2) ? wb->typeOfSheet(unsigned(lua_tointeger(L, 2)))
                                                 : wb->typeOfSheet(check_string(L, 2));
    lua_pushinteger(L, int(type));
    return 1;
    OX_CATCH_ALL
}

int wb_type_of_sheet_name(lua_State *L) {
    OX_TRY
    auto *wb = check<XLWorkbook>(L, 1, MT_WORKBOOK);
    const XLSheetType type = lua_isinteger(L, 2) ? wb->typeOfSheet(unsigned(lua_tointeger(L, 2)))
                                                 : wb->typeOfSheet(check_string(L, 2));
    const int idx = int(type);
    lua_pushstring(L, (idx >= 0 && idx < XL_SHEET_TYPE_COUNT) ? XL_SHEET_TYPE_NAMES[idx] : "unknown");
    return 1;
    OX_CATCH_ALL
}

int wb_sheet_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLWorkbook>(L, 1, MT_WORKBOOK)->sheetCount()));
    return 1;
    OX_CATCH_ALL
}

int wb_worksheet_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLWorkbook>(L, 1, MT_WORKBOOK)->worksheetCount()));
    return 1;
    OX_CATCH_ALL
}

int wb_chartsheet_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLWorkbook>(L, 1, MT_WORKBOOK)->chartsheetCount()));
    return 1;
    OX_CATCH_ALL
}

int wb_sheet_names(lua_State *L) {
    OX_TRY
    const std::vector<std::string> names = check<XLWorkbook>(L, 1, MT_WORKBOOK)->sheetNames();
    lua_createtable(L, int(names.size()), 0);
    int i = 1;
    for (const auto &n : names) {
        push_string(L, n);
        lua_seti(L, -2, i++);
    }
    return 1;
    OX_CATCH_ALL
}

int wb_worksheet_names(lua_State *L) {
    OX_TRY
    const std::vector<std::string> names = check<XLWorkbook>(L, 1, MT_WORKBOOK)->worksheetNames();
    lua_createtable(L, int(names.size()), 0);
    int i = 1;
    for (const auto &n : names) {
        push_string(L, n);
        lua_seti(L, -2, i++);
    }
    return 1;
    OX_CATCH_ALL
}

int wb_chartsheet_names(lua_State *L) {
    OX_TRY
    const std::vector<std::string> names = check<XLWorkbook>(L, 1, MT_WORKBOOK)->chartsheetNames();
    lua_createtable(L, int(names.size()), 0);
    int i = 1;
    for (const auto &n : names) {
        push_string(L, n);
        lua_seti(L, -2, i++);
    }
    return 1;
    OX_CATCH_ALL
}

int wb_sheet_exists(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorkbook>(L, 1, MT_WORKBOOK)->sheetExists(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}

int wb_worksheet_exists(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorkbook>(L, 1, MT_WORKBOOK)->worksheetExists(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}

int wb_chartsheet_exists(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLWorkbook>(L, 1, MT_WORKBOOK)->chartsheetExists(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}

int wb_update_sheet_references(lua_State *L) {
    OX_TRY
    check<XLWorkbook>(L, 1, MT_WORKBOOK)->updateSheetReferences(check_string(L, 2), check_string(L, 3));
    return 0;
    OX_CATCH_ALL
}

int wb_delete_named_ranges(lua_State *L) {
    OX_TRY
    check<XLWorkbook>(L, 1, MT_WORKBOOK)->deleteNamedRanges();
    return 0;
    OX_CATCH_ALL
}

int wb_set_full_calculation_on_load(lua_State *L) {
    OX_TRY
    check<XLWorkbook>(L, 1, MT_WORKBOOK)->setFullCalculationOnLoad();
    return 0;
    OX_CATCH_ALL
}

const luaL_Reg wb_methods[] = {
    { "sheet", wb_sheet },
    { "worksheet", wb_worksheet },
    { "chartsheet", wb_chartsheet },
    { "delete_sheet", wb_delete_sheet },
    { "add_worksheet", wb_add_worksheet },
    { "clone_sheet", wb_clone_sheet },
    { "set_sheet_index", wb_set_sheet_index },
    { "index_of_sheet", wb_index_of_sheet },
    { "type_of_sheet", wb_type_of_sheet },
    { "type_of_sheet_name", wb_type_of_sheet_name },
    { "sheet_count", wb_sheet_count },
    { "worksheet_count", wb_worksheet_count },
    { "chartsheet_count", wb_chartsheet_count },
    { "sheet_names", wb_sheet_names },
    { "worksheet_names", wb_worksheet_names },
    { "chartsheet_names", wb_chartsheet_names },
    { "sheet_exists", wb_sheet_exists },
    { "worksheet_exists", wb_worksheet_exists },
    { "chartsheet_exists", wb_chartsheet_exists },
    { "update_sheet_references", wb_update_sheet_references },
    { "delete_named_ranges", wb_delete_named_ranges },
    { "set_full_calculation_on_load", wb_set_full_calculation_on_load },
    { nullptr, nullptr },
};

// ============================================================================
// XLSheet (variant holder)
// ============================================================================
int sheet_visibility(lua_State *L) {
    OX_TRY
    const int state = int(check<XLSheet>(L, 1, MT_SHEET)->visibility());
    lua_pushstring(L, (state >= 0 && state < XL_SHEET_STATE_COUNT) ? XL_SHEET_STATE_NAMES[state] : "unknown");
    return 1;
    OX_CATCH_ALL
}

int sheet_visibility_num(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, int(check<XLSheet>(L, 1, MT_SHEET)->visibility()));
    return 1;
    OX_CATCH_ALL
}

int sheet_set_visibility(lua_State *L) {
    OX_TRY
    const int state = check_enum(L, 2, XL_SHEET_STATE_NAMES, XL_SHEET_STATE_COUNT, "XLSheetState");
    check<XLSheet>(L, 1, MT_SHEET)->setVisibility(XLSheetState(state));
    return 0;
    OX_CATCH_ALL
}

int sheet_color(lua_State *L) {
    OX_TRY
    push<XLColor>(L, MT_COLOR, check<XLSheet>(L, 1, MT_SHEET)->color());
    return 1;
    OX_CATCH_ALL
}

int sheet_set_color(lua_State *L) {
    OX_TRY
    check<XLSheet>(L, 1, MT_SHEET)->setColor(*check<XLColor>(L, 2, MT_COLOR));
    return 0;
    OX_CATCH_ALL
}

int sheet_index(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLSheet>(L, 1, MT_SHEET)->index()));
    return 1;
    OX_CATCH_ALL
}

int sheet_set_index(lua_State *L) {
    OX_TRY
    check<XLSheet>(L, 1, MT_SHEET)->setIndex(uint16_t(luaL_checkinteger(L, 2)));
    return 0;
    OX_CATCH_ALL
}

int sheet_name(lua_State *L) {
    OX_TRY
    push_string(L, check<XLSheet>(L, 1, MT_SHEET)->name());
    return 1;
    OX_CATCH_ALL
}

int sheet_set_name(lua_State *L) {
    OX_TRY
    check<XLSheet>(L, 1, MT_SHEET)->setName(check_string(L, 2));
    return 0;
    OX_CATCH_ALL
}

int sheet_is_selected(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLSheet>(L, 1, MT_SHEET)->isSelected());
    return 1;
    OX_CATCH_ALL
}

int sheet_set_selected(lua_State *L) {
    OX_TRY
    check<XLSheet>(L, 1, MT_SHEET)->setSelected(lua_toboolean(L, 2) != 0);
    return 0;
    OX_CATCH_ALL
}

int sheet_is_active(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLSheet>(L, 1, MT_SHEET)->isActive());
    return 1;
    OX_CATCH_ALL
}

int sheet_set_active(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLSheet>(L, 1, MT_SHEET)->setActive());
    return 1;
    OX_CATCH_ALL
}

int sheet_clone(lua_State *L) {
    OX_TRY
    check<XLSheet>(L, 1, MT_SHEET)->clone(check_string(L, 2));
    return 0;
    OX_CATCH_ALL
}

int sheet_is_worksheet(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLSheet>(L, 1, MT_SHEET)->isType<XLWorksheet>());
    return 1;
    OX_CATCH_ALL
}

int sheet_is_chartsheet(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLSheet>(L, 1, MT_SHEET)->isType<XLChartsheet>());
    return 1;
    OX_CATCH_ALL
}

int sheet_as_worksheet(lua_State *L) {
    OX_TRY
    push_live<XLWorksheet>(L, MT_WORKSHEET, token_of(L, 1),
                           check<XLSheet>(L, 1, MT_SHEET)->get<XLWorksheet>());
    return 1;
    OX_CATCH_ALL
}

int sheet_as_chartsheet(lua_State *L) {
    OX_TRY
    push_live<XLChartsheet>(L, MT_CHARTSHEET, token_of(L, 1),
                            check<XLSheet>(L, 1, MT_SHEET)->get<XLChartsheet>());
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg sheet_methods[] = {
    { "visibility", sheet_visibility },
    { "visibility_num", sheet_visibility_num },
    { "set_visibility", sheet_set_visibility },
    { "color", sheet_color },
    { "set_color", sheet_set_color },
    { "index", sheet_index },
    { "set_index", sheet_set_index },
    { "name", sheet_name },
    { "set_name", sheet_set_name },
    { "is_selected", sheet_is_selected },
    { "set_selected", sheet_set_selected },
    { "is_active", sheet_is_active },
    { "set_active", sheet_set_active },
    { "clone", sheet_clone },
    { "is_worksheet", sheet_is_worksheet },
    { "is_chartsheet", sheet_is_chartsheet },
    { "as_worksheet", sheet_as_worksheet },
    { "as_chartsheet", sheet_as_chartsheet },
    { nullptr, nullptr },
};

// ============================================================================
// Sheet base methods shared by XLWorksheet and XLChartsheet (CRTP XLSheetBase)
// ============================================================================
template <typename T>
constexpr const char *sheet_base_mt() {
    return MT_WORKSHEET;
}
template <>
constexpr const char *sheet_base_mt<XLChartsheet>() {
    return MT_CHARTSHEET;
}

template <typename T>
int sheet_base_visibility(lua_State *L) {
    OX_TRY
    const int state = int(check<T>(L, 1, sheet_base_mt<T>())->visibility());
    lua_pushstring(L, (state >= 0 && state < XL_SHEET_STATE_COUNT) ? XL_SHEET_STATE_NAMES[state] : "unknown");
    return 1;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_visibility_num(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, int(check<T>(L, 1, sheet_base_mt<T>())->visibility()));
    return 1;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_set_visibility(lua_State *L) {
    OX_TRY
    const int state = check_enum(L, 2, XL_SHEET_STATE_NAMES, XL_SHEET_STATE_COUNT, "XLSheetState");
    check<T>(L, 1, sheet_base_mt<T>())->setVisibility(XLSheetState(state));
    return 0;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_color(lua_State *L) {
    OX_TRY
    push<XLColor>(L, MT_COLOR, check<T>(L, 1, sheet_base_mt<T>())->color());
    return 1;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_set_color(lua_State *L) {
    OX_TRY
    check<T>(L, 1, sheet_base_mt<T>())->setColor(*check<XLColor>(L, 2, MT_COLOR));
    return 0;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_index(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<T>(L, 1, sheet_base_mt<T>())->index()));
    return 1;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_set_index(lua_State *L) {
    OX_TRY
    check<T>(L, 1, sheet_base_mt<T>())->setIndex(uint16_t(luaL_checkinteger(L, 2)));
    return 0;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_name(lua_State *L) {
    OX_TRY
    push_string(L, check<T>(L, 1, sheet_base_mt<T>())->name());
    return 1;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_set_name(lua_State *L) {
    OX_TRY
    check<T>(L, 1, sheet_base_mt<T>())->setName(check_string(L, 2));
    return 0;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_is_selected(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<T>(L, 1, sheet_base_mt<T>())->isSelected());
    return 1;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_set_selected(lua_State *L) {
    OX_TRY
    check<T>(L, 1, sheet_base_mt<T>())->setSelected(lua_toboolean(L, 2) != 0);
    return 0;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_is_active(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<T>(L, 1, sheet_base_mt<T>())->isActive());
    return 1;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_set_active(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<T>(L, 1, sheet_base_mt<T>())->setActive());
    return 1;
    OX_CATCH_ALL
}

template <typename T>
int sheet_base_clone(lua_State *L) {
    OX_TRY
    check<T>(L, 1, sheet_base_mt<T>())->clone(check_string(L, 2));
    return 0;
    OX_CATCH_ALL
}

#define SHEET_BASE_METHODS(T)                                              \
    { "visibility", sheet_base_visibility<T> },                            \
    { "visibility_num", sheet_base_visibility_num<T> },                    \
    { "set_visibility", sheet_base_set_visibility<T> },                    \
    { "color", sheet_base_color<T> },                                      \
    { "set_color", sheet_base_set_color<T> },                              \
    { "index", sheet_base_index<T> },                                      \
    { "set_index", sheet_base_set_index<T> },                              \
    { "name", sheet_base_name<T> },                                        \
    { "set_name", sheet_base_set_name<T> },                                \
    { "is_selected", sheet_base_is_selected<T> },                          \
    { "set_selected", sheet_base_set_selected<T> },                        \
    { "is_active", sheet_base_is_active<T> },                              \
    { "set_active", sheet_base_set_active<T> },                            \
    { "clone", sheet_base_clone<T> }

const luaL_Reg worksheet_base_methods[] = {
    SHEET_BASE_METHODS(XLWorksheet),
    { nullptr, nullptr },
};

const luaL_Reg chartsheet_methods[] = {
    SHEET_BASE_METHODS(XLChartsheet),
    { nullptr, nullptr },
};

}  // namespace

namespace ox {

void add_sheet_base_methods(lua_State *L, const char *tname) {
    luaL_getmetatable(L, tname);
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "__index");  // methods table
        if (lua_istable(L, -1)) {
            luaL_setfuncs(L, std::strcmp(tname, MT_WORKSHEET) == 0 ? worksheet_base_methods
                                                                  : chartsheet_methods,
                          0);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

void bind_document(lua_State *L) {
    register_type<XLDocument>(L, MT_DOCUMENT, doc_methods);
    register_type<XLWorkbook>(L, MT_WORKBOOK, wb_methods);
    register_type<XLSheet>(L, MT_SHEET, sheet_methods);
    register_type<XLChartsheet>(L, MT_CHARTSHEET, chartsheet_methods);

    // module function: openxlsx.new_document([path])
    lua_pushcfunction(L, doc_new);
    lua_setfield(L, 1, "new_document");
}

}  // namespace ox
