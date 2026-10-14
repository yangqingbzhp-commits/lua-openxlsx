-- examples/demo.lua — 生成一个带样式的销售报表
-- 运行: LUA_CPATH="<build>/?.so;;" lua examples/demo.lua [输出路径]
local xlsx = require("openxlsx")

local out = arg[1] or "/tmp/opencode/openxlsx-tests/demo.xlsx"
os.execute("mkdir -p '" .. out:match("^(.*)/[^/]*$") .. "'")

local data = {
    { "产品",   "单价", "数量", "金额" },
    { "苹果",   3.5,    120,    nil },
    { "香蕉",   2.0,    80,     nil },
    { "橙子",   4.25,   60,     nil },
    { "总计",   nil,    nil,    nil },
}

local doc = xlsx.new_document()
doc:create(out, xlsx.FORCE_OVERWRITE)
doc:set_property(xlsx.property.TITLE, "水果销售月报")
doc:set_property(xlsx.property.CREATOR, "lua-openxlsx demo")

local ws = doc:workbook():worksheet("Sheet1")
ws:set_name("销售")

-- ===== 样式准备 =====
local styles = doc:styles()

-- 表头样式：白字 + 蓝底 + 居中
local font_header_idx = styles.fonts:create()
local font_header = styles.fonts:get(font_header_idx)
font_header.bold = true
font_header.font_size = 12
font_header.font_color = xlsx.color("FFFFFF")

local fill_header_idx = styles.fills:create()
local fill_header = styles.fills:get(fill_header_idx)
fill_header.pattern_type = "solid"
fill_header.color = xlsx.color("4472C4")

local fmt_header_idx = styles.cell_formats:create()
local fmt_header = styles.cell_formats:get(fmt_header_idx)
fmt_header.font_index = font_header_idx
fmt_header.fill_index = fill_header_idx
fmt_header.apply_font = true
fmt_header.apply_fill = true
fmt_header.alignment.horizontal = "center"
fmt_header.alignment.vertical = "center"
fmt_header.apply_alignment = true

-- 金额列样式：两位小数
local nf_money_idx = styles.number_formats:create()
styles.number_formats:get(nf_money_idx).format_code = "#,##0.00"

local fmt_money_idx = styles.cell_formats:create()
local fmt_money = styles.cell_formats:get(fmt_money_idx)
fmt_money.number_format_id = styles.number_formats:number_format_id_from_index(nf_money_idx)
fmt_money.apply_number_format = true

-- 汇总行样式：加粗 + 上边框
local font_total_idx = styles.fonts:create()
styles.fonts:get(font_total_idx).bold = true

local border_total_idx = styles.borders:create()
local border_total = styles.borders:get(border_total_idx)
border_total:set_top("thin", xlsx.color("808080"))

local fmt_total_idx = styles.cell_formats:create()
local fmt_total = styles.cell_formats:get(fmt_total_idx)
fmt_total.font_index = font_total_idx
fmt_total.border_index = border_total_idx
fmt_total.apply_font = true
fmt_total.apply_border = true

-- ===== 数据 =====
for r = 1, #data do
    ws:row(r).values = data[r]
end

for r = 2, 4 do
    ws:cell(r, 4).formula = string.format("B%d*C%d", r, r)
    ws:cell(r, 4):set_format(fmt_money_idx)
end
ws:cell(5, 4).formula = "SUM(D2:D4)"

-- 套用表头/汇总样式
for c = 1, 4 do
    ws:cell(1, c):set_format(fmt_header_idx)
    ws:cell(5, c):set_format(fmt_total_idx)
end

-- 列宽 / 行高 / 合并
ws:column("A"):set_width(12)
ws:column("B"):set_width(10)
ws:column("C"):set_width(10)
ws:column("D"):set_width(14)
ws:row(1):set_height(24)
ws:merge_cells("A6:D6")
ws:cell("A6").value = "生成时间：" .. os.date("%Y-%m-%d %H:%M")
ws:cell("A6"):set_format(fmt_total_idx)

doc:save()
doc:close()

print("demo written to " .. out)
