# lua-openxlsx

[![ci](https://github.com/yangqingbzhp-commits/lua-openxlsx/actions/workflows/ci.yml/badge.svg)](https://github.com/yangqingbzhp-commits/lua-openxlsx/actions/workflows/ci.yml)

**OpenXLSX（C++ 读写 .xlsx 的库）的 Lua 绑定** —— 用 Lua 读写 Excel 文件。

- 上游： [OpenXLSX v0.5.1](https://codeberg.org/lars_uffmann/OpenXLSX)（BSD-3-Clause，2026-06 发布）
- 目标 Lua： 5.5（源码同时兼容 5.4）
- 形态： 原生 C++ 模块 `openxlsx.so`，`require("openxlsx")` 直接使用
- 安装： 无 root，默认装到 `~/.local/lib/lua/5.5/`

## 平台支持

| | Linux | macOS | Windows (MinGW-w64) |
|---|---|---|---|
| 状态 | ✅ 本机 + CI 全绿 | ✅ **CI 真机全绿** | ✅ **CI 真机全绿**（真实 windows-latest runner） |
| 模块 | `openxlsx.so` | `openxlsx.so` | `openxlsx.dll` |
| 特殊点 | 解释器提供 Lua 符号 | `-undefined dynamic_lookup`（已自动处理） | **必须链接 Lua 导入库**（`OX_LINK_LUA=ON`，默认） |

三平台 CI 矩阵（每平台：源码编译 Lua 5.5 → 构建 OpenXLSX 全家 → 编译模块 → 跑全部 320 项测试）：
[Actions 运行记录](https://github.com/yangqingbzhp-commits/lua-openxlsx/actions/workflows/ci.yml)。

构建方法、交叉编译（Linux→Windows）、MSVC 说明与 CI 工作流详见
[docs/CROSS_PLATFORM.md](docs/CROSS_PLATFORM.md)。

## 快速开始

### 方式 A：LuaRocks（一行安装）

需要 LuaRocks ≥ 3.13（支持 Lua 5.5）：

```bash
luarocks install https://raw.githubusercontent.com/yangqingbzhp-commits/lua-openxlsx/main/lua-openxlsx-0.1.2-1.rockspec
```

或在源码目录里就地安装：

```bash
git clone https://github.com/yangqingbzhp-commits/lua-openxlsx && cd lua-openxlsx
luarocks make lua-openxlsx-0.1.2-1.rockspec
```

说明：
- OpenXLSX（含 pugixml/miniz，Windows 另含 Nowide）会**自动下载并构建**；
  若已用 `scripts/bootstrap.sh` 建好 `.deps/`，则直接复用（更快）；
- 需要网络、`git`、CMake ≥ 3.16 与 C++17 编译器；
- 安装后 `eval "$(luarocks path)"`（Windows: `luarocks path` 设置环境变量）即可
  `require("openxlsx")`。

### 方式 B：CMake 手动构建（可控、适合开发）

```bash
# 1) 构建依赖（克隆 OpenXLSX v0.5.1 并静态构建到项目内 .deps/，自动拉取 pugixml/miniz）
scripts/bootstrap.sh

# 2) 构建模块
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 3) 运行测试（含 LibreOffice 回读校验，如果已安装）
scripts/run_tests.sh

# 4) 安装到 ~/.local
cmake --install build
```

安装后确保 Lua 能找到模块（首次需要时把这行加入 `~/.bashrc`）：

```bash
export LUA_CPATH="$HOME/.local/lib/lua/5.5/?.so;;"
```

## 用法示例

```lua
local xlsx = require("openxlsx")

local doc = xlsx.new_document()                       -- 也可以 new_document("a.xlsx") 直接打开
doc:create("report.xlsx", xlsx.FORCE_OVERWRITE)

local ws = doc:workbook():worksheet("Sheet1")
ws:cell("A1").value = "产品"                          -- 属性糖，自动分派类型
ws:cell("B1").value = 42
ws:cell("C1").value = xlsx.datetime{ year = 2026, month = 10, day = 14 }
ws:cell("C2").formula = "SUM(B2:B10)"                 -- 公式（上游语义：写入公式会把缓存值重置为 0）

ws:cell("A2").value = "苹果"
ws:cell("B2").value = 3.5
ws:column("B"):set_width(15)
ws:merge_cells("D1:E1")
ws:row(1):set_height(28)

-- 批量写一行（走上游高性能容器赋值路径）
ws:row(3).values = { "香蕉", 1, 2.5, true }

-- 遍历区域
for cell in ws:range("A1:C3"):each() do
    print(cell:address(), cell.value)
end

-- 单元格样式：字体 / 填充 / 边框 / 数字格式 / 对齐
local styles = doc:styles()

local font_idx = styles.fonts:create()
local font = styles.fonts:get(font_idx)
font.bold = true                      -- 属性读/写
font.font_color = xlsx.color("FFFFFF")

local fill_idx = styles.fills:create()
local fill = styles.fills:get(fill_idx)
fill.pattern_type = "solid"           -- 枚举名或数字均可
fill.color = xlsx.color("4472C4")

local border_idx = styles.borders:create()
styles.borders:get(border_idx):set_bottom("thin", xlsx.color("808080"))

local fmt_idx = styles.cell_formats:create()
local fmt = styles.cell_formats:get(fmt_idx)
fmt.font_index = font_idx
fmt.fill_index = fill_idx
fmt.border_index = border_idx
fmt.apply_font = true
fmt.apply_fill = true
fmt.apply_border = true
fmt.alignment.wrap_text = true        -- 对齐
fmt.apply_alignment = true

ws:cell("A1"):set_format(fmt_idx)
ws:cell(1, 2):set_format(fmt_idx)     -- (row, col) 1-based
ws:set_column_format("C", fmt_idx)    -- 整列套用

doc:set_property(xlsx.property.TITLE, "十月报表")
doc:save()
doc:close()
```

更多示例见 `examples/demo.lua`（带样式的完整报表）。

### 批注 / VML 形状 / 关系

```lua
local comments = ws:comments()                       -- 自动创建 comments + VML 文件
comments:add_author("Alice")                          -- 返回作者 id
comments:set("A1", "这是一条批注", 0)
comments:set("B2", "第二条", 0)
comments:shape("A1"):style():show()                   -- 显示批注框
for i = 0, comments:count() - 1 do
    local c = comments:get(i)
    print(c:ref(), c:text(), comments:author(c:author_id()))
end

for _, rel in ipairs(doc:sheet_relationships(1):relationships()) do
    print(rel:type_name(), rel:target())
end
```

## 设计要点

| 主题 | 约定 |
|---|---|
| 命名 | 全部 snake_case（`setBold` → `set_bold`、`createSheet` → `create_sheet`） |
| 寻址 | 行列均为 **1-based**（与上游一致）；接受 `"A1"`、`xlsx.reference(row, col)`、`("A1")`；大小写不敏感；非法地址抛 Lua 错误而不是崩溃 |
| 取值/赋值 | `cell.value`（number/string/boolean/nil/datetime）、`cell.formula`；`row.values` 支持数组批量读写 |
| 样式 | 属性读写（`font.bold = true`）+ `get_/set_` 方法 + 容器 `create()/get()/count()`；样式对象是**活引用**（`create()` 立刻可见） |
| 迭代 | `range:each()` / `rows():each()` / `rows():each_reverse()` 返回闭包迭代器；同时暴露 C++ 风格 `begin()/end()/next()/equals()` |
| 错误 | C++ 异常统一转成 Lua error，前缀异常类名（如 `XLInputError: ...`） |
| 生命周期 | **文档关闭/GC 后，其派生句柄（worksheet/cell/row/…）会干净报错**，不会踩到已释放内存 |
| 常量 | `openxlsx.property.*`、`openxlsx.value_type.*`、`openxlsx.sheet_state.*`、`openxlsx.cf_type.*` 等 |

## 上游已知问题与绕行

实现过程中发现并绕行了这些上游问题（均已记录在代码注释中）：

1. **`XLDocument` 不能 move**：move 之后 moved-from 对象的析构会段错误（`isOpen()` 误判后 `close()` 已搬空的数据）。绑定层改为就地构造，绝不 move。
2. **非法地址是 UB**：上游不校验地址，`XLCellReference("bad")` 会越界崩溃。绑定层在进入库之前统一校验并抛 Lua 错误。
3. **公式赋值语义**：`cell.formula = "..."` 会把该单元格缓存值重置为 0（上游设计），清除公式不会清掉这个 0，必要时 `cell:clear()`。
4. **小写地址**：上游只接受大写地址，绑定层自动转大写。

## 项目结构

```
lua-openxlsx/
├── scripts/bootstrap.sh        # 拉取并构建 OpenXLSX -> .deps/（跨平台）
├── scripts/run_tests.sh        # 跑全部 Lua 测试 + LibreOffice 校验（跨平台）
├── cmake/mingw-w64-x86_64.cmake # Linux→Windows 交叉编译 toolchain
├── .github/workflows/ci.yml    # 三平台 CI（Linux/macOS/Windows）
├── src/                        # 绑定实现（C++）
│   ├── lua_bind.hpp            # 绑定基础设施（Box/令牌/校验/类型转换）
│   ├── style_machinery.hpp     # 通用属性/方法绑定机制（样式类复用）
│   ├── lua_module.cpp          # 模块入口、常量表、工具函数
│   ├── bind_document.cpp       # XLDocument / XLWorkbook / XLSheet
│   ├── bind_sheet.cpp          # XLWorksheet / 条件格式
│   ├── bind_data.cpp           # 单元格/值/引用/行列/区域/迭代器/合并
│   ├── bind_styles.cpp         # XLStyles 全家
│   ├── bind_p4.cpp             # 批注/表格/VML/关系
│   └── bind_p5.cpp             # equals/variant/路径/保存声明
├── test/                       # Lua 测试（testutil.lua 提供跨平台工具）
├── tools/api_inventory.py      # 从头文件生成 API 清单（覆盖度核对用）
├── docs/                       # API 覆盖清单、审计报告、跨平台说明
└── third_party/ .deps/         # 由 bootstrap.sh 生成（不纳入版本管理）
```

## 覆盖状态

**全部完成**：核心读写、样式（XLStyles 全家）、批注/表格/VML/关系、收尾工具（P5）。
覆盖度审计：公开头文件 62 个类 → 59 个直接绑定 + 3 个功能等价暴露，**0 缺失**
（详见 [docs/API_COVERAGE.md](docs/API_COVERAGE.md) 与 `docs/coverage_audit.txt`）。

测试规模：7 个测试文件、**320 项断言**，另含 LibreOffice 独立回读校验与生成文件的 XML 结构校验。

## 许可

绑定代码 MIT。上游 OpenXLSX 为 BSD-3-Clause（见 `third_party/OpenXLSX/LICENSE.md`）。
