-- test/03_lifetime.lua — handle lifetime rules (document close / GC)
local xlsx = require("openxlsx")

local testdir = os.getenv("OX_TEST_DIR") or "test"
local util = dofile(testdir .. "/testutil.lua")

local outdir = os.getenv("OX_TEST_OUT") or "/tmp/opencode/openxlsx-tests"
util.makedirs(outdir)

local checks = 0
local function eq(a, b, what)
    checks = checks + 1
    if a ~= b then error(string.format("FAIL %s: expected %s, got %s", what, tostring(b), tostring(a)), 2) end
end

-- 1. handles derived from a closed document must raise a clean error -------------
local doc = xlsx.new_document()
doc:create(outdir .. "/lifetime.xlsx")
local ws = doc:workbook():worksheet("Sheet1")
ws:cell("A1").value = "alive"
eq(ws:cell("A1").value, "alive", "value before close")

doc:save()
doc:close()

local ok, err = pcall(function() return ws:cell("A1").value end)
eq(ok, false, "worksheet handle after close raises")
eq(tostring(err):find("closed") ~= nil, true, "error mentions closed document")

-- 2. the document itself may be reused after close ----------------------------------
doc:create(outdir .. "/lifetime2.xlsx")
local ws2 = doc:workbook():worksheet("Sheet1")
ws2:cell("A1").value = "second generation"
eq(ws2:cell("A1").value, "second generation", "new handles work after reopen")

-- 3. old generation handles stay invalid after reopen --------------------------------
local ok2 = pcall(function() return ws:cell("A1").value end)
eq(ok2, false, "old-generation handle stays invalid")

-- 4. garbage-collecting the document invalidates its handles --------------------------
local doc3 = xlsx.new_document()
doc3:create(outdir .. "/lifetime3.xlsx")
local ws3 = doc3:workbook():worksheet("Sheet1")
ws3:cell("A1").value = "gc test"
doc3:save()
doc3 = nil
collectgarbage("collect")
collectgarbage("collect")
local ok4, err4 = pcall(function() return ws3:cell("A1").value end)
eq(ok4, false, "handle after document GC raises")
eq(tostring(err4):find("closed") ~= nil, true, "GC error mentions closed document")

-- 5. path / name behaviour --------------------------------------------------------------
local doc5 = xlsx.new_document()
doc5:create(outdir .. "/lifetime5.xlsx")
eq(doc5:name(), "lifetime5.xlsx", "document name")
eq(doc5:path():sub(-14), "lifetime5.xlsx", "document path")
eq(doc5:is_open(), true, "is_open after create")
doc5:save_as(outdir .. "/lifetime5-renamed.xlsx", true)
eq(doc5:name(), "lifetime5-renamed.xlsx", "name after save_as")
doc5:close()
eq(doc5:is_open(), false, "is_open after close")

print(string.format("OK - %d checks passed", checks))
