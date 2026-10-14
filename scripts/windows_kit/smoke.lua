-- smoke.lua — 最小自检（Windows 测试包）
-- 验证 openxlsx 模块能否加载并完成"写→存→读"全流程。
local ok_count, fail_count = 0, 0
local function check(cond, what)
    if cond then ok_count = ok_count + 1
    else fail_count = fail_count + 1; print("  FAIL: " .. what) end
end

print("== lua-openxlsx Windows smoke test ==")
print("Lua:", _VERSION, "| platform:", package.config:sub(1, 1) == "\\" and "Windows" or "Unix")

local ok, xlsx = pcall(require, "openxlsx")
if not ok then
    print("FATAL: cannot load openxlsx module:")
    print("  " .. tostring(xlsx))
    print("请确认 openxlsx.dll / lua55.dll 与本脚本在同一目录，且 LUA_CPATH 已设置。")
    os.exit(1)
end
check(true, "module loads")
print("module:", xlsx.version(), "| zip:", xlsx.zip_library_name())

local out = "smoke-out.xlsx"
os.remove(out)

-- 写：值 / 日期 / 公式 / 样式 / 合并 / 批注
local doc = xlsx.new_document()
check(pcall(function() doc:create(out, xlsx.FORCE_OVERWRITE) end), "create workbook")

local ws = doc:workbook():worksheet("Sheet1")
ws:cell("A1").value = "Hello 世界"                 -- UTF-8
ws:cell("A2").value = 42
ws:cell("A3").value = 3.5
ws:cell("A4").value = true
ws:cell("B1").value = xlsx.datetime{ year = 2026, month = 10, day = 14 }
ws:cell("B2").formula = "SUM(A2:A3)"
ws:row(5).values = { "批量", 1, 2.5, false }
ws:column("A"):set_width(18)
ws:merge_cells("C1:D1")

local styles = doc:styles()
local f = styles.fonts:create()
styles.fonts:get(f).bold = true
local fmt = styles.cell_formats:create()
local cf = styles.cell_formats:get(fmt)
cf.font_index = f
cf.apply_font = true
ws:cell("A1"):set_format(fmt)

local comments = ws:comments()
comments:add_author("Smoke")
comments:set("A1", "comment works", 0)

check(pcall(function() doc:save() end), "save workbook")
doc:close()

-- 读回
local doc2 = xlsx.new_document(out)
local ws2 = doc2:workbook():worksheet("Sheet1")
check(ws2:cell("A1").value == "Hello 世界", "UTF-8 roundtrip")
check(ws2:cell("A2").value == 42, "integer roundtrip")
check(ws2:cell("A4").value == true, "boolean roundtrip")
check(ws2:cell("B2").formula == "SUM(A2:A3)", "formula roundtrip")
check(ws2:cell("A1"):get_value() == "Hello 世界", "styled cell value")
check(ws2:merges:count() == 1, "merge roundtrip")
check(ws2:comments():get("A1") == "comment works", "comment roundtrip")
check(ws2:cell("A3").value == 3.5, "float roundtrip")
doc2:close()

print(string.format("checks: %d ok, %d failed", ok_count, fail_count))
if fail_count == 0 then
    print("RESULT: PASS")
    print("生成文件: " .. out .. "（可用 Excel 打开查看）")
    os.exit(0)
else
    print("RESULT: FAIL")
    os.exit(1)
end
