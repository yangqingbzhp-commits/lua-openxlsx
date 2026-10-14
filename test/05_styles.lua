-- test/05_styles.lua — XLStyles: fonts, fills, borders, number formats, alignment, dxfs
local xlsx = require("openxlsx")

local outdir = os.getenv("OX_TEST_OUT") or "/tmp/opencode/openxlsx-tests"
os.execute("mkdir -p '" .. outdir .. "'")
local path = outdir .. "/styles.xlsx"

local checks = 0
local function eq(a, b, what)
    checks = checks + 1
    if a ~= b then error(string.format("FAIL %s: expected %s, got %s", what, tostring(b), tostring(a)), 2) end
end
local function truthy(a, what)
    checks = checks + 1
    if not a then error("FAIL " .. what .. ": expected truthy value", 2) end
end

-- enum constants ---------------------------------------------------------------
eq(xlsx.underline.SINGLE, 1, "underline.SINGLE")
eq(xlsx.underline.DOUBLE, 2, "underline.DOUBLE")
eq(xlsx.pattern.SOLID, 1, "pattern.SOLID")
eq(xlsx.pattern.GRAY125, 17, "pattern.GRAY125")
eq(xlsx.line_style.THIN, 1, "line_style.THIN")
eq(xlsx.line_style.MEDIUM, 2, "line_style.MEDIUM")
eq(xlsx.align.CENTER, 3, "align.CENTER")
eq(xlsx.align.CENTER_CONTINUOUS, 8, "align.CENTER_CONTINUOUS")
eq(xlsx.fill_type.PATTERN, 1, "fill_type.PATTERN")
eq(xlsx.gradient_type.LINEAR, 0, "gradient_type.LINEAR")
eq(xlsx.font_scheme.NONE, 0, "font_scheme.NONE")
eq(xlsx.vert_align.SUPERSCRIPT, 2, "vert_align.SUPERSCRIPT")
eq(xlsx.reading_order.RIGHT_TO_LEFT, 2, "reading_order.RIGHT_TO_LEFT")

-- build a workbook with styles ---------------------------------------------------
local doc = xlsx.new_document()
doc:create(path, xlsx.FORCE_OVERWRITE)
local ws = doc:workbook():worksheet("Sheet1")

local styles = doc:styles()
local fonts = styles.fonts
local fills = styles.fills
local borders = styles.borders
local cell_formats = styles.cell_formats
local number_formats = styles.number_formats
local dxfs = styles.diff_cell_formats

truthy(fonts:count() >= 1, "default fonts present")
truthy(fills:count() >= 1, "default fills present")

-- 1. font ----------------------------------------------------------------------
local font_idx = fonts:create()
local font = fonts:get(font_idx)
truthy(font:set_font_name("Arial"), "set_font_name")
font.font_size = 14
font.bold = true
font.italic = true
font.underline = "single"
font.font_color = xlsx.color("FF0000")

eq(fonts:get(font_idx).font_name, "Arial", "font_name roundtrip")
eq(fonts:get(font_idx).bold, true, "bold roundtrip")
eq(fonts:get(font_idx).bold, true, "bold via method")
eq(fonts:get(font_idx).underline, xlsx.underline.SINGLE, "underline roundtrip")
eq(fonts:get(font_idx).font_color:hex(), "ffff0000", "font color roundtrip")

fonts:get(font_idx).font_size = 16 -- write through a fresh handle
eq(fonts:get(font_idx).font_size, 16, "font_size update")

-- 2. cell format with font --------------------------------------------------------
local fmt_font = cell_formats:create()
local cf = cell_formats:get(fmt_font)
cf.font_index = font_idx
cf.apply_font = true
eq(cf.font_index, font_idx, "font_index roundtrip")

ws:cell("A1").value = "styled A1"
ws:cell("A1"):set_format(fmt_font)
eq(ws:cell("A1"):format(), fmt_font, "cell format index")

-- 3. fill --------------------------------------------------------------------------
local fill_idx = fills:create()
local fill = fills:get(fill_idx)
fill.pattern_type = "solid"
fill.color = xlsx.color(255, 255, 0)
fill.background_color = xlsx.color(255, 255, 0)
eq(fills:get(fill_idx).pattern_type, xlsx.pattern.SOLID, "pattern roundtrip")

local fmt_fill = cell_formats:create()
cell_formats:get(fmt_fill):set_fill_index(fill_idx)
cell_formats:get(fmt_fill).apply_fill = true
ws:cell("A2").value = "yellow A2"
ws:cell("A2"):set_format(fmt_fill)

-- 4. border ---------------------------------------------------------------------------
local border_idx = borders:create()
local border = borders:get(border_idx)
truthy(border:set_left("thin", xlsx.color("0000FF")), "set_left")
truthy(border:set_bottom("medium", xlsx.color("00FF00")), "set_bottom")
truthy(border:set_top(xlsx.line_style.THIN, "AAAAAA"), "set_top (hex string color)")
truthy(border.left:is_set(), "left line is set")
eq(border.left.style, xlsx.line_style.THIN, "left style")
eq(border.bottom.style, xlsx.line_style.MEDIUM, "bottom style")
eq(border.right:is_set(), false, "right line not set")

local fmt_border = cell_formats:create()
local cfb = cell_formats:get(fmt_border)
cfb.border_index = border_idx
cfb.apply_border = true

-- 5. alignment ---------------------------------------------------------------------------
local align = cell_formats:get(fmt_border).alignment
align.wrap_text = true
align.horizontal = "center"
align.vertical = "center"
align.text_rotation = 0
cell_formats:get(fmt_border).apply_alignment = true
eq(cell_formats:get(fmt_border).alignment.wrap_text, true, "alignment roundtrip")
eq(cell_formats:get(fmt_border).alignment.horizontal, xlsx.align.CENTER, "alignment horizontal")

ws:cell("A3").value = "bordered A3"
ws:cell("A3"):set_format(fmt_border)

-- 6. number format --------------------------------------------------------------------------
local nf_idx = number_formats:create()
number_formats:get(nf_idx).format_code = "0.00%"
eq(number_formats:get(nf_idx).format_code, "0.00%", "format code roundtrip")

local fmt_num = cell_formats:create()
local cfn = cell_formats:get(fmt_num)
cfn.number_format_id = number_formats:number_format_id_from_index(nf_idx)
cfn.apply_number_format = true
ws:cell("B1").value = 0.42
ws:cell("B1"):set_format(fmt_num)

-- 7. column format ---------------------------------------------------------------------------
truthy(ws:set_column_format("C", fmt_font), "set_column_format")
eq(ws:get_column_format("C"), fmt_font, "get_column_format")
truthy(ws:set_row_format(5, fmt_num), "set_row_format")
eq(ws:get_row_format(5), fmt_num, "get_row_format")

-- 8. dxf (conditional formatting styles) ------------------------------------------------------
local dxf_idx = dxfs:create()
truthy(dxfs:count() >= 1, "dxfs count")
local dxf = dxfs:get(dxf_idx)
eq(dxf.empty, false, "dxf not empty")
dxf.font:set_font_color(xlsx.color("FF0000"))
dxf.font.bold = true
dxf.fill:set_pattern_type("solid")
dxf.fill.color = xlsx.color("FFCCCC")
eq(dxfs:get(dxf_idx).font.bold, true, "dxf font bold")

-- attach the dxf to a conditional format rule
local cfs = ws:conditional_formats()
local cf_idx = cfs:create()
local conffmt = cfs:get(cf_idx)
conffmt:set_sqref("D1:D10")
local rules = conffmt:cf_rules()
local rule_idx = rules:create()
local rule = rules:get(rule_idx)
rule:set_type("cell_is")
rule:set_operator("greater_than")
rule:set_formula("5")
rule:set_dxf_id(dxf_idx)
eq(rules:get(rule_idx):dxf_id(), dxf_idx, "rule dxf id")

-- containers __len and indexing helpers ---------------------------------------------------------
eq(#fonts, fonts:count(), "fonts __len")
eq(#cell_formats, cell_formats:count(), "cell_formats __len")
truthy(number_formats:number_format_by_id(0) ~= nil, "number_format_by_id")

doc:save()
doc:close()

-- reopen and verify persistence -------------------------------------------------------------------
local doc2 = xlsx.new_document(path)
local styles2 = doc2:styles()
local ws2 = doc2:workbook():worksheet("Sheet1")

local fmt_a1 = ws2:cell("A1"):format()
local font_a1_idx = styles2.cell_formats:get(fmt_a1).font_index
local font_a1 = styles2.fonts:get(font_a1_idx)
eq(font_a1.font_name, "Arial", "reopen font name")
eq(font_a1.bold, true, "reopen font bold")
eq(font_a1.font_size, 16, "reopen font size")
eq(font_a1.font_color:hex(), "ffff0000", "reopen font color")
eq(font_a1.underline, xlsx.underline.SINGLE, "reopen underline")

local fmt_a2 = ws2:cell("A2"):format()
local fill_a2_idx = styles2.cell_formats:get(fmt_a2).fill_index
local fill_a2 = styles2.fills:get(fill_a2_idx)
eq(fill_a2.pattern_type, xlsx.pattern.SOLID, "reopen pattern")
eq(fill_a2.color:hex(), "ffffff00", "reopen fill color")

local fmt_a3 = ws2:cell("A3"):format()
local border_a3_idx = styles2.cell_formats:get(fmt_a3).border_index
local border_a3 = styles2.borders:get(border_a3_idx)
eq(border_a3.left.style, xlsx.line_style.THIN, "reopen left style")
eq(border_a3.bottom.style, xlsx.line_style.MEDIUM, "reopen bottom style")
eq(styles2.cell_formats:get(fmt_a3).alignment.horizontal, xlsx.align.CENTER, "reopen alignment")

local fmt_b1 = ws2:cell("B1"):format()
local nf_id = styles2.cell_formats:get(fmt_b1).number_format_id
eq(styles2.number_formats:number_format_by_id(nf_id).format_code, "0.00%", "reopen number format")

eq(styles2.diff_cell_formats:count() >= 1, true, "reopen dxfs")
doc2:close()

print(string.format("OK - %d checks passed", checks))
