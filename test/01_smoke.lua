-- test/01_smoke.lua — basic smoke test for the openxlsx Lua module
-- Run: LUA_CPATH="<build>/?.so;;" lua test/01_smoke.lua
local xlsx = require("openxlsx")

local outdir = os.getenv("OX_TEST_OUT") or "/tmp/opencode/openxlsx-tests"
os.execute("mkdir -p '" .. outdir .. "'")

local path = outdir .. "/smoke.xlsx"
os.remove(path)

print("module  :", xlsx.version())
print("zip lib :", xlsx.zip_library_name(), xlsx.zip_library_version())

-- helpers -------------------------------------------------------------------
local checks = 0
local function eq(a, b, what)
    checks = checks + 1
    if a ~= b then
        error(string.format("FAIL %s: expected %s, got %s", what, tostring(b), tostring(a)), 2)
    end
end

-- create workbook ------------------------------------------------------------
local doc = xlsx.new_document()
eq(doc:is_open(), false, "new document is_open")
doc:create(path, xlsx.FORCE_OVERWRITE)
eq(doc:is_open(), true, "created document is_open")

local wb = doc:workbook()
eq(wb:sheet_count(), 1, "sheet_count after create")
eq(wb:sheet_names()[1], "Sheet1", "default sheet name")

local ws = wb:worksheet("Sheet1")

-- values of every type -------------------------------------------------------
ws:cell("A1").value = "Hello 世界"
ws:cell("A2").value = 42
ws:cell("A3").value = 3.5
ws:cell("A4").value = true
ws:cell("A5").value = xlsx.datetime{ year = 2026, month = 10, day = 14, hour = 9, min = 30, sec = 0 }
ws:cell("A6").value = nil -- empty on purpose

eq(ws:cell("A1").value, "Hello 世界", "string roundtrip")
eq(ws:cell("A2").value, 42, "integer roundtrip")
eq(ws:cell("A3").value, 3.5, "float roundtrip")
eq(ws:cell("A4").value, true, "boolean roundtrip")
eq(math.abs(ws:cell("A5").value - 46309.395833333336) < 1e-6, true, "datetime serial")
eq(ws:cell("A6").value, nil, "empty cell is nil")

eq(ws:cell("A1"):value_type_name(), "string", "A1 type name")
eq(ws:cell("A2"):value_type_name(), "integer", "A2 type name")
eq(ws:cell("A3"):value_type_name(), "float", "A3 type name")
eq(ws:cell("A4"):value_type_name(), "boolean", "A4 type name")
eq(ws:cell("A6"):value_type_name(), "empty", "A6 type name")

-- 1-based (row, col) addressing + references ---------------------------------
eq(ws:cell(2, 1).value, 42, "cell(2,1) == A2")
eq(ws:cell("A2"):address(), "A2", "cell address")
eq(ws:cell(xlsx.reference("A2")).value, 42, "cell(reference)")

-- formulas --------------------------------------------------------------------
ws:cell("B1").formula = "SUM(A2:A3)"
eq(ws:cell("B1"):has_formula(), true, "has formula")
eq(ws:cell("B1").formula, "SUM(A2:A3)", "formula roundtrip")
ws:cell("B1").formula = nil
eq(ws:cell("B1"):has_formula(), false, "formula cleared")
ws:cell("B1"):clear() -- upstream: assigning a formula resets the cached value to 0; clear it explicitly

-- rows / columns ---------------------------------------------------------------
local row2 = ws:row(2)
eq(row2:row_number(), 2, "row number")
eq(row2:find_cell(1).value, 42, "row cell value (find_cell)")

row2.values = { "one", 2, 3.5, true }
eq(ws:cell("A2").value, "one", "row values assignment A2")
eq(ws:cell("D2").value, true, "row values assignment D2")

local t = row2.values
eq(t[1], "one", "row values read [1]")
eq(t[2], 2, "row values read [2]")
eq(t[4], true, "row values read [4]")

local col1 = ws:column(1)
col1:set_width(20)
eq(ws:column(1):width(), 20, "column width roundtrip")
eq(ws:column("A"):width(), 20, "column by letter")

-- ranges and iteration ---------------------------------------------------------
local rng = ws:range("A1:B2")
eq(rng:address(), "A1:B2", "range address")
eq(rng:num_rows(), 2, "range rows")
eq(rng:num_columns(), 2, "range cols")

local count = 0
for cell in rng:each() do
    count = count + 1
    local _ = cell:address()
end
eq(count, 4, "range iteration count")

local rows_seen = 0
for row in ws:rows():each() do
    rows_seen = rows_seen + 1
end
eq(rows_seen >= 2, true, "rows iteration")

-- merges -----------------------------------------------------------------------
ws:merge_cells("C1:D1")
eq(ws:merges():count(), 1, "merge count")
eq(ws:merges():merge_exists("C1:D1"), true, "merge exists")
ws:unmerge_cells("C1:D1")
eq(ws:merges():count(), 0, "merge removed")

-- document properties ------------------------------------------------------------
doc:set_property(xlsx.property.TITLE, "smoke test")
eq(doc:property(xlsx.property.TITLE), "smoke test", "doc property roundtrip")
eq(doc:property("title"), "smoke test", "doc property by name")

-- save / reopen / verify -----------------------------------------------------------
doc:save()
eq(doc:name(), "smoke.xlsx", "doc name")
doc:close()

local doc2 = xlsx.new_document(path)
eq(doc2:is_open(), true, "reopen")
local ws2 = doc2:workbook():worksheet("Sheet1")
eq(ws2:cell("A1").value, "Hello 世界", "reopen string")
eq(ws2:cell("A3").value, 3.5, "reopen float")
eq(ws2:cell("B1").value, nil, "reopen empty formula cell")
doc2:close()

-- error handling --------------------------------------------------------------------
-- stale handles (document closed) must raise a clean Lua error, not crash
local ok, err = pcall(function() return ws2:cell("A1").value end)
eq(ok, false, "stale handle after close raises")
eq(type(err) == "string" and err:find("closed") ~= nil, true, "stale handle error message")

local ok2, err2 = pcall(function() return xlsx.reference("!!invalid!!") end)
eq(ok2, false, "invalid reference raises")

print(string.format("OK - %d checks passed", checks))
