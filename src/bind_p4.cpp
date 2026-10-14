// =============================================================================
// bind_p4.cpp — comments, tables, VML drawings/shapes and relationships
//   XLComment(s), XLTables, XLVmlDrawing, XLShape, XLShapeStyle,
//   XLShapeClientData, XLRelationships, XLRelationshipItem
//   + document / worksheet accessors for those objects
// =============================================================================
#include "lua_bind.hpp"
#include "style_machinery.hpp"

#include <cstring>
#include <iterator>
#include <vector>

using namespace OpenXLSX;
using namespace ox;

// (style_mt / is_style_object specializations live in style_machinery.hpp)

// ---------------------------------------------------------------------------
// enum name tables
// ---------------------------------------------------------------------------
namespace ox {

#define OX_REL_TYPE(v) long(OpenXLSX::XLRelationshipType::v)
OX_ENUM_TABLE(XLShapeTextVAlign,
              { "center", long(XLShapeTextVAlign::Center) }, { "top", long(XLShapeTextVAlign::Top) },
              { "bottom", long(XLShapeTextVAlign::Bottom) }, { "invalid", long(XLShapeTextVAlign::Invalid) })
OX_ENUM_TABLE(XLShapeTextHAlign,
              { "left", long(XLShapeTextHAlign::Left) }, { "right", long(XLShapeTextHAlign::Right) },
              { "center", long(XLShapeTextHAlign::Center) }, { "invalid", long(XLShapeTextHAlign::Invalid) })
OX_ENUM_TABLE(XLRelationshipType,
              { "core_properties", OX_REL_TYPE(CoreProperties) },
              { "extended_properties", OX_REL_TYPE(ExtendedProperties) },
              { "custom_properties", OX_REL_TYPE(CustomProperties) },
              { "workbook", OX_REL_TYPE(Workbook) },
              { "worksheet", OX_REL_TYPE(Worksheet) },
              { "chartsheet", OX_REL_TYPE(Chartsheet) },
              { "dialogsheet", OX_REL_TYPE(Dialogsheet) },
              { "macrosheet", OX_REL_TYPE(Macrosheet) },
              { "calculation_chain", OX_REL_TYPE(CalculationChain) },
              { "external_link", OX_REL_TYPE(ExternalLink) },
              { "external_link_path", OX_REL_TYPE(ExternalLinkPath) },
              { "theme", OX_REL_TYPE(Theme) },
              { "styles", OX_REL_TYPE(Styles) },
              { "chart", OX_REL_TYPE(Chart) },
              { "chart_style", OX_REL_TYPE(ChartStyle) },
              { "chart_color_style", OX_REL_TYPE(ChartColorStyle) },
              { "image", OX_REL_TYPE(Image) },
              { "drawing", OX_REL_TYPE(Drawing) },
              { "vml_drawing", OX_REL_TYPE(VMLDrawing) },
              { "shared_strings", OX_REL_TYPE(SharedStrings) },
              { "printer_settings", OX_REL_TYPE(PrinterSettings) },
              { "vba_project", OX_REL_TYPE(VBAProject) },
              { "control_properties", OX_REL_TYPE(ControlProperties) },
              { "comments", OX_REL_TYPE(Comments) },
              { "table", OX_REL_TYPE(Table) },
              { "unknown", OX_REL_TYPE(Unknown) })
#undef OX_REL_TYPE

}  // namespace ox

namespace {

// ============================================================================
// XLComment
// ============================================================================
int comment_valid(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLComment>(L, 1, MT_COMMENT)->valid());
    return 1;
    OX_CATCH_ALL
}
int comment_ref(lua_State *L) {
    OX_TRY
    push_string(L, check<XLComment>(L, 1, MT_COMMENT)->ref());
    return 1;
    OX_CATCH_ALL
}
int comment_text(lua_State *L) {
    OX_TRY
    push_string(L, check<XLComment>(L, 1, MT_COMMENT)->text());
    return 1;
    OX_CATCH_ALL
}
int comment_author_id(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLComment>(L, 1, MT_COMMENT)->authorId()));
    return 1;
    OX_CATCH_ALL
}
int comment_set_text(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLComment>(L, 1, MT_COMMENT)->setText(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int comment_set_author_id(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLComment>(L, 1, MT_COMMENT)->setAuthorId(uint16_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg comment_methods[] = {
    { "valid", comment_valid },       { "ref", comment_ref },
    { "text", comment_text },         { "author_id", comment_author_id },
    { "set_text", comment_set_text }, { "set_author_id", comment_set_author_id },
    { nullptr, nullptr },
};

// ============================================================================
// XLComments
// ============================================================================
int comments_set_vml_drawing(lua_State *L) {
    OX_TRY
    auto *comments = check<XLComments>(L, 1, MT_COMMENTS);
    auto *vml = check<XLVmlDrawing>(L, 2, MT_VML_DRAWING);
    lua_pushboolean(L, comments->setVmlDrawing(*vml));
    return 1;
    OX_CATCH_ALL
}
int comments_author_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLComments>(L, 1, MT_COMMENTS)->authorCount()));
    return 1;
    OX_CATCH_ALL
}
int comments_author(lua_State *L) {
    OX_TRY
    push_string(L, check<XLComments>(L, 1, MT_COMMENTS)->author(uint16_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int comments_delete_author(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLComments>(L, 1, MT_COMMENTS)->deleteAuthor(uint16_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int comments_add_author(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLComments>(L, 1, MT_COMMENTS)->addAuthor(check_string(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int comments_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLComments>(L, 1, MT_COMMENTS)->count()));
    return 1;
    OX_CATCH_ALL
}
int comments_len(lua_State *L) { return comments_count(L); }
int comments_author_id(lua_State *L) {
    OX_TRY
    auto *comments = check<XLComments>(L, 1, MT_COMMENTS);
    const std::string cell_ref = check_cell_address(L, 2);
    lua_pushinteger(L, lua_Integer(comments->authorId(cell_ref)));
    return 1;
    OX_CATCH_ALL
}
int comments_delete_comment(lua_State *L) {
    OX_TRY
    auto *comments = check<XLComments>(L, 1, MT_COMMENTS);
    const std::string cell_ref = check_cell_address(L, 2);
    lua_pushboolean(L, comments->deleteComment(cell_ref));
    return 1;
    OX_CATCH_ALL
}
// get(index) -> XLComment ; get(cellRef) -> string
int comments_get(lua_State *L) {
    OX_TRY
    auto *comments = check<XLComments>(L, 1, MT_COMMENTS);
    if (lua_isinteger(L, 2)) {
        push_live<XLComment>(L, MT_COMMENT, token_of(L, 1), comments->get(size_t(lua_tointeger(L, 2))));
    } else {
        const std::string cell_ref = check_cell_address(L, 2);
        push_string(L, comments->get(cell_ref));
    }
    return 1;
    OX_CATCH_ALL
}
int comments_set(lua_State *L) {
    OX_TRY
    auto *comments = check<XLComments>(L, 1, MT_COMMENTS);
    const std::string cell_ref = check_cell_address(L, 2);
    const std::string text = check_string(L, 3);
    const uint16_t author = uint16_t(opt_integer(L, 4, 0));
    lua_pushboolean(L, comments->set(cell_ref, text, author));
    return 1;
    OX_CATCH_ALL
}
int comments_shape(lua_State *L) {
    OX_TRY
    auto *comments = check<XLComments>(L, 1, MT_COMMENTS);
    const std::string cell_ref = check_cell_address(L, 2);
    push_live<XLShape>(L, MT_SHAPE, token_of(L, 1), comments->shape(cell_ref));
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg comments_methods[] = {
    { "set_vml_drawing", comments_set_vml_drawing },
    { "author_count", comments_author_count },
    { "author", comments_author },
    { "delete_author", comments_delete_author },
    { "add_author", comments_add_author },
    { "count", comments_count },
    { "author_id", comments_author_id },
    { "delete_comment", comments_delete_comment },
    { "get", comments_get },
    { "set", comments_set },
    { "shape", comments_shape },
    { nullptr, nullptr },
};

// ============================================================================
// XLVmlDrawing
// ============================================================================
int vml_shape_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLVmlDrawing>(L, 1, MT_VML_DRAWING)->shapeCount()));
    return 1;
    OX_CATCH_ALL
}
int vml_shape(lua_State *L) {
    OX_TRY
    auto *vml = check<XLVmlDrawing>(L, 1, MT_VML_DRAWING);
    push_live<XLShape>(L, MT_SHAPE, token_of(L, 1), vml->shape(uint32_t(luaL_checkinteger(L, 2))));
    return 1;
    OX_CATCH_ALL
}
int vml_delete_shape(lua_State *L) {
    OX_TRY
    auto *vml = check<XLVmlDrawing>(L, 1, MT_VML_DRAWING);
    if (lua_isinteger(L, 2)) {
        lua_pushboolean(L, vml->deleteShape(uint32_t(lua_tointeger(L, 2))));
    } else {
        lua_pushboolean(L, vml->deleteShape(check_cell_address(L, 2)));
    }
    return 1;
    OX_CATCH_ALL
}
int vml_create_shape(lua_State *L) {
    OX_TRY
    auto *vml = check<XLVmlDrawing>(L, 1, MT_VML_DRAWING);
    if (auto *shape_template = maybe<XLShape>(L, 2, MT_SHAPE)) {
        push_live<XLShape>(L, MT_SHAPE, token_of(L, 1), vml->createShape(*shape_template));
    } else {
        push_live<XLShape>(L, MT_SHAPE, token_of(L, 1), vml->createShape());
    }
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg vml_methods[] = {
    { "shape_count", vml_shape_count },
    { "shape", vml_shape },
    { "delete_shape", vml_delete_shape },
    { "create_shape", vml_create_shape },
    { nullptr, nullptr },
};

// ============================================================================
// XLShape
// ============================================================================
int shape_shape_id(lua_State *L) {
    OX_TRY
    push_string(L, check<XLShape>(L, 1, MT_SHAPE)->shapeId());
    return 1;
    OX_CATCH_ALL
}
int shape_fill_color(lua_State *L) {
    OX_TRY
    push_string(L, check<XLShape>(L, 1, MT_SHAPE)->fillColor());
    return 1;
    OX_CATCH_ALL
}
int shape_stroked(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLShape>(L, 1, MT_SHAPE)->stroked());
    return 1;
    OX_CATCH_ALL
}
int shape_type(lua_State *L) {
    OX_TRY
    push_string(L, check<XLShape>(L, 1, MT_SHAPE)->type());
    return 1;
    OX_CATCH_ALL
}
int shape_allow_in_cell(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLShape>(L, 1, MT_SHAPE)->allowInCell());
    return 1;
    OX_CATCH_ALL
}
// XLShapeStyle is not copyable (unique_ptr member), so it is constructed in
// place - C++17 guaranteed copy elision makes this work for the prvalue
// returned by XLShape::style().
int shape_style(lua_State *L) {
    OX_TRY
    auto *shape = check<XLShape>(L, 1, MT_SHAPE);
    Box *b = box_new<XLShapeStyle>(L, MT_SHAPE_STYLE);
    b->obj = new XLShapeStyle(shape->style());
    b->token = token_of(L, 1);
    b->check_token = true;
    return 1;
    OX_CATCH_ALL
}

/** openxlsx.shape_style("position:absolute;width:100pt;...") */
int shape_style_new(lua_State *L) {
    OX_TRY
    push_new<XLShapeStyle>(L, MT_SHAPE_STYLE, check_string(L, 1));
    return 1;
    OX_CATCH_ALL
}
int shape_client_data(lua_State *L) {
    OX_TRY
    auto *shape = check<XLShape>(L, 1, MT_SHAPE);
    push_live<XLShapeClientData>(L, MT_SHAPE_CLIENT_DATA, token_of(L, 1), shape->clientData());
    return 1;
    OX_CATCH_ALL
}
int shape_set_fill_color(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLShape>(L, 1, MT_SHAPE)->setFillColor(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int shape_set_stroked(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLShape>(L, 1, MT_SHAPE)->setStroked(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}
int shape_set_type(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLShape>(L, 1, MT_SHAPE)->setType(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int shape_set_allow_in_cell(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLShape>(L, 1, MT_SHAPE)->setAllowInCell(opt_bool(L, 2, true)));
    return 1;
    OX_CATCH_ALL
}
// setStyle(std::string) or setStyle(XLShapeStyle)
int shape_set_style(lua_State *L) {
    OX_TRY
    auto *shape = check<XLShape>(L, 1, MT_SHAPE);
    if (auto *style = maybe<XLShapeStyle>(L, 2, MT_SHAPE_STYLE)) {
        lua_pushboolean(L, shape->setStyle(*style));
    } else {
        lua_pushboolean(L, shape->setStyle(check_string(L, 2)));
    }
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg shape_methods[] = {
    { "shape_id", shape_shape_id },
    { "fill_color", shape_fill_color },
    { "stroked", shape_stroked },
    { "type", shape_type },
    { "allow_in_cell", shape_allow_in_cell },
    { "style", shape_style },
    { "client_data", shape_client_data },
    { "set_fill_color", shape_set_fill_color },
    { "set_stroked", shape_set_stroked },
    { "set_type", shape_set_type },
    { "set_allow_in_cell", shape_set_allow_in_cell },
    { "set_style", shape_set_style },
    { nullptr, nullptr },
};

// ============================================================================
// XLRelationships / XLRelationshipItem
// ============================================================================
int relationships_relationship_by_id(lua_State *L) {
    OX_TRY
    auto *rels = check<XLRelationships>(L, 1, MT_RELATIONSHIPS);
    push_live<XLRelationshipItem>(L, MT_RELATIONSHIP_ITEM, token_of(L, 1),
                                  rels->relationshipById(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int relationships_relationship_by_target(lua_State *L) {
    OX_TRY
    auto *rels = check<XLRelationships>(L, 1, MT_RELATIONSHIPS);
    push_live<XLRelationshipItem>(L, MT_RELATIONSHIP_ITEM, token_of(L, 1),
                                  rels->relationshipByTarget(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int relationships_relationships(lua_State *L) {
    OX_TRY
    auto *rels = check<XLRelationships>(L, 1, MT_RELATIONSHIPS);
    const std::vector<XLRelationshipItem> items = rels->relationships();
    lua_createtable(L, int(items.size()), 0);
    int i = 1;
    for (const auto &item : items) {
        push_live<XLRelationshipItem>(L, MT_RELATIONSHIP_ITEM, token_of(L, 1), item);
        lua_seti(L, -2, i++);
    }
    return 1;
    OX_CATCH_ALL
}
int relationships_count(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, lua_Integer(check<XLRelationships>(L, 1, MT_RELATIONSHIPS)->relationships().size()));
    return 1;
    OX_CATCH_ALL
}
int relationships_len(lua_State *L) { return relationships_count(L); }
int relationships_delete_relationship(lua_State *L) {
    OX_TRY
    auto *rels = check<XLRelationships>(L, 1, MT_RELATIONSHIPS);
    if (auto *item = maybe<XLRelationshipItem>(L, 2, MT_RELATIONSHIP_ITEM)) {
        rels->deleteRelationship(*item);
    } else {
        rels->deleteRelationship(check_string(L, 2));
    }
    return 0;
    OX_CATCH_ALL
}
int relationships_add_relationship(lua_State *L) {
    OX_TRY
    auto *rels = check<XLRelationships>(L, 1, MT_RELATIONSHIPS);
    const auto type = check_enum_arg<XLRelationshipType>(L, 2);
    push_live<XLRelationshipItem>(L, MT_RELATIONSHIP_ITEM, token_of(L, 1),
                                  rels->addRelationship(type, check_string(L, 3)));
    return 1;
    OX_CATCH_ALL
}
int relationships_target_exists(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLRelationships>(L, 1, MT_RELATIONSHIPS)->targetExists(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}
int relationships_id_exists(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLRelationships>(L, 1, MT_RELATIONSHIPS)->idExists(check_string(L, 2)));
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg relationships_methods[] = {
    { "relationship_by_id", relationships_relationship_by_id },
    { "relationship_by_target", relationships_relationship_by_target },
    { "relationships", relationships_relationships },
    { "count", relationships_count },
    { "delete_relationship", relationships_delete_relationship },
    { "add_relationship", relationships_add_relationship },
    { "target_exists", relationships_target_exists },
    { "id_exists", relationships_id_exists },
    { nullptr, nullptr },
};

int relationship_item_cell_type_name(lua_State *L) {
    OX_TRY
    const int value = int(check<XLRelationshipItem>(L, 1, MT_RELATIONSHIP_ITEM)->type());
    for (const EnumEntry *e = enum_traits_closed<XLRelationshipType>::table(); e->name != nullptr; ++e) {
        if (e->value == value) {
            lua_pushstring(L, e->name);
            return 1;
        }
    }
    lua_pushstring(L, "unknown");
    return 1;
    OX_CATCH_ALL
}
int relationship_item_type(lua_State *L) {
    OX_TRY
    lua_pushinteger(L, int(check<XLRelationshipItem>(L, 1, MT_RELATIONSHIP_ITEM)->type()));
    return 1;
    OX_CATCH_ALL
}
int relationship_item_target(lua_State *L) {
    OX_TRY
    push_string(L, check<XLRelationshipItem>(L, 1, MT_RELATIONSHIP_ITEM)->target());
    return 1;
    OX_CATCH_ALL
}
int relationship_item_id(lua_State *L) {
    OX_TRY
    push_string(L, check<XLRelationshipItem>(L, 1, MT_RELATIONSHIP_ITEM)->id());
    return 1;
    OX_CATCH_ALL
}
int relationship_item_empty(lua_State *L) {
    OX_TRY
    lua_pushboolean(L, check<XLRelationshipItem>(L, 1, MT_RELATIONSHIP_ITEM)->empty());
    return 1;
    OX_CATCH_ALL
}

const luaL_Reg relationship_item_methods[] = {
    { "type", relationship_item_type },
    { "type_name", relationship_item_cell_type_name },
    { "target", relationship_item_target },
    { "id", relationship_item_id },
    { "empty", relationship_item_empty },
    { nullptr, nullptr },
};

// ============================================================================
// document / worksheet accessors
// ============================================================================
int doc_sheet_comments(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    const uint16_t sheet_no = uint16_t(luaL_checkinteger(L, 2));
    push_live<XLComments>(L, MT_COMMENTS, token_of(L, 1), doc->sheetComments(sheet_no));
    return 1;
    OX_CATCH_ALL
}
int doc_sheet_tables(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    const uint16_t sheet_no = uint16_t(luaL_checkinteger(L, 2));
    push_live<XLTables>(L, MT_TABLES, token_of(L, 1), doc->sheetTables(sheet_no));
    return 1;
    OX_CATCH_ALL
}
int doc_sheet_vml_drawing(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    const uint16_t sheet_no = uint16_t(luaL_checkinteger(L, 2));
    push_live<XLVmlDrawing>(L, MT_VML_DRAWING, token_of(L, 1), doc->sheetVmlDrawing(sheet_no));
    return 1;
    OX_CATCH_ALL
}
int doc_sheet_relationships(lua_State *L) {
    OX_TRY
    auto *doc = check<XLDocument>(L, 1, MT_DOCUMENT);
    const uint16_t sheet_no = uint16_t(luaL_checkinteger(L, 2));
    push_live<XLRelationships>(L, MT_RELATIONSHIPS, token_of(L, 1), doc->sheetRelationships(sheet_no));
    return 1;
    OX_CATCH_ALL
}

int ws_comments(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    // live reference: the worksheet's comments object keeps the VML drawing link
    push_ref<XLComments>(L, MT_COMMENTS, token_of(L, 1), &ws->comments(), 1);
    return 1;
    OX_CATCH_ALL
}
int ws_tables(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    push_ref<XLTables>(L, MT_TABLES, token_of(L, 1), &ws->tables(), 1);
    return 1;
    OX_CATCH_ALL
}
int ws_vml_drawing(lua_State *L) {
    OX_TRY
    auto *ws = check<XLWorksheet>(L, 1, MT_WORKSHEET);
    // live reference: the VML drawing object manages shape IDs and must not be copied
    push_ref<XLVmlDrawing>(L, MT_VML_DRAWING, token_of(L, 1), &ws->vmlDrawing(), 1);
    return 1;
    OX_CATCH_ALL
}

// ============================================================================
// member tables for the drawing classes
// ============================================================================
const StyleMember shape_style_members[] = {
    OX_PROP(XLShapeStyle, position, position, setPosition),
    OX_PROP(XLShapeStyle, margin_left, marginLeft, setMarginLeft),
    OX_PROP(XLShapeStyle, margin_top, marginTop, setMarginTop),
    OX_PROP(XLShapeStyle, width, width, setWidth),
    OX_PROP(XLShapeStyle, height, height, setHeight),
    OX_PROP(XLShapeStyle, mso_wrap_style, msoWrapStyle, setMsoWrapStyle),
    OX_PROP(XLShapeStyle, v_text_anchor, vTextAnchor, setVTextAnchor),
    OX_PROP_RO(XLShapeStyle, hidden, hidden),
    OX_PROP_RO(XLShapeStyle, visible, visible),
    OX_PROP(XLShapeStyle, raw, raw, setRaw),
};

const StyleMember shape_client_data_members[] = {
    OX_PROP(XLShapeClientData, object_type, objectType, setObjectType),
    OX_PROP(XLShapeClientData, move_with_cells, moveWithCells, setMoveWithCells),
    OX_PROP(XLShapeClientData, size_with_cells, sizeWithCells, setSizeWithCells),
    OX_PROP(XLShapeClientData, anchor, anchor, setAnchor),
    OX_PROP(XLShapeClientData, auto_fill, autoFill, setAutoFill),
    OX_PROP(XLShapeClientData, text_v_align, textVAlign, setTextVAlign),
    OX_PROP(XLShapeClientData, text_h_align, textHAlign, setTextHAlign),
    OX_PROP(XLShapeClientData, row, row, setRow),
    OX_PROP(XLShapeClientData, column, column, setColumn),
};

}  // namespace

// ============================================================================
// registration
// ============================================================================
namespace ox {

void bind_p4(lua_State *L) {
    const luaL_Reg no_methods[] = { { nullptr, nullptr } };

    register_type<XLComment>(L, MT_COMMENT, comment_methods);
    register_type<XLComments>(L, MT_COMMENTS, comments_methods);
    register_type<XLTables>(L, MT_TABLES, no_methods);
    register_type<XLVmlDrawing>(L, MT_VML_DRAWING, vml_methods);
    register_type<XLShape>(L, MT_SHAPE, shape_methods);
    register_type<XLRelationships>(L, MT_RELATIONSHIPS, relationships_methods);
    register_type<XLRelationshipItem>(L, MT_RELATIONSHIP_ITEM, relationship_item_methods);

    register_style_type<XLShapeStyle>(L, MT_SHAPE_STYLE, shape_style_members, std::size(shape_style_members));
    register_style_type<XLShapeClientData>(L, MT_SHAPE_CLIENT_DATA, shape_client_data_members,
                                           std::size(shape_client_data_members));

    set_len_metamethod(L, MT_COMMENTS, comments_len);
    set_len_metamethod(L, MT_RELATIONSHIPS, relationships_len);

    // extra methods for shape style / client data
    {
        const luaL_Reg style_extra[] = {
            { "hide", &style_set<XLShapeStyle, &XLShapeStyle::hide> },
            { "show", &style_set<XLShapeStyle, &XLShapeStyle::show> },
            { nullptr, nullptr },
        };
        add_style_methods(L, MT_SHAPE_STYLE, style_extra);
    }

    // ---- patch the document metatable ----
    {
        const luaL_Reg extra[] = {
            { "sheet_comments", doc_sheet_comments },
            { "sheet_tables", doc_sheet_tables },
            { "sheet_vml_drawing", doc_sheet_vml_drawing },
            { "sheet_relationships", doc_sheet_relationships },
            { nullptr, nullptr },
        };
        luaL_getmetatable(L, MT_DOCUMENT);
        lua_getfield(L, -1, "__index");
        luaL_setfuncs(L, extra, 0);
        lua_pop(L, 2);
    }
    // ---- patch the worksheet metatable ----
    {
        const luaL_Reg extra[] = {
            { "comments", ws_comments },
            { "tables", ws_tables },
            { "vml_drawing", ws_vml_drawing },
            { nullptr, nullptr },
        };
        luaL_getmetatable(L, MT_WORKSHEET);
        lua_getfield(L, -1, "__index");
        luaL_setfuncs(L, extra, 0);
        lua_pop(L, 2);
    }

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
    enum_table("shape_v_align", enum_traits_closed<XLShapeTextVAlign>::table());
    enum_table("shape_h_align", enum_traits_closed<XLShapeTextHAlign>::table());
    enum_table("relationship_type", enum_traits_closed<XLRelationshipType>::table());

    lua_pushcfunction(L, shape_style_new);
    lua_setfield(L, 1, "shape_style");
}

}  // namespace ox
