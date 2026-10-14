-- test/04_workbook.lua — workbook / sheet management, protection, conditional formats
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
local function truthy(a, what)
    checks = checks + 1
    if not a then error("FAIL " .. what .. ": expected truthy value", 2) end
end

local doc = xlsx.new_document()
doc:create(outdir .. "/workbook.xlsx")
local wb = doc:workbook()

-- sheet management ------------------------------------------------------------
eq(wb:sheet_count(), 1, "initial sheet count")
eq(wb:worksheet_count(), 1, "initial worksheet count")

wb:add_worksheet("Data")
wb:add_worksheet("Report")
eq(wb:sheet_count(), 3, "sheet count after adding")
eq(#wb:sheet_names(), 3, "sheet_names array")

local names = wb:sheet_names()
eq(names[1], "Sheet1", "first sheet name")
eq(names[2], "Data", "second sheet name")
eq(names[3], "Report", "third sheet name")

eq(wb:worksheet_names()[2], "Data", "worksheet_names")
eq(wb:index_of_sheet("Report"), 3, "index_of_sheet")
eq(wb:sheet_exists("Data"), true, "sheet_exists")
eq(wb:worksheet_exists("Nope"), false, "worksheet_exists false")
eq(wb:type_of_sheet("Data"), xlsx.sheet_type.WORKSHEET, "type_of_sheet")
eq(wb:type_of_sheet_name("Data"), "worksheet", "type_of_sheet_name")

wb:set_sheet_index("Report", 1)
eq(wb:index_of_sheet("Report"), 1, "set_sheet_index")
-- order is now: Report(1), Sheet1(2), Data(3)
eq(wb:index_of_sheet("Data"), 3, "order after set_sheet_index")

wb:clone_sheet("Data", "DataCopy")
eq(wb:worksheet_exists("DataCopy"), true, "clone_sheet")

wb:delete_sheet("DataCopy")
eq(wb:worksheet_exists("DataCopy"), false, "delete_sheet")

-- sheet objects ----------------------------------------------------------------
local sheet = wb:sheet("Data")
eq(sheet:name(), "Data", "sheet name")
eq(sheet:is_worksheet(), true, "sheet is_worksheet")
eq(sheet:is_chartsheet(), false, "sheet is_chartsheet")
local ws = sheet:as_worksheet()
eq(type(ws), "userdata", "as_worksheet")
eq(wb:sheet(3):name(), "Data", "sheet by index")

ws:set_name("DataSheet")
eq(wb:worksheet_names()[3], "DataSheet", "renamed worksheet")
ws:set_name("Data")

-- visibility / selection / color ---------------------------------------------------
local ws2 = wb:worksheet("Report")
eq(ws2:visibility(), "visible", "visibility visible")
ws2:set_visibility(xlsx.sheet_state.HIDDEN)
eq(ws2:visibility(), "hidden", "visibility hidden")
eq(ws2:visibility_num(), xlsx.sheet_state.HIDDEN, "visibility_num")
ws2:set_visibility("visible")
eq(ws2:visibility(), "visible", "visibility restored")

eq(ws:index(), 3, "sheet index")
ws:set_selected(false)
eq(ws:is_selected(), false, "set_selected")
ws:set_selected(true)

ws:set_color(xlsx.color(255, 0, 0))
eq(ws:color():hex(), "ffff0000", "sheet tab color (ARGB hex)")

-- protection ------------------------------------------------------------------------
eq(ws:sheet_protected(), false, "not protected")
truthy(ws:protect_sheet(true), "protect_sheet")
eq(ws:sheet_protected(), true, "protected")
truthy(ws:protect_objects(false), "protect_objects")
eq(ws:objects_protected(), false, "objects_protected")
truthy(ws:deny_insert_rows(), "deny_insert_rows")
eq(ws:insert_rows_allowed(), false, "insert_rows_allowed")
truthy(ws:allow_insert_rows(), "allow_insert_rows")
eq(ws:insert_rows_allowed(), true, "insert_rows_allowed restored")

truthy(ws:set_password("secret"), "set_password")
truthy(ws:password_is_set(), "password_is_set")
eq(type(ws:password_hash()), "string", "password_hash")
truthy(ws:clear_password(), "clear_password")
truthy(ws:clear_sheet_protection(), "clear_sheet_protection")
eq(ws:sheet_protected(), false, "protection cleared")

-- conditional formats -------------------------------------------------------------------
local cfs = ws:conditional_formats()
eq(cfs:count(), 0, "no conditional formats")
local idx = cfs:create()
eq(cfs:count(), 1, "conditional format created")
eq(#cfs, 1, "conditional formats __len")

local cf = cfs:get(idx)
truthy(cf:set_sqref("A1:A10"), "set_sqref")
eq(cf:sqref(), "A1:A10", "sqref roundtrip")

local rules = cf:cf_rules()
eq(rules:count(), 0, "no rules")
local rIdx = rules:create()
eq(rules:count(), 1, "rule created")
local rule = rules:get(rIdx)
truthy(rule:set_type("cell_is"), "set_type")
eq(rule:type(), xlsx.cf_type.CELL_IS, "rule type value")
eq(rule:type_name(), "cell_is", "rule type name")
truthy(rule:set_operator("greater_than"), "set_operator")
eq(rule:cf_operator_name(), "greater_than", "operator name")
truthy(rule:set_formula("5"), "set_formula")
eq(rule:formula(), "5", "rule formula")
local _ = rule:summary()

-- dimension / counts / rows --------------------------------------------------------------
ws:cell("A1").value = 1
ws:cell("C5").value = "end"
eq(ws:last_cell():address(), "C5", "last_cell")
eq(ws:row_count(), 5, "row_count")
eq(ws:column_count(), 3, "column_count")

ws:row(3):set_height(30)
eq(ws:row(3):height(), 30, "row height")
ws:row(3):set_hidden(true)
eq(ws:row(3):is_hidden(), true, "row hidden")
ws:row(3):set_hidden(false)
ws:column("B"):set_width(25)
eq(ws:column(2):width(), 25, "column width by index")

local del = ws:delete_row(5)
eq(del, true, "delete_row")

-- save & verify reopen -------------------------------------------------------------------
doc:save()
doc:close()

local doc2 = xlsx.new_document(outdir .. "/workbook.xlsx")
local wb2 = doc2:workbook()
eq(wb2:sheet_count(), 3, "sheets persisted")
eq(wb2:worksheet_names()[3], "Data", "worksheet names persisted")
local ws3 = wb2:worksheet("Data")
eq(ws3:cell("A1").value, 1, "value persisted")
eq(ws3:row(3):height(), 30, "row height persisted")
eq(ws3:column("B"):width(), 25, "column width persisted")
eq(ws3:conditional_formats():count(), 1, "conditional format persisted")
doc2:close()

print(string.format("OK - %d checks passed", checks))
