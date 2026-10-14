#pragma once
// =============================================================================
// lua_bind.hpp — shared binding infrastructure for lua-openxlsx
//
// Design notes
// ------------
// * Every C++ object exposed to Lua lives in a heap-allocated Box<T> stored
//   inside a Lua userdata; the __gc metamethod deletes it. OpenXLSX objects
//   are value types with internal shared state, so this is safe and simple.
// * All C++ exceptions are translated into Lua errors of the form
//   "XLInputError: <message>" (see OX_TRY / OX_CATCH_ALL).
// * Names are snake_case on the Lua side, mirroring OpenXLSX camelCase.
// =============================================================================

#include <lua.hpp>
#include <OpenXLSX.hpp>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace ox {

// ============================================================================
// Metatable (type) names
// ============================================================================
inline constexpr const char *MT_DOCUMENT            = "openxlsx.document";
inline constexpr const char *MT_WORKBOOK            = "openxlsx.workbook";
inline constexpr const char *MT_SHEET               = "openxlsx.sheet";
inline constexpr const char *MT_WORKSHEET           = "openxlsx.worksheet";
inline constexpr const char *MT_CHARTSHEET          = "openxlsx.chartsheet";
inline constexpr const char *MT_CELL                = "openxlsx.cell";
inline constexpr const char *MT_CELLVALUE           = "openxlsx.cell_value";
inline constexpr const char *MT_REFERENCE           = "openxlsx.reference";
inline constexpr const char *MT_FORMULA             = "openxlsx.formula";
inline constexpr const char *MT_DATETIME            = "openxlsx.datetime";
inline constexpr const char *MT_COLOR               = "openxlsx.color";
inline constexpr const char *MT_ROW                 = "openxlsx.row";
inline constexpr const char *MT_COLUMN              = "openxlsx.column";
inline constexpr const char *MT_RANGE               = "openxlsx.range";
inline constexpr const char *MT_ROW_RANGE           = "openxlsx.row_range";
inline constexpr const char *MT_ROW_DATA_RANGE      = "openxlsx.row_data_range";
inline constexpr const char *MT_MERGES              = "openxlsx.merges";
inline constexpr const char *MT_CONDITIONAL_FORMATS = "openxlsx.conditional_formats";
inline constexpr const char *MT_CONDITIONAL_FORMAT  = "openxlsx.conditional_format";
inline constexpr const char *MT_CF_RULES            = "openxlsx.cf_rules";
inline constexpr const char *MT_CF_RULE             = "openxlsx.cf_rule";
inline constexpr const char *MT_CELL_ITERATOR       = "openxlsx.cell_iterator";
inline constexpr const char *MT_ROW_ITERATOR        = "openxlsx.row_iterator";
inline constexpr const char *MT_ROW_REVERSE_ITERATOR = "openxlsx.row_reverse_iterator";
inline constexpr const char *MT_ROW_DATA_ITERATOR   = "openxlsx.row_data_iterator";

// ============================================================================
// Error translation
// ============================================================================
inline int raise(lua_State *L, const char *klass, const char *what) {
    std::string msg(klass);
    msg += ": ";
    msg += (what != nullptr ? what : "(no message)");
    lua_pushlstring(L, msg.data(), msg.size());
    return lua_error(L);
}
inline int raise(lua_State *L, const char *klass, const std::string &what) {
    return raise(L, klass, what.c_str());
}

#define OX_TRY try {

#define OX_CATCH_ALL                                                                                        \
    }                                                                                                       \
    catch (const OpenXLSX::XLInputError &e) { return ox::raise(L, "XLInputError", e.what()); }              \
    catch (const OpenXLSX::XLInternalError &e) { return ox::raise(L, "XLInternalError", e.what()); }        \
    catch (const OpenXLSX::XLValueTypeError &e) { return ox::raise(L, "XLValueTypeError", e.what()); }      \
    catch (const OpenXLSX::XLOverflowError &e) { return ox::raise(L, "XLOverflowError", e.what()); }        \
    catch (const OpenXLSX::XLCellAddressError &e) { return ox::raise(L, "XLCellAddressError", e.what()); }  \
    catch (const OpenXLSX::XLPropertyError &e) { return ox::raise(L, "XLPropertyError", e.what()); }        \
    catch (const OpenXLSX::XLSheetError &e) { return ox::raise(L, "XLSheetError", e.what()); }              \
    catch (const OpenXLSX::XLDateTimeError &e) { return ox::raise(L, "XLDateTimeError", e.what()); }        \
    catch (const OpenXLSX::XLFormulaError &e) { return ox::raise(L, "XLFormulaError", e.what()); }          \
    catch (const OpenXLSX::XLException &e) { return ox::raise(L, "XLException", e.what()); }                \
    catch (const std::bad_variant_access &) {                                                               \
        return ox::raise(L, "Error",                                                                        \
                         "std::bad_variant_access: the cell/value does not hold the requested type "         \
                         "(inspect it with cell:value_type() / value:type())");                             \
    }                                                                                                       \
    catch (const std::exception &e) { return ox::raise(L, "Error", e.what()); }                             \
    catch (...) { return ox::raise(L, "Error", "unknown C++ exception"); }

// ============================================================================
// Userdata box
//
// Every exposed C++ object lives inside a Box stored in a Lua userdata.
// Box is type-erased (void* + deleter) so that one generic __gc serves all
// types. Handles derived from an XLDocument carry a shared "document token";
// when the document is closed or garbage-collected the token is marked dead
// and any further use of a stale handle raises a Lua error instead of
// dereferencing freed XML data (which would segfault).
// ============================================================================
struct DocToken {
    bool alive { true };
};

struct Box {
    void *obj { nullptr };
    void (*deleter)(void *) { nullptr };
    std::shared_ptr<DocToken> token {};
    bool check_token { false };   // true for handles derived from a document
    bool owns_token { false };    // true for the document object itself
};

template <typename T>
inline void box_delete(void *p) {
    delete static_cast<T *>(p);
}

inline void box_destroy(Box *b) {
    if (b->owns_token && b->token) b->token->alive = false;
    if (b->obj != nullptr && b->deleter != nullptr) b->deleter(b->obj);
    b->obj = nullptr;
    b->token.reset();
}

inline int box_gc(lua_State *L) {
    auto *b = static_cast<Box *>(lua_touserdata(L, 1));
    if (b != nullptr) box_destroy(b);
    return 0;
}

template <typename T>
inline Box *box_new(lua_State *L, const char *tname) {
    void *ud = lua_newuserdatauv(L, sizeof(Box), 0);
    auto *b  = new (ud) Box();  // placement new: shared_ptr members need construction
    b->deleter = &box_delete<T>;
    luaL_setmetatable(L, tname);
    return b;
}

template <typename T>
inline T *check(lua_State *L, int idx, const char *tname) {
    auto *b = static_cast<Box *>(luaL_checkudata(L, idx, tname));
    if (b->obj == nullptr) luaL_error(L, "attempt to use an invalidated %s object", tname);
    if (b->check_token && b->token && !b->token->alive) {
        luaL_error(L, "the document this %s belongs to has been closed - handle is no longer valid", tname);
    }
    return static_cast<T *>(b->obj);
}

/** Returns nullptr instead of raising when the value is not a matching userdata. */
template <typename T>
inline T *maybe(lua_State *L, int idx, const char *tname) {
    auto *b = static_cast<Box *>(luaL_testudata(L, idx, tname));
    if (b == nullptr || b->obj == nullptr) return nullptr;
    if (b->check_token && b->token && !b->token->alive) return nullptr;
    return static_cast<T *>(b->obj);
}

/** Reads the document token carried by the object at idx (nullptr if none). */
inline std::shared_ptr<DocToken> token_of(lua_State *L, int idx) {
    if (lua_type(L, idx) != LUA_TUSERDATA) return nullptr;
    if (!lua_getmetatable(L, idx)) return nullptr;  // not one of our full userdata
    lua_pop(L, 1);
    return static_cast<Box *>(lua_touserdata(L, idx))->token;
}

/** Copies a value into a fresh userdata (uses the copy constructor). */
template <typename T>
inline T *push(lua_State *L, const char *tname, const T &value) {
    Box *b = box_new<T>(L, tname);
    b->obj = new T(value);
    return static_cast<T *>(b->obj);
}

/**
 * Constructs the object in place from constructor arguments.
 * Use this instead of push() for classes that must not be moved (XLDocument).
 */
template <typename T, typename... Args>
inline T *push_new(lua_State *L, const char *tname, Args &&...args) {
    Box *b = box_new<T>(L, tname);
    b->obj = new T(std::forward<Args>(args)...);
    return static_cast<T *>(b->obj);
}

/** Like push(), but the new handle shares the given document token. */
template <typename T>
inline T *push_live(lua_State *L, const char *tname, std::shared_ptr<DocToken> token, const T &value) {
    Box *b = box_new<T>(L, tname);
    b->obj    = new T(value);
    b->token  = std::move(token);
    b->check_token = true;
    return static_cast<T *>(b->obj);
}

/** For the document object itself: it owns the token (killed on close / gc). */
template <typename T>
inline void box_set_owner_token(lua_State *L, int idx, std::shared_ptr<DocToken> token) {
    auto *b = static_cast<Box *>(lua_touserdata(L, idx));
    b->token = std::move(token);
    b->check_token = false;
    b->owns_token = true;
}

/** Creates a fresh token generation for the document (invalidates old handles). */
template <typename T>
inline void box_new_token(lua_State *L, int idx) {
    auto *b = static_cast<Box *>(lua_touserdata(L, idx));
    if (b->token) b->token->alive = false;
    b->token = std::make_shared<DocToken>();
    b->check_token = false;
    b->owns_token = true;
}

/**
 * Registers a metatable with methods, __gc and optional custom __index /
 * __newindex. When index_fn is given it receives the methods table as upvalue 1
 * so it can look methods up before handling properties.
 */
template <typename T>
inline void register_type(lua_State *L, const char *tname, const luaL_Reg *methods,
                          lua_CFunction index_fn = nullptr, lua_CFunction newindex_fn = nullptr) {
    if (luaL_newmetatable(L, tname)) {
        const int mt = lua_gettop(L);
        lua_newtable(L);                       // methods table
        luaL_setfuncs(L, methods, 0);
        if (index_fn != nullptr) {
            lua_pushvalue(L, -1);              // methods table as upvalue
            lua_pushcclosure(L, index_fn, 1);
            lua_setfield(L, mt, "__index");
        } else {
            lua_pushvalue(L, -1);
            lua_setfield(L, mt, "__index");
        }
        if (newindex_fn != nullptr) {
            lua_pushcfunction(L, newindex_fn);
            lua_setfield(L, mt, "__newindex");
        }
        lua_pushcfunction(L, &box_gc);
        lua_setfield(L, mt, "__gc");
        lua_pop(L, 1);                         // methods table
        lua_pop(L, 1);                         // metatable
    }
}

// ============================================================================
// Small argument / result helpers
// ============================================================================
inline void push_string(lua_State *L, const std::string &s) {
    lua_pushlstring(L, s.data(), s.size());
}

inline void push_string(lua_State *L, const char *s) {
    lua_pushstring(L, s);
}

inline std::string check_string(lua_State *L, int idx) {
    size_t n = 0;
    const char *s = luaL_checklstring(L, idx, &n);
    return std::string(s, n);
}

inline std::string opt_string(lua_State *L, int idx, const std::string &dflt = std::string()) {
    if (lua_isnoneornil(L, idx)) return dflt;
    return check_string(L, idx);
}

inline bool opt_bool(lua_State *L, int idx, bool dflt = false) {
    return lua_isnoneornil(L, idx) ? dflt : (lua_toboolean(L, idx) != 0);
}

inline lua_Integer opt_integer(lua_State *L, int idx, lua_Integer dflt) {
    return lua_isnoneornil(L, idx) ? dflt : luaL_checkinteger(L, idx);
}

// ---- case/underscore-insensitive comparison for enum names -----------------
inline bool iequal_loose(const char *a, const char *b) {
    auto norm = [](char c) { return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c; };
    while (*a != '\0' && *b != '\0') {
        const char ca = *a;
        const char cb = *b;
        if (ca == '_' || ca == '-' || ca == ' ') { ++a; continue; }
        if (cb == '_' || cb == '-' || cb == ' ') { ++b; continue; }
        if (norm(ca) != norm(cb)) return false;
        ++a;
        ++b;
    }
    while (*a == '_' || *a == '-' || *a == ' ') ++a;
    while (*b == '_' || *b == '-' || *b == ' ') ++b;
    return *a == '\0' && *b == '\0';
}

/** Accepts a numeric enum value or a name string; returns the numeric value. */
inline int check_enum(lua_State *L, int idx, const char *const names[], int count, const char *what) {
    if (lua_isinteger(L, idx)) return int(lua_tointeger(L, idx));
    if (lua_type(L, idx) == LUA_TSTRING) {
        const char *s = lua_tostring(L, idx);
        for (int i = 0; i < count; ++i) {
            if (iequal_loose(s, names[i])) return i;
        }
        return luaL_error(L, "invalid %s name: '%s'", what, s);
    }
    return luaL_error(L, "invalid %s: expected a name string or a number", what);
}

// ============================================================================
// Enum name tables (must match OpenXLSX declaration order)
// ============================================================================
inline const char *const XL_PROPERTY_NAMES[] = {
    "Title",           "Subject",      "Creator",     "Keywords",    "Description",
    "LastModifiedBy",  "LastPrinted",  "CreationDate", "ModificationDate", "Category",
    "Application",     "DocSecurity",  "ScaleCrop",   "Manager",     "Company",
    "LinksUpToDate",   "SharedDoc",    "HyperlinkBase", "HyperlinksChanged", "AppVersion",
};
inline constexpr int XL_PROPERTY_COUNT = 20;

inline const char *const XL_VALUE_TYPE_NAMES[] = { "empty", "boolean", "integer", "float", "error", "string" };
inline constexpr int XL_VALUE_TYPE_COUNT = 6;

inline const char *const XL_SHEET_TYPE_NAMES[] = { "worksheet", "chartsheet", "dialogsheet", "macrosheet" };
inline constexpr int XL_SHEET_TYPE_COUNT = 4;

inline const char *const XL_SHEET_STATE_NAMES[] = { "visible", "hidden", "very_hidden" };
inline constexpr int XL_SHEET_STATE_COUNT = 3;

inline const char *const XL_CF_TYPE_NAMES[] = {
    "expression", "cell_is", "color_scale", "data_bar", "icon_set", "top10", "unique_values", "duplicate_values",
    "contains_text", "not_contains_text", "begins_with", "ends_with", "contains_blanks", "not_contains_blanks",
    "contains_errors", "not_contains_errors", "time_period", "above_average", "invalid",
};
inline constexpr int XL_CF_TYPE_COUNT = 19;

inline const char *const XL_CF_OPERATOR_NAMES[] = {
    "less_than", "less_than_or_equal", "equal", "not_equal", "greater_than_or_equal", "greater_than",
    "between", "not_between", "contains_text", "not_contains", "begins_with", "ends_with", "invalid",
};
inline constexpr int XL_CF_OPERATOR_COUNT = 13;

inline const char *const XL_CF_TIME_PERIOD_NAMES[] = {
    "today", "yesterday", "tomorrow", "last7_days", "this_month", "last_month", "next_month",
    "this_week", "last_week", "next_week", "invalid",
};
inline constexpr int XL_CF_TIME_PERIOD_COUNT = 11;

// Numeric values of XLCfType / XLCfOperator / XLCfTimePeriod (name index != value)
inline int xl_cf_type_value(int name_index) {
    static const int values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 255 };
    return values[name_index];
}
inline int xl_cf_operator_value(int name_index) {
    static const int values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 255 };
    return values[name_index];
}
inline int xl_cf_time_period_value(int name_index) {
    static const int values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 255 };
    return values[name_index];
}

// ============================================================================
// Address validation
//
// OpenXLSX does not validate cell/range addresses; passing garbage leads to
// undefined behaviour (observed: segfault). Every user-supplied address is
// validated here before it reaches the library.
// ============================================================================
inline bool is_valid_column_letters(const std::string &s) {
    if (s.empty() || s.size() > 3) return false;
    uint32_t col = 0;
    for (char ch : s) {
        if (ch >= 'a' && ch <= 'z') ch = char(ch - 'a' + 'A');
        if (ch < 'A' || ch > 'Z') return false;
        col = col * 26 + uint32_t(ch - 'A' + 1);
    }
    return col >= 1 && col <= 16384;
}

inline bool is_valid_row_digits(const std::string &s) {
    if (s.empty() || s.size() > 7) return false;
    uint64_t v = 0;
    for (char ch : s) {
        if (ch < '0' || ch > '9') return false;
        v = v * 10 + uint64_t(ch - '0');
        if (v > 1048576) return false;
    }
    return v >= 1;
}

inline bool is_valid_cell_address(const std::string &s) {
    size_t i = 0;
    while (i < s.size() && ((s[i] >= 'A' && s[i] <= 'Z') || (s[i] >= 'a' && s[i] <= 'z'))) ++i;
    if (i == 0 || i > 3) return false;
    return is_valid_column_letters(s.substr(0, i)) && is_valid_row_digits(s.substr(i));
}

inline bool is_valid_range_address(const std::string &s) {
    const size_t pos = s.find(':');
    if (pos == std::string::npos) return is_valid_cell_address(s);
    return is_valid_cell_address(s.substr(0, pos)) && is_valid_cell_address(s.substr(pos + 1));
}

inline void check_row_col_range(lua_State *L, lua_Integer row, lua_Integer col) {
    if (row < 1 || row > 1048576) luaL_error(L, "row out of range: %d (valid: 1..1048576)", int(row));
    if (col < 1 || col > 16384) luaL_error(L, "column out of range: %d (valid: 1..16384)", int(col));
}

inline std::string ascii_upper(std::string s) {
    for (char &c : s) {
        if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
    }
    return s;
}

inline std::string check_cell_address(lua_State *L, int idx, const char *what = "cell address") {
    const std::string s = ascii_upper(check_string(L, idx));
    if (!is_valid_cell_address(s)) luaL_error(L, "invalid %s: '%s'", what, s.c_str());
    return s;
}

inline std::string check_range_address(lua_State *L, int idx, const char *what = "range address") {
    const std::string s = ascii_upper(check_string(L, idx));
    if (!is_valid_range_address(s)) luaL_error(L, "invalid %s: '%s'", what, s.c_str());
    return s;
}

inline std::string check_column_letters(lua_State *L, int idx) {
    const std::string s = ascii_upper(check_string(L, idx));
    if (!is_valid_column_letters(s)) luaL_error(L, "invalid column reference: '%s'", s.c_str());
    return s;
}

inline std::string check_row_digits(lua_State *L, int idx) {
    const std::string s = check_string(L, idx);
    if (!is_valid_row_digits(s)) luaL_error(L, "invalid row reference: '%s'", s.c_str());
    return s;
}

// ============================================================================
// XLCellValue <-> Lua conversions
// ============================================================================
inline void push_cellvalue(lua_State *L, const OpenXLSX::XLCellValue &v) {
    switch (v.type()) {
        case OpenXLSX::XLValueType::Empty:   lua_pushnil(L); break;
        case OpenXLSX::XLValueType::Boolean: lua_pushboolean(L, v.get<bool>()); break;
        case OpenXLSX::XLValueType::Integer: lua_pushinteger(L, lua_Integer(v.get<int64_t>())); break;
        case OpenXLSX::XLValueType::Float:   lua_pushnumber(L, v.get<double>()); break;
        case OpenXLSX::XLValueType::String:
        case OpenXLSX::XLValueType::Error: {
            const std::string s = v.get<std::string>();
            lua_pushlstring(L, s.data(), s.size());
            break;
        }
    }
}

inline void push_cellvalue_proxy(lua_State *L, const OpenXLSX::XLCellValueProxy &v) {
    switch (v.type()) {
        case OpenXLSX::XLValueType::Empty:   lua_pushnil(L); break;
        case OpenXLSX::XLValueType::Boolean: lua_pushboolean(L, v.get<bool>()); break;
        case OpenXLSX::XLValueType::Integer: lua_pushinteger(L, lua_Integer(v.get<int64_t>())); break;
        case OpenXLSX::XLValueType::Float:   lua_pushnumber(L, v.get<double>()); break;
        case OpenXLSX::XLValueType::String:
        case OpenXLSX::XLValueType::Error: {
            const std::string s = v.get<std::string>();
            lua_pushlstring(L, s.data(), s.size());
            break;
        }
    }
}

/** Converts the Lua value at idx to an XLCellValue (nil -> Empty). */
inline OpenXLSX::XLCellValue check_cellvalue(lua_State *L, int idx) {
    switch (lua_type(L, idx)) {
        case LUA_TNIL:
            return OpenXLSX::XLCellValue{};
        case LUA_TBOOLEAN:
            return OpenXLSX::XLCellValue{ lua_toboolean(L, idx) != 0 };
        case LUA_TNUMBER:
            if (lua_isinteger(L, idx)) return OpenXLSX::XLCellValue{ int64_t(lua_tointeger(L, idx)) };
            return OpenXLSX::XLCellValue{ double(lua_tonumber(L, idx)) };
        case LUA_TSTRING: {
            size_t n = 0;
            const char *s = lua_tolstring(L, idx, &n);
            return OpenXLSX::XLCellValue{ std::string(s, n) };
        }
        case LUA_TUSERDATA: {
            if (auto *dt = maybe<OpenXLSX::XLDateTime>(L, idx, MT_DATETIME)) return OpenXLSX::XLCellValue{ *dt };
            if (auto *cv = maybe<OpenXLSX::XLCellValue>(L, idx, MT_CELLVALUE)) return *cv;
            break;
        }
        default:
            break;
    }
    luaL_error(L, "cannot convert value at stack index %d to a cell value", idx);
    return OpenXLSX::XLCellValue{};
}

/** Reads a Lua array (1..n) of cell values. */
inline std::vector<OpenXLSX::XLCellValue> check_value_array(lua_State *L, int idx) {
    luaL_checktype(L, idx, LUA_TTABLE);
    const lua_Integer n = luaL_len(L, idx);
    std::vector<OpenXLSX::XLCellValue> out;
    if (n > 0) out.reserve(size_t(n));
    for (lua_Integer i = 1; i <= n; ++i) {
        lua_geti(L, idx, i);
        out.push_back(check_cellvalue(L, -1));
        lua_pop(L, 1);
    }
    return out;
}

inline void push_value_array(lua_State *L, const std::vector<OpenXLSX::XLCellValue> &values) {
    lua_createtable(L, int(values.size()), 0);
    int i = 1;
    for (const auto &v : values) {
        push_cellvalue(L, v);
        lua_seti(L, -2, i++);
    }
}

// ============================================================================
// XLDateTime <-> Lua conversions
// ============================================================================
inline void push_datetime_table(lua_State *L, const OpenXLSX::XLDateTime &dt) {
    const std::tm t = dt.tm();
    lua_createtable(L, 0, 7);
    lua_pushinteger(L, t.tm_year + 1900);
    lua_setfield(L, -2, "year");
    lua_pushinteger(L, t.tm_mon + 1);
    lua_setfield(L, -2, "month");
    lua_pushinteger(L, t.tm_mday);
    lua_setfield(L, -2, "day");
    lua_pushinteger(L, t.tm_hour);
    lua_setfield(L, -2, "hour");
    lua_pushinteger(L, t.tm_min);
    lua_setfield(L, -2, "min");
    lua_pushinteger(L, t.tm_sec);
    lua_setfield(L, -2, "sec");
    lua_pushnumber(L, dt.serial());
    lua_setfield(L, -2, "serial");
}

/** Accepts an Excel serial number, a Lua table {year,month,day,hour,min,sec}, or an XLDateTime userdata. */
inline OpenXLSX::XLDateTime check_datetime(lua_State *L, int idx) {
    if (auto *dt = maybe<OpenXLSX::XLDateTime>(L, idx, MT_DATETIME)) return *dt;
    if (lua_type(L, idx) == LUA_TNUMBER) return OpenXLSX::XLDateTime(double(lua_tonumber(L, idx)));
    if (lua_type(L, idx) == LUA_TTABLE) {
        auto getfield = [&](const char *key, lua_Integer dflt) -> lua_Integer {
            lua_getfield(L, idx, key);
            const lua_Integer v = lua_isnil(L, -1) ? dflt : luaL_checkinteger(L, -1);
            lua_pop(L, 1);
            return v;
        };
        std::tm t {};
        t.tm_year  = int(getfield("year", 1900)) - 1900;
        t.tm_mon   = int(getfield("month", 1)) - 1;
        t.tm_mday  = int(getfield("day", 1));
        t.tm_hour  = int(getfield("hour", 0));
        t.tm_min   = int(getfield("min", 0));
        t.tm_sec   = int(getfield("sec", 0));
        t.tm_isdst = 0;
        return OpenXLSX::XLDateTime(t);
    }
    luaL_error(L, "datetime expected: number (Excel serial), table {year,month,day,hour,min,sec} or datetime object");
    return OpenXLSX::XLDateTime{};
}

// ============================================================================
// Closure-based Lua iterators over C++ iterator pairs
// ============================================================================
/**
 * Builds a Lua closure performing a generic-for over [it, last).  The two C++
 * iterators are boxed into userdata that serve as closure upvalues; the
 * producer function must return the loop values for the current position and
 * then advance the iterator.
 */
template <typename Iter, typename Producer>
inline void push_closure_iterator(lua_State *L, const char *tname, std::shared_ptr<DocToken> token,
                                  const Iter &begin, const Iter &end, Producer producer) {
    push_live<Iter>(L, tname, token, begin);            // upvalue 1
    push_live<Iter>(L, tname, std::move(token), end);   // upvalue 2
    lua_pushcclosure(L, producer, 2);
}

/** Generic "are we at the end?" check used by closure producers. */
template <typename Iter>
inline bool iter_at_end(lua_State *L, const char *tname) {
    auto *it   = check<Iter>(L, lua_upvalueindex(1), tname);
    auto *last = check<Iter>(L, lua_upvalueindex(2), tname);
    return *it == *last;
}

// Implemented in bind_document.cpp: adds the shared XLSheetBase<> methods to the
// methods table of the worksheet / chartsheet metatables.
void add_sheet_base_methods(lua_State *L, const char *tname);

}  // namespace ox
