// =============================================================================
// bind_styles.cpp — XLStyles and friends (fonts, fills, borders, number
//                   formats, alignments, cell formats/styles, dxfs, gradients)
//
// The generic getter/setter + property machinery lives in style_machinery.hpp.
// =============================================================================
#include "lua_bind.hpp"
#include "style_machinery.hpp"

#include <cstring>
#include <iterator>

using namespace OpenXLSX;
using namespace ox;

// (style_mt / is_style_object specializations live in style_machinery.hpp)

// ---------------------------------------------------------------------------
// enum name tables
// ---------------------------------------------------------------------------
namespace ox {

OX_ENUM_TABLE(XLUnderlineStyle,
              { "none", XLUnderlineNone }, { "single", XLUnderlineSingle },
              { "double", XLUnderlineDouble }, { "invalid", XLUnderlineInvalid })
OX_ENUM_TABLE(XLFontSchemeStyle,
              { "none", XLFontSchemeNone }, { "major", XLFontSchemeMajor },
              { "minor", XLFontSchemeMinor }, { "invalid", XLFontSchemeInvalid })
OX_ENUM_TABLE(XLVerticalAlignRunStyle,
              { "baseline", XLBaseline }, { "subscript", XLSubscript },
              { "superscript", XLSuperscript }, { "invalid", XLVerticalAlignRunInvalid })
OX_ENUM_TABLE(XLFillType,
              { "gradient", XLGradientFill }, { "pattern", XLPatternFill },
              { "invalid", XLFillTypeInvalid })
OX_ENUM_TABLE(XLGradientType,
              { "linear", XLGradientLinear }, { "path", XLGradientPath },
              { "invalid", XLGradientTypeInvalid })
OX_ENUM_TABLE(XLPatternType,
              { "none", XLPatternNone }, { "solid", XLPatternSolid },
              { "medium_gray", XLPatternMediumGray }, { "dark_gray", XLPatternDarkGray },
              { "light_gray", XLPatternLightGray }, { "dark_horizontal", XLPatternDarkHorizontal },
              { "dark_vertical", XLPatternDarkVertical }, { "dark_down", XLPatternDarkDown },
              { "dark_up", XLPatternDarkUp }, { "dark_grid", XLPatternDarkGrid },
              { "dark_trellis", XLPatternDarkTrellis }, { "light_horizontal", XLPatternLightHorizontal },
              { "light_vertical", XLPatternLightVertical }, { "light_down", XLPatternLightDown },
              { "light_up", XLPatternLightUp }, { "light_grid", XLPatternLightGrid },
              { "light_trellis", XLPatternLightTrellis }, { "gray125", XLPatternGray125 },
              { "gray0625", XLPatternGray0625 }, { "invalid", XLPatternTypeInvalid })
OX_ENUM_TABLE(XLLineType,
              { "left", XLLineLeft }, { "right", XLLineRight }, { "top", XLLineTop },
              { "bottom", XLLineBottom }, { "diagonal", XLLineDiagonal },
              { "vertical", XLLineVertical }, { "horizontal", XLLineHorizontal },
              { "invalid", XLLineInvalid })
OX_ENUM_TABLE(XLLineStyle,
              { "none", XLLineStyleNone }, { "thin", XLLineStyleThin }, { "medium", XLLineStyleMedium },
              { "dashed", XLLineStyleDashed }, { "dotted", XLLineStyleDotted },
              { "thick", XLLineStyleThick }, { "double", XLLineStyleDouble },
              { "hair", XLLineStyleHair }, { "medium_dashed", XLLineStyleMediumDashed },
              { "dash_dot", XLLineStyleDashDot }, { "medium_dash_dot", XLLineStyleMediumDashDot },
              { "dash_dot_dot", XLLineStyleDashDotDot },
              { "medium_dash_dot_dot", XLLineStyleMediumDashDotDot },
              { "slant_dash_dot", XLLineStyleSlantDashDot }, { "invalid", XLLineStyleInvalid })
OX_ENUM_TABLE(XLAlignmentStyle,
              { "general", XLAlignGeneral }, { "left", XLAlignLeft }, { "right", XLAlignRight },
              { "center", XLAlignCenter }, { "top", XLAlignTop }, { "bottom", XLAlignBottom },
              { "fill", XLAlignFill }, { "justify", XLAlignJustify },
              { "center_continuous", XLAlignCenterContinuous }, { "distributed", XLAlignDistributed },
              { "invalid", XLAlignInvalid })
OX_ENUM_TABLE(XLReadingOrder,
              { "contextual", XLReadingOrderContextual },
              { "left_to_right", XLReadingOrderLeftToRight },
              { "right_to_left", XLReadingOrderRightToLeft })

}  // namespace ox

namespace {

// ============================================================================
// hand-written helpers for members with non-standard signatures
// ============================================================================
int font_set_underline(lua_State *L) {
    OX_TRY
    auto *font = check<XLFont>(L, 1, MT_FONT);
    const XLUnderlineStyle style = lua_isnoneornil(L, 2) ? XLUnderlineSingle : check_enum_arg<XLUnderlineStyle>(L, 2);
    lua_pushboolean(L, font->setUnderline(style));
    return 1;
    OX_CATCH_ALL
}

int line_is_set(lua_State *L) {
    OX_TRY
    auto *line = check<XLLine>(L, 1, MT_LINE);
    lua_pushboolean(L, bool(*line));
    return 1;
    OX_CATCH_ALL
}

int cell_format_alignment(lua_State *L) {
    OX_TRY
    auto *fmt = check<XLCellFormat>(L, 1, MT_CELL_FORMAT);
    // NOTE: upstream defaults createIfMissing to false; here it defaults to true
    // because almost every use is "style this cell format".
    push_live<XLAlignment>(L, MT_ALIGNMENT, token_of(L, 1), fmt->alignment(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}

int number_format_by_id(lua_State *L) {
    OX_TRY
    auto *fmts = check<XLNumberFormats>(L, 1, MT_NUMBER_FORMATS);
    push_live<XLNumberFormat>(L, MT_NUMBER_FORMAT, token_of(L, 1),
                              fmts->numberFormatById(uint32_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

int number_format_id_from_index(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLNumberFormats>(L, 1, MT_NUMBER_FORMATS)
                                       ->numberFormatIdFromIndex(XLStyleIndex(luaL_checkinteger(L, 2)))));
    return 1;
    OX_CATCH_ALL
}

int data_bar_color_set(lua_State *L) {
    OX_TRY
    auto *c = check<XLDataBarColor>(L, 1, MT_DATA_BAR_COLOR);
    lua_pushboolean(L, c->set(check_style_arg<XLColor>(L, 2)));
    return 1;
    OX_CATCH_ALL
}

// ---------------------------------------------------------------------------
// Border line setters.
// NOTE: upstream setLine() reports failure when the (optional) tint is 0.0,
// because an empty tint attribute cannot be written - although the line style
// and color were applied successfully. These wrappers therefore verify the
// actual result instead of returning the upstream flag.
// ---------------------------------------------------------------------------
#define OX_BORDER_LINE_SETTER(fn_name, setter, getter)                                  \
    int fn_name(lua_State *L) {                                                         \
        OX_TRY                                                                          \
        auto *b = check<XLBorder>(L, 1, MT_BORDER);                                     \
        const XLLineStyle style = check_style_arg<XLLineStyle>(L, 2);                   \
        const XLColor color = check_style_arg<XLColor>(L, 3);                           \
        const double tint = lua_isnoneornil(L, 4) ? 0.0 : luaL_checknumber(L, 4);       \
        b->setter(style, color, tint);                                                  \
        lua_pushboolean(L, b->getter().style() == style);                               \
        return 1;                                                                       \
        OX_CATCH_ALL                                                                    \
    }

OX_BORDER_LINE_SETTER(border_set_left, setLeft, left)
OX_BORDER_LINE_SETTER(border_set_right, setRight, right)
OX_BORDER_LINE_SETTER(border_set_top, setTop, top)
OX_BORDER_LINE_SETTER(border_set_bottom, setBottom, bottom)
OX_BORDER_LINE_SETTER(border_set_diagonal, setDiagonal, diagonal)
OX_BORDER_LINE_SETTER(border_set_vertical, setVertical, vertical)
OX_BORDER_LINE_SETTER(border_set_horizontal, setHorizontal, horizontal)
#undef OX_BORDER_LINE_SETTER

int border_set_line(lua_State *L) {
    OX_TRY
    auto *b = check<XLBorder>(L, 1, MT_BORDER);
    const XLLineType line_type = check_enum_arg<XLLineType>(L, 2);
    const XLLineStyle style = check_style_arg<XLLineStyle>(L, 3);
    const XLColor color = check_style_arg<XLColor>(L, 4);
    const double tint = lua_isnoneornil(L, 5) ? 0.0 : luaL_checknumber(L, 5);
    b->setLine(line_type, style, color, tint);
    XLLine line;
    switch (line_type) {
        case XLLineLeft:       line = b->left();       break;
        case XLLineRight:      line = b->right();      break;
        case XLLineTop:        line = b->top();        break;
        case XLLineBottom:     line = b->bottom();     break;
        case XLLineDiagonal:   line = b->diagonal();   break;
        case XLLineVertical:   line = b->vertical();   break;
        case XLLineHorizontal: line = b->horizontal(); break;
        default:               lua_pushboolean(L, false); return 1;
    }
    lua_pushboolean(L, line.style() == style);
    return 1;
    OX_CATCH_ALL
}

// ============================================================================
// containers
// ============================================================================
#define OX_DEFINE_CONTAINER(C, ITEM, BY_INDEX)                                                    \
    int C##_count(lua_State *L) {                                                                 \
        OX_TRY                                                                                    \
        lua_pushinteger(L, lua_Integer(check<C>(L, 1, style_mt<C>())->count()));                   \
        return 1;                                                                                 \
        OX_CATCH_ALL                                                                              \
    }                                                                                             \
    int C##_get(lua_State *L) {                                                                   \
        OX_TRY                                                                                    \
        auto *obj = check<C>(L, 1, style_mt<C>());                                                \
        push_live<ITEM>(L, style_mt<ITEM>(), token_of(L, 1),                                      \
                        obj->BY_INDEX(XLStyleIndex(luaL_checkinteger(L, 2))));                    \
        return 1;                                                                                 \
        OX_CATCH_ALL                                                                              \
    }                                                                                             \
    int C##_create(lua_State *L) {                                                                \
        OX_TRY                                                                                    \
        auto *obj = check<C>(L, 1, style_mt<C>());                                                \
        XLStyleIndex index;                                                                       \
        if (auto *copy_from = maybe<ITEM>(L, 2, style_mt<ITEM>())) {                              \
            index = obj->create(*copy_from);                                                      \
        } else {                                                                                  \
            index = obj->create();                                                                \
        }                                                                                         \
        lua_pushinteger(L, lua_Integer(index));                                                   \
        return 1;                                                                                 \
        OX_CATCH_ALL                                                                              \
    }                                                                                             \
    const luaL_Reg C##_methods[] = {                                                              \
        { "count", C##_count }, { "get", C##_get }, { "create", C##_create }, { nullptr, nullptr } \
    };

OX_DEFINE_CONTAINER(XLNumberFormats, XLNumberFormat, numberFormatByIndex)
OX_DEFINE_CONTAINER(XLFonts, XLFont, fontByIndex)
OX_DEFINE_CONTAINER(XLFills, XLFill, fillByIndex)
OX_DEFINE_CONTAINER(XLBorders, XLBorder, borderByIndex)
OX_DEFINE_CONTAINER(XLCellFormats, XLCellFormat, cellFormatByIndex)
OX_DEFINE_CONTAINER(XLCellStyles, XLCellStyle, cellStyleByIndex)
OX_DEFINE_CONTAINER(XLDiffCellFormats, XLDiffCellFormat, diffCellFormatByIndex)
OX_DEFINE_CONTAINER(XLGradientStops, XLGradientStop, stopByIndex)

int number_formats_len(lua_State *L) { return XLNumberFormats_count(L); }
int fonts_len(lua_State *L) { return XLFonts_count(L); }
int fills_len(lua_State *L) { return XLFills_count(L); }
int borders_len(lua_State *L) { return XLBorders_count(L); }
int cell_formats_len(lua_State *L) { return XLCellFormats_count(L); }
int cell_styles_len(lua_State *L) { return XLCellStyles_count(L); }
int diff_cell_formats_len(lua_State *L) { return XLDiffCellFormats_count(L); }
int gradient_stops_len(lua_State *L) { return XLGradientStops_count(L); }

// ============================================================================
// member tables
// ============================================================================
const StyleMember styles_members[] = {
    OX_PROP_RO(XLStyles, number_formats, numberFormats),
    OX_PROP_RO(XLStyles, fonts, fonts),
    OX_PROP_RO(XLStyles, fills, fills),
    OX_PROP_RO(XLStyles, borders, borders),
    OX_PROP_RO(XLStyles, cell_style_formats, cellStyleFormats),
    OX_PROP_RO(XLStyles, cell_formats, cellFormats),
    OX_PROP_RO(XLStyles, cell_styles, cellStyles),
    OX_PROP_RO(XLStyles, diff_cell_formats, diffCellFormats),
};

const StyleMember number_format_members[] = {
    OX_PROP(XLNumberFormat, number_format_id, numberFormatId, setNumberFormatId),
    OX_PROP(XLNumberFormat, format_code, formatCode, setFormatCode),
    OX_PROP_RO(XLNumberFormat, summary, summary),
};

const StyleMember font_members[] = {
    OX_PROP(XLFont, font_name, fontName, setFontName),
    OX_PROP(XLFont, font_charset, fontCharset, setFontCharset),
    OX_PROP(XLFont, font_family, fontFamily, setFontFamily),
    OX_PROP(XLFont, font_size, fontSize, setFontSize),
    OX_PROP(XLFont, font_color, fontColor, setFontColor),
    OX_PROP(XLFont, bold, bold, setBold),
    OX_PROP(XLFont, italic, italic, setItalic),
    OX_PROP(XLFont, strikethrough, strikethrough, setStrikethrough),
    { "underline", &style_get<XLFont, &XLFont::underline>, &font_set_underline, true },
    OX_PROP(XLFont, scheme, scheme, setScheme),
    OX_PROP(XLFont, vert_align, vertAlign, setVertAlign),
    OX_PROP(XLFont, outline, outline, setOutline),
    OX_PROP(XLFont, shadow, shadow, setShadow),
    OX_PROP(XLFont, condense, condense, setCondense),
    OX_PROP(XLFont, extend, extend, setExtend),
    OX_PROP_RO(XLFont, summary, summary),
};

const StyleMember fill_members[] = {
    { "fill_type", &style_get<XLFill, &XLFill::fillType>, &style_set<XLFill, &XLFill::setFillType>, false },
    OX_PROP(XLFill, gradient_type, gradientType, setGradientType),
    OX_PROP(XLFill, degree, degree, setDegree),
    OX_PROP(XLFill, left, left, setLeft),
    OX_PROP(XLFill, right, right, setRight),
    OX_PROP(XLFill, top, top, setTop),
    OX_PROP(XLFill, bottom, bottom, setBottom),
    OX_PROP_RO(XLFill, stops, stops),
    OX_PROP(XLFill, pattern_type, patternType, setPatternType),
    OX_PROP(XLFill, color, color, setColor),
    OX_PROP(XLFill, background_color, backgroundColor, setBackgroundColor),
    OX_PROP_RO(XLFill, summary, summary),
};

const StyleMember data_bar_color_members[] = {
    OX_PROP(XLDataBarColor, rgb, rgb, setRgb),
    OX_PROP(XLDataBarColor, tint, tint, setTint),
    OX_PROP(XLDataBarColor, automatic, automatic, setAutomatic),
    OX_PROP(XLDataBarColor, indexed, indexed, setIndexed),
    OX_PROP(XLDataBarColor, theme, theme, setTheme),
    OX_PROP_RO(XLDataBarColor, summary, summary),
};

const StyleMember gradient_stop_members[] = {
    OX_PROP_RO(XLGradientStop, color, color),
    OX_PROP(XLGradientStop, position, position, setPosition),
    OX_PROP_RO(XLGradientStop, summary, summary),
};

const StyleMember line_members[] = {
    OX_PROP_RO(XLLine, style, style),
    OX_PROP_RO(XLLine, color, color),
    OX_PROP_RO(XLLine, summary, summary),
};

const StyleMember border_members[] = {
    OX_PROP(XLBorder, diagonal_up, diagonalUp, setDiagonalUp),
    OX_PROP(XLBorder, diagonal_down, diagonalDown, setDiagonalDown),
    OX_PROP(XLBorder, outline, outline, setOutline),
    OX_PROP_RO(XLBorder, left, left),
    OX_PROP_RO(XLBorder, right, right),
    OX_PROP_RO(XLBorder, top, top),
    OX_PROP_RO(XLBorder, bottom, bottom),
    OX_PROP_RO(XLBorder, diagonal, diagonal),
    OX_PROP_RO(XLBorder, vertical, vertical),
    OX_PROP_RO(XLBorder, horizontal, horizontal),
    OX_PROP_RO(XLBorder, summary, summary),
};

// multi-argument border setters (style, color[, tint]) are methods only
const luaL_Reg border_extra_methods[] = {
    { "set_line", border_set_line },
    { "set_left", border_set_left },
    { "set_right", border_set_right },
    { "set_top", border_set_top },
    { "set_bottom", border_set_bottom },
    { "set_diagonal", border_set_diagonal },
    { "set_vertical", border_set_vertical },
    { "set_horizontal", border_set_horizontal },
    { nullptr, nullptr },
};

const StyleMember alignment_members[] = {
    OX_PROP(XLAlignment, horizontal, horizontal, setHorizontal),
    OX_PROP(XLAlignment, vertical, vertical, setVertical),
    OX_PROP(XLAlignment, text_rotation, textRotation, setTextRotation),
    OX_PROP(XLAlignment, wrap_text, wrapText, setWrapText),
    OX_PROP(XLAlignment, indent, indent, setIndent),
    OX_PROP(XLAlignment, relative_indent, relativeIndent, setRelativeIndent),
    OX_PROP(XLAlignment, justify_last_line, justifyLastLine, setJustifyLastLine),
    OX_PROP(XLAlignment, shrink_to_fit, shrinkToFit, setShrinkToFit),
    OX_PROP(XLAlignment, reading_order, readingOrder, setReadingOrder),
    OX_PROP_RO(XLAlignment, summary, summary),
};

const StyleMember cell_format_members[] = {
    OX_PROP(XLCellFormat, number_format_id, numberFormatId, setNumberFormatId),
    OX_PROP(XLCellFormat, font_index, fontIndex, setFontIndex),
    OX_PROP(XLCellFormat, fill_index, fillIndex, setFillIndex),
    OX_PROP(XLCellFormat, border_index, borderIndex, setBorderIndex),
    OX_PROP(XLCellFormat, xf_id, xfId, setXfId),
    OX_PROP(XLCellFormat, apply_number_format, applyNumberFormat, setApplyNumberFormat),
    OX_PROP(XLCellFormat, apply_font, applyFont, setApplyFont),
    OX_PROP(XLCellFormat, apply_fill, applyFill, setApplyFill),
    OX_PROP(XLCellFormat, apply_border, applyBorder, setApplyBorder),
    OX_PROP(XLCellFormat, apply_alignment, applyAlignment, setApplyAlignment),
    OX_PROP(XLCellFormat, apply_protection, applyProtection, setApplyProtection),
    OX_PROP(XLCellFormat, quote_prefix, quotePrefix, setQuotePrefix),
    OX_PROP(XLCellFormat, pivot_button, pivotButton, setPivotButton),
    OX_PROP(XLCellFormat, locked, locked, setLocked),
    OX_PROP(XLCellFormat, hidden, hidden, setHidden),
    { "alignment", &cell_format_alignment, nullptr, false },
    OX_PROP_RO(XLCellFormat, summary, summary),
};

const StyleMember cell_style_members[] = {
    OX_PROP_RO(XLCellStyle, empty, empty),
    OX_PROP(XLCellStyle, name, name, setName),
    OX_PROP(XLCellStyle, xf_id, xfId, setXfId),
    OX_PROP(XLCellStyle, builtin_id, builtinId, setBuiltinId),
    OX_PROP(XLCellStyle, outline_style, outlineStyle, setOutlineStyle),
    OX_PROP(XLCellStyle, hidden, hidden, setHidden),
    OX_PROP(XLCellStyle, custom_builtin, customBuiltin, setCustomBuiltin),
    OX_PROP_RO(XLCellStyle, summary, summary),
};

const StyleMember diff_cell_format_members[] = {
    OX_PROP_RO(XLDiffCellFormat, empty, empty),
    OX_PROP_RO(XLDiffCellFormat, font, font),
    OX_PROP_RO(XLDiffCellFormat, num_fmt, numFmt),
    OX_PROP_RO(XLDiffCellFormat, fill, fill),
    OX_PROP_RO(XLDiffCellFormat, alignment, alignment),
    OX_PROP_RO(XLDiffCellFormat, border, border),
    OX_PROP_RO(XLDiffCellFormat, summary, summary),
};

}  // namespace

// ============================================================================
// registration
// ============================================================================
namespace ox {

void bind_styles(lua_State *L) {
    register_style_type<XLStyles>(L, MT_STYLES, styles_members, std::size(styles_members));
    register_style_type<XLNumberFormat>(L, MT_NUMBER_FORMAT, number_format_members, std::size(number_format_members));
    register_style_type<XLFont>(L, MT_FONT, font_members, std::size(font_members));
    register_style_type<XLFill>(L, MT_FILL, fill_members, std::size(fill_members));
    register_style_type<XLDataBarColor>(L, MT_DATA_BAR_COLOR, data_bar_color_members,
                                        std::size(data_bar_color_members));
    register_style_type<XLGradientStop>(L, MT_GRADIENT_STOP, gradient_stop_members,
                                        std::size(gradient_stop_members));
    register_style_type<XLLine>(L, MT_LINE, line_members, std::size(line_members));
    register_style_type<XLBorder>(L, MT_BORDER, border_members, std::size(border_members), border_extra_methods);
    register_style_type<XLAlignment>(L, MT_ALIGNMENT, alignment_members, std::size(alignment_members));
    register_style_type<XLCellFormat>(L, MT_CELL_FORMAT, cell_format_members, std::size(cell_format_members));
    register_style_type<XLCellStyle>(L, MT_CELL_STYLE, cell_style_members, std::size(cell_style_members));
    register_style_type<XLDiffCellFormat>(L, MT_DIFF_CELL_FORMAT, diff_cell_format_members,
                                          std::size(diff_cell_format_members));

    // containers (methods + __len)
    register_style_type<XLNumberFormats>(L, MT_NUMBER_FORMATS, nullptr, 0, XLNumberFormats_methods);
    register_style_type<XLFonts>(L, MT_FONTS, nullptr, 0, XLFonts_methods);
    register_style_type<XLFills>(L, MT_FILLS, nullptr, 0, XLFills_methods);
    register_style_type<XLBorders>(L, MT_BORDERS, nullptr, 0, XLBorders_methods);
    register_style_type<XLCellFormats>(L, MT_CELL_FORMATS, nullptr, 0, XLCellFormats_methods);
    register_style_type<XLCellStyles>(L, MT_CELL_STYLES, nullptr, 0, XLCellStyles_methods);
    register_style_type<XLDiffCellFormats>(L, MT_DIFF_CELL_FORMATS, nullptr, 0, XLDiffCellFormats_methods);
    register_style_type<XLGradientStops>(L, MT_GRADIENT_STOPS, nullptr, 0, XLGradientStops_methods);

    // extra methods
    {
        const luaL_Reg number_formats_extra[] = {
            { "number_format_by_id", number_format_by_id },
            { "number_format_id_from_index", number_format_id_from_index },
            { nullptr, nullptr },
        };
        add_style_methods(L, MT_NUMBER_FORMATS, number_formats_extra);
    }
    {
        const luaL_Reg line_extra[] = { { "is_set", line_is_set }, { nullptr, nullptr } };
        add_style_methods(L, MT_LINE, line_extra);
    }
    {
        const luaL_Reg color_extra[] = { { "set", data_bar_color_set }, { nullptr, nullptr } };
        add_style_methods(L, MT_DATA_BAR_COLOR, color_extra);
    }

    // __len metamethods
    set_len_metamethod(L, MT_NUMBER_FORMATS, number_formats_len);
    set_len_metamethod(L, MT_FONTS, fonts_len);
    set_len_metamethod(L, MT_FILLS, fills_len);
    set_len_metamethod(L, MT_BORDERS, borders_len);
    set_len_metamethod(L, MT_CELL_FORMATS, cell_formats_len);
    set_len_metamethod(L, MT_CELL_STYLES, cell_styles_len);
    set_len_metamethod(L, MT_DIFF_CELL_FORMATS, diff_cell_formats_len);
    set_len_metamethod(L, MT_GRADIENT_STOPS, gradient_stops_len);

    // ---- enum constants ----
    auto enum_table = [&](const char *table_name, const EnumEntry *entries) {
        lua_newtable(L);
        for (const EnumEntry *e = entries; e->name != nullptr; ++e) {
            lua_pushinteger(L, lua_Integer(e->value));
            lua_setfield(L, -2, e->name);
            std::string upper(e->name);
            for (char &c : upper) c = (c >= 'a' && c <= 'z') ? char(c - 'a' + 'A') : c;
            lua_pushinteger(L, lua_Integer(e->value));
            lua_setfield(L, -2, upper.c_str());
        }
        lua_setfield(L, 1, table_name);
    };

    enum_table("underline", enum_traits_closed<XLUnderlineStyle>::table());
    enum_table("font_scheme", enum_traits_closed<XLFontSchemeStyle>::table());
    enum_table("vert_align", enum_traits_closed<XLVerticalAlignRunStyle>::table());
    enum_table("fill_type", enum_traits_closed<XLFillType>::table());
    enum_table("gradient_type", enum_traits_closed<XLGradientType>::table());
    enum_table("pattern", enum_traits_closed<XLPatternType>::table());
    enum_table("line_type", enum_traits_closed<XLLineType>::table());
    enum_table("line_style", enum_traits_closed<XLLineStyle>::table());
    enum_table("align", enum_traits_closed<XLAlignmentStyle>::table());
    enum_table("reading_order", enum_traits_closed<XLReadingOrder>::table());
}

}  // namespace ox
