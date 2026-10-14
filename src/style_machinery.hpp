#pragma once
// =============================================================================
// style_machinery.hpp — generic getter/setter + property binding for the
// "style-like" OpenXLSX classes (XLStyles.hpp, XLDrawing.hpp).
//
// Classes bind their members through StyleMember tables:
//     OX_PROP(C, lua_name, getter, setter)     - getter + unary setter
//     OX_PROP_RO(C, lua_name, getter)          - getter only
//     { "name", getter_fn, setter_fn, prop_settable }  - manual entry
//
// Lua side per member:
//     obj.name            (property read)
//     obj.name = v        (property write, unary setters only)
//     obj:get_name()      (explicit getter)
//     obj:set_name(v)     (explicit setter, any arity)
//
// Each participating class must specialize `ox::style_mt<T>()` with its
// metatable name; object-returning getters are detected through it.
// =============================================================================
#include "lua_bind.hpp"

#include <tuple>
#include <type_traits>

namespace ox {

// ---------------------------------------------------------------------------
// metatable name lookup + object trait
//
// All participating types are enumerated here (single definition point) because
// a SFINAE check on `style_mt<V>()` cannot work: decltype does not require the
// function definition, so it would report true for unrelated types too.
// ---------------------------------------------------------------------------
template <typename T>
constexpr const char *style_mt();

template <typename V>
struct is_style_object : std::false_type {};

#define OX_OBJECT_TYPE(T, MT)                     \
    template <>                                   \
    constexpr const char *style_mt<T>() {         \
        return MT;                                \
    }                                             \
    template <>                                   \
    struct is_style_object<T> : std::true_type {};

// XLStyles.hpp
OX_OBJECT_TYPE(OpenXLSX::XLColor, MT_COLOR)
OX_OBJECT_TYPE(OpenXLSX::XLStyles, MT_STYLES)
OX_OBJECT_TYPE(OpenXLSX::XLNumberFormat, MT_NUMBER_FORMAT)
OX_OBJECT_TYPE(OpenXLSX::XLNumberFormats, MT_NUMBER_FORMATS)
OX_OBJECT_TYPE(OpenXLSX::XLFont, MT_FONT)
OX_OBJECT_TYPE(OpenXLSX::XLFonts, MT_FONTS)
OX_OBJECT_TYPE(OpenXLSX::XLFill, MT_FILL)
OX_OBJECT_TYPE(OpenXLSX::XLFills, MT_FILLS)
OX_OBJECT_TYPE(OpenXLSX::XLLine, MT_LINE)
OX_OBJECT_TYPE(OpenXLSX::XLBorder, MT_BORDER)
OX_OBJECT_TYPE(OpenXLSX::XLBorders, MT_BORDERS)
OX_OBJECT_TYPE(OpenXLSX::XLAlignment, MT_ALIGNMENT)
OX_OBJECT_TYPE(OpenXLSX::XLCellFormat, MT_CELL_FORMAT)
OX_OBJECT_TYPE(OpenXLSX::XLCellFormats, MT_CELL_FORMATS)
OX_OBJECT_TYPE(OpenXLSX::XLCellStyle, MT_CELL_STYLE)
OX_OBJECT_TYPE(OpenXLSX::XLCellStyles, MT_CELL_STYLES)
OX_OBJECT_TYPE(OpenXLSX::XLDiffCellFormat, MT_DIFF_CELL_FORMAT)
OX_OBJECT_TYPE(OpenXLSX::XLDiffCellFormats, MT_DIFF_CELL_FORMATS)
OX_OBJECT_TYPE(OpenXLSX::XLGradientStop, MT_GRADIENT_STOP)
OX_OBJECT_TYPE(OpenXLSX::XLGradientStops, MT_GRADIENT_STOPS)
OX_OBJECT_TYPE(OpenXLSX::XLDataBarColor, MT_DATA_BAR_COLOR)
// XLDrawing.hpp
OX_OBJECT_TYPE(OpenXLSX::XLShapeStyle, MT_SHAPE_STYLE)
OX_OBJECT_TYPE(OpenXLSX::XLShapeClientData, MT_SHAPE_CLIENT_DATA)
#undef OX_OBJECT_TYPE

// ---------------------------------------------------------------------------
// enum name tables
// ---------------------------------------------------------------------------
struct EnumEntry {
    const char *name;
    long value;
};

template <typename E>
struct enum_traits_closed;  // specialized per enum, must be declared in namespace ox

#define OX_ENUM_TABLE(E, ...)                                         \
    template <>                                                       \
    struct enum_traits_closed<E> {                                    \
        static const EnumEntry *table() {                             \
            static const EnumEntry t[] = { __VA_ARGS__, { nullptr, 0 } }; \
            return t;                                                 \
        }                                                             \
    };

template <typename E>
inline E check_enum_arg(lua_State *L, int idx) {
    if (lua_isinteger(L, idx)) return E(lua_tointeger(L, idx));
    const std::string s = check_string(L, idx);
    for (const EnumEntry *e = enum_traits_closed<E>::table(); e->name != nullptr; ++e) {
        if (iequal_loose(s.c_str(), e->name)) return E(e->value);
    }
    return luaL_error(L, "invalid enum name: '%s'", s.c_str()), E(0);
}

// ---------------------------------------------------------------------------
// value push / argument check
// ---------------------------------------------------------------------------
template <typename V>
inline void push_style_value(lua_State *L, const V &v) {
    using U = std::decay_t<V>;
    if constexpr (std::is_same_v<U, bool>) {
        lua_pushboolean(L, v);
    } else if constexpr (std::is_enum_v<U>) {
        lua_pushinteger(L, lua_Integer(static_cast<std::underlying_type_t<U>>(v)));
    } else if constexpr (std::is_integral_v<U>) {
        lua_pushinteger(L, lua_Integer(v));
    } else if constexpr (std::is_floating_point_v<U>) {
        lua_pushnumber(L, double(v));
    } else if constexpr (std::is_same_v<U, std::string>) {
        push_string(L, v);
    } else {
        static_assert(sizeof(U) == 0, "unsupported style getter return type");
    }
}

template <typename A>
inline A check_style_arg(lua_State *L, int idx) {
    using U = std::decay_t<A>;
    if constexpr (std::is_same_v<U, bool>) {
        // upstream bool setter parameters default to `true`
        return lua_isnoneornil(L, idx) ? true : (lua_toboolean(L, idx) != 0);
    } else if constexpr (std::is_enum_v<U>) {
        return check_enum_arg<U>(L, idx);
    } else if constexpr (std::is_integral_v<U>) {
        return U(luaL_checkinteger(L, idx));
    } else if constexpr (std::is_floating_point_v<U>) {
        return lua_isnoneornil(L, idx) ? U(0) : U(luaL_checknumber(L, idx));
    } else if constexpr (std::is_same_v<U, std::string>) {
        return check_string(L, idx);
    } else if constexpr (std::is_same_v<U, OpenXLSX::XLColor>) {
        if (auto *c = maybe<OpenXLSX::XLColor>(L, idx, MT_COLOR)) return *c;
        if (lua_type(L, idx) == LUA_TSTRING) return OpenXLSX::XLColor(check_string(L, idx));
        luaL_error(L, "expected a color object or 'RRGGBB'/'AARRGGBB' string");
        return OpenXLSX::XLColor();
    } else {
        static_assert(sizeof(U) == 0, "unsupported style setter argument type");
    }
}

// ---------------------------------------------------------------------------
// member function traits
// ---------------------------------------------------------------------------
template <typename F>
struct mem_fn_arg_tuple;

template <typename R, typename C, typename... A>
struct mem_fn_arg_tuple<R (C::*)(A...)> {
    using args = std::tuple<A...>;
};
template <typename R, typename C, typename... A>
struct mem_fn_arg_tuple<R (C::*)(A...) const> {
    using args = std::tuple<A...>;
};

template <auto F>
struct mem_fn_args : mem_fn_arg_tuple<decltype(F)> {};

template <auto F>
constexpr bool setter_is_unary() {
    return std::tuple_size_v<typename mem_fn_args<F>::args> == 1;
}

// ---------------------------------------------------------------------------
// generic getter / setter wrappers
// ---------------------------------------------------------------------------
template <typename C, auto Getter>
inline int style_get(lua_State *L) {
    OX_TRY
    auto *obj = check<C>(L, 1, style_mt<C>());
    using R = decltype((obj->*Getter)());
    using V = std::decay_t<R>;
    if constexpr (is_style_object<V>::value) {
        if constexpr (std::is_reference_v<R>) {
            // reference to a member of the owner: expose a live, non-owning handle
            // (upstream containers cache their children, copies would go stale)
            V &ref = (obj->*Getter)();
            push_ref<V>(L, style_mt<V>(), token_of(L, 1), &ref, 1);
        } else {
            const V value = (obj->*Getter)();  // copy; XML nodes are shared underneath
            push_live<V>(L, style_mt<V>(), token_of(L, 1), value);
        }
        return 1;
    } else {
        push_style_value(L, (obj->*Getter)());
        return 1;
    }
    OX_CATCH_ALL
}

template <typename C, auto Setter, std::size_t... I>
auto call_style_setter(lua_State *L, C *obj, std::index_sequence<I...>) {
    using Args = typename mem_fn_args<Setter>::args;
    return (obj->*Setter)(check_style_arg<std::tuple_element_t<I, Args>>(L, 2 + int(I))...);
}

template <typename C, auto Setter>
inline int style_set(lua_State *L) {
    OX_TRY
    auto *obj = check<C>(L, 1, style_mt<C>());
    using Args = typename mem_fn_args<Setter>::args;
    constexpr auto seq = std::make_index_sequence<std::tuple_size_v<Args>>{};
    if constexpr (std::is_void_v<decltype(call_style_setter<C, Setter>(L, obj, seq))>) {
        call_style_setter<C, Setter>(L, obj, seq);
        return 0;
    } else {
        const auto ok = call_style_setter<C, Setter>(L, obj, seq);
        push_style_value(L, ok);
        return 1;
    }
    OX_CATCH_ALL
}

// ---------------------------------------------------------------------------
// property machinery:  obj.name  /  obj.name = value
// ---------------------------------------------------------------------------
struct StyleMember {
    const char *name;
    lua_CFunction getter;
    lua_CFunction setter;
    bool prop_settable;  // setter takes exactly one argument
};

template <typename T>
int style_index(lua_State *L) {
    if (lua_type(L, 2) == LUA_TSTRING) {
        // methods first
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (!lua_isnil(L, -1)) {
            lua_remove(L, -2);
            return 1;
        }
        lua_pop(L, 2);
        // then property getter
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);                 // [props, entry]
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "get");    // [props, entry, get]
            lua_remove(L, -2);             // [props, get]
            if (!lua_isnil(L, -1)) {
                lua_pushvalue(L, 1);       // [props, get, self]
                lua_call(L, 1, 1);         // [props, result]
                lua_remove(L, -2);
                return 1;
            }
        }
        lua_pop(L, 2);
    }
    lua_pushnil(L);
    return 1;
}

template <typename T>
int style_newindex(lua_State *L) {
    if (lua_type(L, 2) == LUA_TSTRING) {
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);                 // [props, entry]
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "set");
            lua_remove(L, -2);             // [props, setter]
            if (!lua_isnil(L, -1)) {
                lua_pushvalue(L, 1);       // self
                lua_pushvalue(L, 3);       // value
                lua_call(L, 2, 0);
                lua_pop(L, 1);             // props
                return 0;
            }
        }
        lua_pop(L, 2);
        return luaL_error(L, "style property '%s' is read-only or does not exist", lua_tostring(L, 2));
    }
    return luaL_error(L, "style objects only accept string property names");
}

template <typename T>
void register_style_type(lua_State *L, const char *tname, const StyleMember *members, size_t count,
                         const luaL_Reg *extra_methods = nullptr) {
    if (!luaL_newmetatable(L, tname)) return;
    const int mt = lua_gettop(L);
    lua_newtable(L);
    const int methods = lua_gettop(L);
    lua_newtable(L);
    const int props = lua_gettop(L);

    if (extra_methods != nullptr) {
        lua_pushvalue(L, methods);  // luaL_setfuncs works on the top of the stack
        luaL_setfuncs(L, extra_methods, 0);
        lua_pop(L, 1);
    }

    for (size_t i = 0; i < count; ++i) {
        const StyleMember &m = members[i];
        if (m.getter != nullptr) {
            const std::string get_name = std::string("get_") + m.name;
            lua_pushcfunction(L, m.getter);
            lua_setfield(L, methods, get_name.c_str());
            lua_newtable(L);  // props entry
            lua_pushcfunction(L, m.getter);
            lua_setfield(L, -2, "get");
            if (m.setter != nullptr && m.prop_settable) {
                lua_pushcfunction(L, m.setter);
                lua_setfield(L, -2, "set");
            }
            lua_setfield(L, props, m.name);
        }
        if (m.setter != nullptr) {
            const std::string set_name = std::string("set_") + m.name;
            lua_pushcfunction(L, m.setter);
            lua_setfield(L, methods, set_name.c_str());
            if (m.getter == nullptr) {
                lua_newtable(L);
                if (m.prop_settable) {
                    lua_pushcfunction(L, m.setter);
                    lua_setfield(L, -2, "set");
                }
                lua_setfield(L, props, m.name);
            }
        }
    }

    lua_pushvalue(L, methods);
    lua_pushvalue(L, props);
    lua_pushcclosure(L, style_index<T>, 2);
    lua_setfield(L, mt, "__index");

    lua_pushvalue(L, methods);
    lua_pushvalue(L, props);
    lua_pushcclosure(L, style_newindex<T>, 2);
    lua_setfield(L, mt, "__newindex");

    lua_pushcfunction(L, &box_gc);
    lua_setfield(L, mt, "__gc");

    lua_pop(L, 2);  // props, methods
    lua_pop(L, 1);  // metatable
}

/** Installs a __len metamethod calling the given count-style function. */
inline void set_len_metamethod(lua_State *L, const char *tname, lua_CFunction fn) {
    luaL_getmetatable(L, tname);
    lua_pushcfunction(L, fn);
    lua_setfield(L, -2, "__len");
    lua_pop(L, 1);
}

/** Adds plain methods to a style type's methods table (__index closure upvalue 1). */
inline void add_style_methods(lua_State *L, const char *tname, const luaL_Reg *methods) {
    luaL_getmetatable(L, tname);
    lua_getfield(L, -1, "__index");   // index closure
    lua_getupvalue(L, -1, 1);         // methods table
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 3);
}

}  // namespace ox

// ---------------------------------------------------------------------------
// member table macros (use inside a TU that has `using namespace ox;`)
// ---------------------------------------------------------------------------
#define OX_PROP(C, n, g, s) { #n, &style_get<C, &C::g>, &style_set<C, &C::s>, setter_is_unary<&C::s>() }
#define OX_PROP_RO(C, n, g) { #n, &style_get<C, &C::g>, nullptr, false }
#define OX_PROP_WO(C, n, s) { #n, nullptr, &style_set<C, &C::s>, setter_is_unary<&C::s>() }
