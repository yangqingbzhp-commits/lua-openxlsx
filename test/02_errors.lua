-- test/02_errors.lua — input validation and error reporting
local xlsx = require("openxlsx")

local checks = 0
local function fails(fn, pattern, what)
    checks = checks + 1
    local ok, err = pcall(fn)
    if ok then error("FAIL " .. what .. ": expected an error", 2) end
    if pattern and not tostring(err):find(pattern) then
        error(string.format("FAIL %s: error '%s' does not match '%s'", what, tostring(err), pattern), 2)
    end
end

local function works(fn, what)
    checks = checks + 1
    local ok, err = pcall(fn)
    if not ok then error("FAIL " .. what .. ": unexpected error: " .. tostring(err), 2) end
end

-- invalid references ----------------------------------------------------------
fails(function() return xlsx.reference("!!invalid!!") end, "invalid cell address", "garbage reference")
fails(function() return xlsx.reference("ZZZZ9") end, "invalid cell address", "too many letters")
fails(function() return xlsx.reference("A0") end, "invalid cell address", "row 0")
fails(function() return xlsx.reference("XFE1") end, "invalid cell address", "column beyond XFD")
fails(function() return xlsx.reference("A1048577") end, "invalid cell address", "row beyond max")
fails(function() return xlsx.reference("A-1") end, "invalid cell address", "negative row")
fails(function() return xlsx.reference(0, 1) end, "row out of range", "row 0 via numbers")
fails(function() return xlsx.reference(1, 20000) end, "column out of range", "column 20000")
fails(function() return xlsx.column_as_number("!!!") end, "invalid column", "bad column letters")
fails(function() return xlsx.row_as_number("abc") end, "invalid row", "bad row digits")
fails(function() return xlsx.coordinates_from_address("nope") end, "invalid cell address", "bad coordinates")
fails(function() return xlsx.column_as_string(0) end, "column out of range", "column_as_string(0)")

-- valid edge cases ---------------------------------------------------------------
works(function() assert(xlsx.reference("A1"):address() == "A1") end, "A1 valid")
works(function() assert(xlsx.reference("XFD1048576"):address() == "XFD1048576") end, "last cell valid")
works(function() assert(xlsx.reference("aa1"):address() == "AA1") end, "lowercase letters")
works(function() assert(xlsx.column_as_string(16384) == "XFD") end, "column_as_string(XFD)")
works(function() assert(xlsx.column_as_number("xfd") == 16384) end, "column_as_number lowercase")
works(function()
    local row, col = xlsx.coordinates_from_address("C7")
    assert(row == 7 and col == 3)
end, "coordinates_from_address")

-- worksheet cell addressing ---------------------------------------------------------
local doc = xlsx.new_document()
doc:create((os.getenv("OX_TEST_OUT") or "/tmp/opencode/openxlsx-tests") .. "/errors.xlsx")
local ws = doc:workbook():worksheet("Sheet1")

fails(function() return ws:cell("bad") end, "invalid cell address", "ws:cell bad address")
fails(function() return ws:cell(0, 1) end, "row out of range", "ws:cell row 0")
fails(function() return ws:cell(1, 0) end, "column out of range", "ws:cell col 0")
fails(function() return ws:row(0) end, "row out of range", "ws:row(0)")
fails(function() return ws:row(1048577) end, "row out of range", "ws:row too large")
fails(function() return ws:column(16385) end, "column out of range", "ws:column too large")
fails(function() return ws:column("XFE") end, "invalid column", "ws:column bad letters")
fails(function() return ws:range("A1:bad") end, "invalid range address", "ws:range bad")
fails(function() return ws:merge_cells("A1:B2:C3") end, "invalid range address", "merge bad range")

-- value assignment errors --------------------------------------------------------------
local c = ws:cell("A1")
fails(function() c.value = {} end, "unsupported value type", "assigning a table to cell.value")
fails(function() c.foo = 1 end, "read%-only", "unknown cell property write")
fails(function() return c:set_value({}) end, "cannot convert", "set_value with a table")

-- type errors ----------------------------------------------------------------------------
c.value = "text"
local v = xlsx.cell_value("text")
fails(function() return v:get_boolean() end, "XLValueTypeError", "get_boolean on a string")
fails(function() return v:get_datetime() end, "XLValueTypeError", "get_datetime on a string")
works(function() assert(v:get_string() == "text") end, "get_string on a string")

-- document property errors ------------------------------------------------------------------
fails(function() return doc:property("nonsense") end, "invalid XLProperty", "bad property name")
fails(function() doc:set_property("TITLE") end, nil, "missing property value")

-- wrong receiver types -----------------------------------------------------------------------
fails(function() ws:set_color("red") end, "openxlsx.color expected", "ws:set_color with a string")
fails(function() return ws:cell({}) end, "number expected", "ws:cell with a table")

-- open / save errors ---------------------------------------------------------------------------
fails(function() return xlsx.new_document("/nonexistent/dir/nope.xlsx") end, "failed to load archive", "open missing file")

doc:close()
print(string.format("OK - %d checks passed", checks))
