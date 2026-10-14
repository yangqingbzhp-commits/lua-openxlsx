-- examples/demo.lua — 生成一个小的销售报表
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

-- 表头与数据
for r = 1, #data do
    ws:row(r).values = data[r]
end

-- 第 5 行是汇总：金额列求和，并给数据行补金额公式
for r = 2, 4 do
    ws:cell(r, 4).formula = string.format("B%d*C%d", r, r)
end
ws:cell(5, 4).formula = "SUM(D2:D4)"

-- 布局
ws:column("A"):set_width(12)
ws:column("B"):set_width(10)
ws:column("C"):set_width(10)
ws:column("D"):set_width(12)
ws:row(1):set_height(24)
ws:merge_cells("A6:D6")
ws:cell("A6").value = "生成时间：" .. os.date("%Y-%m-%d %H:%M")

-- 冻结表头（通过行选中/视图方式——上游尚未提供 freeze 便捷方法）
ws:set_selected(true)

doc:save()
doc:close()

print("demo written to " .. out)
