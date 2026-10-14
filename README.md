# lua-openxlsx

**OpenXLSX（C++ 读写 .xlsx 的库）的 Lua 绑定** —— 用 Lua 读写 Excel 文件。

- 上游： [OpenXLSX v0.5.1](https://codeberg.org/lars_uffmann/OpenXLSX)（BSD-3-Clause，2026-06 发布）
- 目标 Lua： 5.5（源码同时兼容 5.4）
- 形态： 原生 C++ 模块 `openxlsx.so`，`require("openxlsx")` 直接使用
- 安装： 无 root，默认装到 `~/.local/lib/lua/5.5/`

## 快速开始

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

doc:set_property(xlsx.property.TITLE, "十月报表")
doc:save()
doc:close()
```

## 设计要点

| 主题 | 约定 |
|---|---|
| 命名 | 全部 snake_case（`setBold` → `set_bold`、`createSheet` → `create_sheet`） |
| 寻址 | 行列均为 **1-based**（与上游一致）；接受 `"A1"`、`xlsx.reference(row, col)`、`("A1")`；大小写不敏感；非法地址抛 Lua 错误而不是崩溃 |
| 取值/赋值 | `cell.value`（number/string/boolean/nil/datetime）、`cell.formula`；`row.values` 支持数组批量读写 |
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
├── scripts/bootstrap.sh        # 拉取并构建 OpenXLSX -> .deps/
├── scripts/run_tests.sh        # 跑全部 Lua 测试 + LibreOffice 校验
├── src/                        # 绑定实现（C++）
│   ├── lua_bind.hpp            # 绑定基础设施（Box/令牌/校验/类型转换）
│   ├── lua_module.cpp          # 模块入口、常量表、工具函数
│   ├── bind_document.cpp       # XLDocument / XLWorkbook / XLSheet
│   ├── bind_sheet.cpp          # XLWorksheet / 条件格式
│   └── bind_data.cpp           # 单元格/值/引用/行列/区域/迭代器/合并
├── test/                       # Lua 测试
├── tools/api_inventory.py      # 从头文件生成 API 清单（覆盖度核对用）
├── docs/API_COVERAGE.md        # API 覆盖清单
└── third_party/ .deps/         # 由 bootstrap.sh 生成（不纳入版本管理）
```

## 覆盖状态

见 [docs/API_COVERAGE.md](docs/API_COVERAGE.md)。当前已完成核心读写全部 API；样式（XLStyles）、批注、表格、VML 绘图为后续阶段。

## 许可

绑定代码 MIT。上游 OpenXLSX 为 BSD-3-Clause（见 `third_party/OpenXLSX/LICENSE.md`）。
