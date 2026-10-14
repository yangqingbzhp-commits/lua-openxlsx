-- test/testutil.lua — cross-platform helpers for the test suite
-- (Linux / macOS / Windows; on Windows run from MSYS2 or Git-Bash)
local M = {}

local is_windows = package.config:sub(1, 1) == "\\"

local function native(path)
    if is_windows then return (path:gsub("/", "\\")) end
    return path
end

--- create directory (and parents) if missing
function M.makedirs(path)
    if is_windows then
        os.execute('if not exist "' .. native(path) .. '" mkdir "' .. native(path) .. '" >NUL 2>NUL')
    else
        os.execute("mkdir -p '" .. path .. "'")
    end
end

--- remove a file, ignoring errors
function M.remove(path)
    os.remove(path)
end

--- recursively remove a directory, ignoring errors
function M.rmtree(path)
    if is_windows then
        os.execute('rmdir /s /q "' .. native(path) .. '" >NUL 2>NUL')
    else
        os.execute("rm -rf '" .. path .. "'")
    end
end

--- extract a zip archive (uses unzip on Unix, bsdtar on Windows 10+)
function M.extract_zip(zip_path, dir)
    M.makedirs(dir)
    if is_windows then
        os.execute(string.format('tar -xf "%s" -C "%s"', native(zip_path), native(dir)))
    else
        os.execute(string.format("unzip -o -q '%s' -d '%s'", zip_path, dir))
    end
end

--- first line of a file, or nil
function M.read_first_line(path)
    local f = io.open(path, "r")
    if not f then return nil end
    local line = f:read("*l")
    f:close()
    return line
end

--- path separator used by the host
function M.is_windows()
    return is_windows
end

return M
