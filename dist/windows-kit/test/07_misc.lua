-- test/07_misc.lua — P5: equality helpers, value variant, path helpers,
--                    XML saving declaration
local xlsx = require("openxlsx")

local testdir = os.getenv("OX_TEST_DIR") or "test"
local util = dofile(testdir .. "/testutil.lua")

local outdir = os.getenv("OX_TEST_OUT") or "/tmp/opencode/openxlsx-tests"
util.makedirs(outdir)
local path = outdir .. "/misc.xlsx"
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

-- path helpers -------------------------------------------------------------------
eq(xlsx.path_relative_to("xl/comments1.xml", "xl/worksheets/sheet1.xml"), "../comments1.xml",
   "path_relative_to")
eq(xlsx.path_relative_to("xl/worksheets/sheet1.xml", "xl/worksheets/sheet2.xml"), "sheet1.xml",
   "path_relative_to same dir")
eq(xlsx.normalize_path("xl/./worksheets/../comments1.xml"), "xl/comments1.xml", "normalize_path")
local ok_bad = pcall(function() return xlsx.normalize_path("xl/../../escape") end)
eq(ok_bad, false, "normalize_path rejects escaping")

-- saving declaration ----------------------------------------------------------------
local decl = xlsx.saving_declaration("1.0", "UTF-8", true)
eq(decl.version, "1.0", "declaration version")
eq(decl.encoding, "UTF-8", "declaration encoding")
eq(decl.standalone, "yes", "declaration standalone string")
eq(decl.standalone_as_bool, true, "declaration standalone bool")

local decl2 = xlsx.saving_declaration()          -- defaults
eq(decl2.version, "1.0", "default version")
eq(decl2.standalone_as_bool, false, "default standalone")

local doc = xlsx.new_document()
doc:create(path, xlsx.FORCE_OVERWRITE)
truthy(pcall(function() doc:set_saving_declaration(decl) end), "set_saving_declaration")

local ws = doc:workbook():worksheet("Sheet1")
ws:cell("A1").value = "misc"

-- equality helpers / variant -----------------------------------------------------------
eq(ws:cell("A1"):equals(ws:cell("A1")), true, "cell equals itself")
eq(ws:cell("A1"):equals(ws:cell("B1")), false, "different cells not equal")
eq(ws:row(1):equals(ws:row(1)), true, "row equals itself")
eq(ws:row(1):equals(ws:row(2)), false, "different rows not equal")

ws:cell("A2").value = 42
ws:cell("A3").value = 3.25
ws:cell("A4").value = true
ws:cell("A5").value = "text"

local value_int = xlsx.cell_value(42)
local var = value_int:variant()
eq(var.type, xlsx.value_type.INTEGER, "variant integer type")
eq(var.value, 42, "variant integer value")

local var_s = xlsx.cell_value("hi"):variant()
eq(var_s.type, xlsx.value_type.STRING, "variant string type")
eq(var_s.value, "hi", "variant string value")

local var_b = xlsx.cell_value(true):variant()
eq(var_b.type, xlsx.value_type.BOOLEAN, "variant boolean type")
eq(var_b.value, true, "variant boolean value")

local var_e = xlsx.cell_value():variant()
eq(var_e.type, xlsx.value_type.EMPTY, "variant empty type")
eq(var_e.value, nil, "variant empty value")

doc:save()
doc:close()

-- verify the saving declaration took effect in the produced XML -------------------------
local unzip_dir = outdir .. "/misc_x"
util.rmtree(unzip_dir)
util.extract_zip(path, unzip_dir)

local function first_line(file)
    return util.read_first_line(unzip_dir .. "/" .. file) or ""
end

truthy(first_line("xl/workbook.xml"):find('standalone="yes"') ~= nil, "workbook.xml has standalone=yes")
truthy(first_line("xl/worksheets/sheet1.xml"):find('standalone="yes"') ~= nil, "sheet1.xml has standalone=yes")

print(string.format("OK - %d checks passed", checks))
