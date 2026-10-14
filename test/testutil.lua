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

--- extract a zip archive
--- Unix: unzip;  Windows: unzip when available (MSYS2/Git-Bash), otherwise the
--- Windows built-in bsdtar (C:\Windows\System32\tar.exe). NOTE: plain `tar`
--- is avoided on Windows because MSYS2 ships GNU tar, which misreads "D:/..."
--- paths as remote hosts.
function M.extract_zip(zip_path, dir)
    M.makedirs(dir)
    if not is_windows then
        os.execute(string.format("unzip -o -q '%s' -d '%s'", zip_path, dir))
        return
    end
    local z, d = native(zip_path), native(dir)
    local ok = os.execute(string.format('unzip -o -q "%s" -d "%s"', z, d))
    if not ok then
        local systemroot = os.getenv("SystemRoot") or "C:\\Windows"
        os.execute(string.format('"%s\\System32\\tar.exe" -xf "%s" -C "%s"', systemroot, z, d))
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
