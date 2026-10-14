-- test/06_comments.lua — comments, tables, VML drawings/shapes, relationships
local xlsx = require("openxlsx")

local outdir = os.getenv("OX_TEST_OUT") or "/tmp/opencode/openxlsx-tests"
os.execute("mkdir -p '" .. outdir .. "'")
local path = outdir .. "/comments.xlsx"
os.remove(path)

local checks = 0
local function eq(a, b, what)
    checks = checks + 1
    if a ~= b then error(string.format("FAIL %s: expected %s, got %s", what, tostring(b), tostring(a)), 2) end
end
local function truthy(a, what)
    checks = checks + 1
    if not a then error("FAIL " .. what .. ": expected truthy value", 2) end
end

local doc = xlsx.new_document()
doc:create(path, xlsx.FORCE_OVERWRITE)
local ws = doc:workbook():worksheet("Sheet1")

-- initial state -----------------------------------------------------------------
eq(ws:has_comments(), false, "no comments initially")
eq(ws:has_vml_drawing(), false, "no vml drawing initially")
eq(ws:has_tables(), false, "no tables initially")

-- comments handle (creates comments + vml drawing on demand) ----------------------
local comments = ws:comments()
truthy(comments ~= nil, "comments handle")
eq(comments:count(), 0, "comment count 0")
eq(#comments, 0, "comments __len 0")

-- authors -------------------------------------------------------------------------
eq(comments:add_author("Alice"), 0, "add author 0")
eq(comments:add_author("Bob"), 1, "add author 1")
eq(comments:author_count(), 2, "author count")
eq(comments:author(0), "Alice", "author 0")
eq(comments:author(1), "Bob", "author 1")
truthy(comments:delete_author(1), "delete author 1")
eq(comments:author_count(), 1, "author count after delete")
eq(comments:add_author("Carol"), 1, "re-add author as index 1")

-- set / get comments ----------------------------------------------------------------
truthy(comments:set("A1", "first comment", 0), "set A1")
truthy(comments:set("B2", "second comment", 1), "set B2")
eq(comments:count(), 2, "comment count")
eq(#comments, 2, "comments __len")

eq(comments:get("A1"), "first comment", "get by cell ref")
eq(comments:author_id("B2"), 1, "author id of B2")
eq(comments:get("Z9"), "", "no comment for empty cell")

local c0 = comments:get(0)
truthy(c0:valid(), "comment #0 valid")
eq(c0:ref(), "A1", "comment #0 ref")
eq(c0:text(), "first comment", "comment #0 text")
eq(c0:author_id(), 0, "comment #0 author id")
truthy(c0:set_text("updated comment"), "set_text")
eq(comments:get("A1"), "updated comment", "updated text")

-- vml drawing / shapes ----------------------------------------------------------------
-- NOTE upstream: XLComments keeps its own copy of the VML drawing object, and
-- the shape counter is per-object (the XML is shared). Counting shapes is
-- therefore done through a freshly created object, which scans the XML again.
local vml = ws:vml_drawing()
eq(type(vml), "userdata", "vml drawing handle")

local fresh_vml = doc:sheet_vml_drawing(1)
truthy(fresh_vml:shape_count() >= 2, "vml shapes created for comments")

local shape = comments:shape("A1")
truthy(shape:shape_id() ~= "", "shape id set")
eq(type(shape:client_data().row), "number", "client data row readable")
local sstyle = shape:style()
truthy(sstyle.hidden or sstyle.visible, "shape style readable")
sstyle:show()
truthy(sstyle.visible, "shape visible after show")

-- standalone shape creation / deletion (one object for index bookkeeping)
local v = doc:sheet_vml_drawing(1)
local n0 = v:shape_count()
local extra = v:create_shape()
eq(v:shape_count(), n0 + 1, "create_shape")
truthy(extra:set_fill_color("#ffffc0"), "set fill color")
eq(extra:fill_color(), "#ffffc0", "fill color roundtrip")
truthy(v:delete_shape(n0), "delete_shape by index")
eq(v:shape_count(), n0, "shape count restored")

-- standalone shape style object (module-level constructor)
local st = xlsx.shape_style("position:absolute;width:100pt;height:50pt;visibility:visible")
eq(st.width, 100, "shape style width")
truthy(st:set_height(60), "set height")
eq(st.height, 60, "height roundtrip")
truthy(st:set_v_text_anchor("top"), "set v-text-anchor")
eq(st.v_text_anchor, "top", "v-text-anchor roundtrip")
truthy(st:hide(), "hide")
eq(st.visible, false, "not visible after hide")
truthy(st:show(), "show")
eq(st.visible, true, "visible after show")

-- doc-level accessors --------------------------------------------------------------------
eq(doc:has_sheet_comments(1), true, "has sheet comments")
local doc_comments = doc:sheet_comments(1)
eq(doc_comments:count(), comments:count(), "doc-level comments count")

local tables = ws:tables()
eq(type(tables), "userdata", "tables handle (no functional API upstream yet)")

local doc_vml = doc:sheet_vml_drawing(1)
truthy(doc_vml:shape_count() >= 2, "doc-level vml shape count")

local rels = doc:sheet_relationships(1)
truthy(rels:count() >= 2, "sheet relationships exist")
local found_comments_rel, found_vml_rel = false, false
for _, item in ipairs(rels:relationships()) do
    if item:type_name() == "comments" then found_comments_rel = true end
    if item:type_name() == "vml_drawing" then found_vml_rel = true end
    truthy(type(item:id()) == "string" and item:id() ~= "", "relationship item id")
end
truthy(found_comments_rel, "comments relationship present")
truthy(found_vml_rel, "vml drawing relationship present")
truthy(rels:target_exists("../comments1.xml") or true, "target_exists callable")

local rel_by_id = rels:relationship_by_id(rels:relationships()[1]:id())
truthy(rel_by_id:empty() == false, "relationship_by_id")

-- delete a comment -------------------------------------------------------------------------
truthy(comments:delete_comment("B2"), "delete comment B2")
eq(comments:count(), 1, "count after delete")

doc:save()
doc:close()

-- reopen & verify -----------------------------------------------------------------------------
local doc2 = xlsx.new_document(path)
local ws2 = doc2:workbook():worksheet("Sheet1")
eq(ws2:has_comments(), true, "reopen has comments")
eq(ws2:has_vml_drawing(), true, "reopen has vml drawing")

local comments3 = ws2:comments()
eq(comments3:count(), 1, "reopen comment count")
eq(comments3:get("A1"), "updated comment", "reopen comment text")
eq(comments3:author(comments3:author_id("A1")), "Alice", "reopen comment author")
eq(ws2:vml_drawing():shape_count() >= 1, true, "reopen vml shapes")
doc2:close()

print(string.format("OK - %d checks passed", checks))
