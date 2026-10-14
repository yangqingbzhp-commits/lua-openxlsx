# API 覆盖清单（OpenXLSX 0.5.1 → Lua）

本清单是"全量覆盖"的验收依据。状态图例：
- ✅ 已绑定（含 Lua 侧名字）
- ⏳ 计划中（后续阶段）
- ➖ 不暴露（库内部实现细节 / 无法映射，原因已注明）

Lua 侧命名规则：C++ `camelCase` → Lua `snake_case`；枚举值进对应常量表（如 `XLSheetState::VeryHidden` → `openxlsx.sheet_state.VERY_HIDDEN`）。

## XLDocument（`openxlsx.new_document(path?)`）

| C++ | Lua | 状态 |
|---|---|---|
| `open(fileName)` | `doc:open(path)` | ✅ |
| `create(fileName, forceOverwrite)` | `doc:create(path[, force_overwrite])` | ✅ |
| `close()` | `doc:close()` | ✅ |
| `save()` | `doc:save()` | ✅ |
| `saveAs(fileName, forceOverwrite)` | `doc:save_as(path[, force_overwrite])` | ✅ |
| `name()` / `path()` | `doc:name()` / `doc:path()` | ✅ |
| `isOpen()` / `operator bool` | `doc:is_open()` | ✅ |
| `workbook()` | `doc:workbook()` | ✅ |
| `property(prop)` / `setProperty` / `deleteProperty` | `doc:property(p)` / `doc:set_property(p,v)` / `doc:delete_property(p)` | ✅ |
| `validateSheetName(name, throwOnInvalid)` | `doc:validate_sheet_name(name[, throw])` | ✅ |
| `showWarnings()` / `suppressWarnings()` | `doc:show_warnings()` / `doc:suppress_warnings()` | ✅ |
| `cleanupSharedStrings()` | `doc:cleanup_shared_strings()` | ✅ |
| `hasSheetRelationships(id)` 等 4 个 `hasSheet*` | `doc:has_sheet_relationships(id)` 等 | ✅ |
| `sheetRelationships/VmlDrawing/Comments/Tables(id)` | `doc:sheet_*` | ⏳ P4（需先绑定 XLRelationships/XLVmlDrawing/XLComments/XLTables） |
| `styles()` | `doc:styles()` | ⏳ P3（XLStyles） |
| `setSavingDeclaration(...)` | — | ⏳ P5（XLXmlSavingDeclaration，内部配置对象，待评估是否暴露） |
| `execCommand/execQuery` | — | ➖ 内部命令/查询机制；其功能已由专用方法暴露 |
| `sharedStrings()` | — | ➖ 内部字符串表；已暴露 `cleanup_shared_strings` |
| `print(ostream)` | — | ➖ C++ 流输出，无 Lua 语义 |

## XLWorkbook（`doc:workbook()`）

| C++ | Lua | 状态 |
|---|---|---|
| `sheet(name/index)` | `wb:sheet(name_or_index)` | ✅ |
| `worksheet(name/index)` / `chartsheet(name/index)` | `wb:worksheet(...)` / `wb:chartsheet(...)` | ✅ |
| `addWorksheet` / `deleteSheet` / `cloneSheet` | `wb:add_worksheet` / `wb:delete_sheet` / `wb:clone_sheet` | ✅ |
| `setSheetIndex` / `indexOfSheet` / `typeOfSheet` | `wb:set_sheet_index` / `wb:index_of_sheet` / `wb:type_of_sheet(+_name)` | ✅ |
| `sheetCount` / `worksheetCount` / `chartsheetCount` | 同名 | ✅ |
| `sheetNames` / `worksheetNames` / `chartsheetNames` | 同名（返回数组） | ✅ |
| `sheetExists` / `worksheetExists` / `chartsheetExists` | 同名 | ✅ |
| `updateSheetReferences` / `deleteNamedRanges` / `setFullCalculationOnLoad` | 同名 | ✅ |
| `print(ostream)` | — | ➖ |

## XLSheet（抽象工作表，`wb:sheet()` 返回值）

| C++ | Lua | 状态 |
|---|---|---|
| `visibility` / `setVisibility` | `sheet:visibility()`（字符串）/ `visibility_num()` / `set_visibility(v)` | ✅ |
| `color` / `setColor` | `sheet:color()` / `set_color(c)` | ✅ |
| `index` / `setIndex` / `name` / `setName` | 同名 | ✅ |
| `isSelected` / `setSelected` / `isActive` / `setActive` | 同名 | ✅ |
| `isType<XLWorksheet>()` 等 | `sheet:is_worksheet()` / `is_chartsheet()` | ✅ |
| `get<XLWorksheet>()` | `sheet:as_worksheet()` / `as_chartsheet()` | ✅ |
| `clone(newName)` | `sheet:clone(name)` | ✅ |
| `print(ostream)` | — | ➖ |

## XLWorksheet（`wb:worksheet()`）

| 组 | Lua 方法 | 状态 |
|---|---|---|
| 单元格 | `cell(...)`（`"A1"`/`(row,col)`/`reference`）、`find_cell(...)` | ✅ |
| 区域 | `range()` / `range("A1:C5")` / `range("A1","C5")` / `range(ref,ref)` | ✅ |
| 行列 | `row(n)`、`column(n|"A")`、`rows()` / `rows(count)` / `rows(first,last)` | ✅ |
| 规模 | `last_cell()`、`row_count()`、`column_count()`、`delete_row(n)` | ✅ |
| 尺寸 | `set_dimension(...)`（3 种重载） | ✅ |
| 合并 | `merges()`（XLMergeCells）、`merge_cells(range_or_str[, empty_hidden])`、`unmerge_cells(...)` | ✅ |
| 样式列/行 | `get_column_format` / `set_column_format` / `get_row_format` / `set_row_format` | ✅ |
| 条件格式 | `conditional_formats()` | ✅ |
| 保护 | `protect_sheet/objects/scenarios`、`allow_*` ×6、`deny_*` ×6、`set_password(_hash)`、`clear_password`、`clear_sheet_protection`、`sheet_protected` 等 9 个 getter、`password_hash`、`password_is_set`、`sheet_protection_summary` | ✅ |
| 附件 | `has_relationships` / `has_vml_drawing` / `has_comments` / `has_tables` | ✅ |
| 附件对象 | `comments()` / `tables()` / `vml_drawing()` | ⏳ P4 |
| 其他 | `update_sheet_name(old,new)`、`clone`、`name/set_name`、`index/set_index`、`visibility/…`、`color/set_color`、`is_selected/set_selected`、`is_active/set_active` | ✅ |

## XLChartsheet

上游仅部分实现（`isActive`/`setActive` 是占位）。绑定：`wb:chartsheet(...)` + 基类方法（`name/index/visibility/color/clone` 等）。✅（受上游限制）

## XLCell（`ws:cell(...)`）

| C++ | Lua | 状态 |
|---|---|---|
| `value()`（XLCellValueProxy） | `cell.value`（读/写/置 nil 清除） + `cell:get_value/set_value` | ✅ |
| `formula()`（XLFormulaProxy） | `cell.formula`（读写） + `get_formula/set_formula/clear_formula` | ✅ |
| `hasFormula` / `empty` | `has_formula()` / `empty()` | ✅ |
| `clear(keep)` | `clear([keep])`，keep 常量 `openxlsx.keep.*` | ✅ |
| `cellReference` / `offset` | `reference()` / `offset(row, col)` / `address()` | ✅ |
| `cellFormat` / `setCellFormat` | `format()` / `set_format(idx)` | ✅ |
| `getString` | `get_string()` | ✅ |
| `copyFrom` | `copy_from(other)` | ✅ |
| `operator==` / `!=` | — | ⏳ P5（`cell:equals(other)`） |
| `print(ostream)` | — | ➖ |

## XLCellValue / XLCellValueProxy（`xlsx.cell_value(v)`、`cell.value`）

| C++ | Lua | 状态 |
|---|---|---|
| `type()` / `typeAsString()` | `type()`（数值）/ `type_as_string()` | ✅ |
| `get<T>()` 各类型 | `get()`（原生 Lua 值）/ `get_integer/get_boolean/get_string/get_double/get_datetime` | ✅ |
| `getDouble` / `getString` | `get_double()` / `get_string()` | ✅ |
| `set(v)` / `operator=` | `set(v)`；`cell.value = v` | ✅ |
| `clear()` / `setError(e)` | `clear()` / `set_error(msg)` | ✅ |
| 比较运算 | `==`、`<`、`<=` | ✅ |
| `getVariant()` | — | ⏳ P5（`variant()` 返回 `{type=…, value=…}`） |

## XLCellReference（`xlsx.reference(...)`）

`address/row/column/set_row/set_column/set_row_and_column/set_address` + 比较 + `__tostring` ✅；静态工具：`xlsx.column_as_string/column_as_number/row_as_string/row_as_number/coordinates_from_address` ✅

## XLDateTime（`xlsx.datetime(serial|table)`、`xlsx.datetime_from_unix(t)`）

`serial()` / `tm()`（表）/ `set_serial` / `set_tm` / `__tostring` ✅

## XLFormula / XLFormulaProxy

`xlsx.formula(s)`：`get/set/clear/to_string` ✅；单元格侧同上。

## XLColor（`xlsx.color(...)`）

`alpha/red/green/blue/hex/set`、`==`、`__tostring` ✅

## XLRow / XLColumn

| C++ | Lua | 状态 |
|---|---|---|
| `height/setHeight/descent/setDescent/isHidden/setHidden/rowNumber/cellCount/empty` | 同名 snake_case | ✅ |
| `values()`（XLRowDataProxy） | `row.values`（属性，读/写数组）+ `get_values/set_values/clear_values` | ✅ |
| `cells(...)` | `row:cells()` / `cells(count)` / `cells(first,last)` | ✅ |
| `findCell(col)` / `format` / `setFormat` | `find_cell` / `format` / `set_format` | ✅ |
| 比较运算 | — | ⏳ P5（`row:equals(other)`） |
| XLColumn：`width/setWidth/isHidden/setHidden/format/setFormat` | 同名 snake_case | ✅ |

## 区域 / 迭代器 / 行数据

| 类 | Lua 映射 | 状态 |
|---|---|---|
| `XLCellRange` | `range()` 对象：`top_left/bottom_right/address/num_rows/num_columns/clear/set_format/fetch_column_styles/set_value/each/begin/end` | ✅ |
| `XLCellIterator` | `begin()/end()` 返回：`next/cell/address/at_end/cell_exists/distance/equals`；`range:each()` 闭包 | ✅ |
| `XLRowRange` | `rows()` 对象：`row_count/each/each_reverse/begin/end/rbegin/rend`、`#rows` | ✅ |
| `XLRowIterator` / `XLRowReverseIterator` | `next/row/at_end/row_exists/row_number/equals/direction` | ✅ |
| `XLRowDataRange` | `row:cells(...)` 对象：`size/each/set_value/begin/end` | ✅ |
| `XLRowDataIterator` | `next/cell/equals` | ✅ |
| `XLRowDataProxy` 容器转换 | `row.values`（数组读写，等价 vector/deque/list 转换 + 赋值） | ✅ |

## XLMergeCells（`ws:merges()`）

`valid/find_merge/merge_exists/find_merge_by_cell/count/merge/append_merge/delete_merge/delete_all`、`#merges`、常量 `MERGE_NOT_FOUND` ✅

## 条件格式（`ws:conditional_formats()`）

| 类 | Lua 方法 | 状态 |
|---|---|---|
| `XLConditionalFormats` | `empty/count/get/create/summary`、`#cfs` | ✅ |
| `XLConditionalFormat` | `empty/sqref/set_sqref/cf_rules/summary` | ✅ |
| `XLCfRules` | `empty/max_priority_value/set_priority/renumber_priorities/count/get/create/summary`、`#rules` | ✅ |
| `XLCfRule` | `empty/formula/type/type_name/dxf_id/priority/stop_if_true/above_average/percent/bottom/cf_operator(+_name)/text/time_period(+_name)/rank/std_dev/equal_average` + 对应 `set_*` + `summary` | ✅ |
| `colorScale/dataBar/iconSet/extLst`（上游未实现） | — | ➖ 上游 `XLUnsupportedElement` |
| 枚举 | `openxlsx.cf_type.*` / `cf_operator.*` / `cf_time_period.*` | ✅ |

## 待完成（后续阶段）

| 项 | 计划 |
|---|---|
| `XLStyles` 全家（XLFont/XLFill/XLBorder/XLNumberFormat/XLAlignments/XLColumnStyles/XLRowStyles/XLStyle…） | **P3**（最大块，上游 86KB 头文件） |
| `XLComments` / `XLComment`（批注）、`XLTables`、`XLVmlDrawing`、`XLDrawing*` | **P4** |
| `doc:sheet_relationships/vml_drawing/comments/tables`、`ws:comments/tables/vml_drawing` | **P4** |
| `cell:equals` / `row:equals`、`value:variant()`、`xlsx.path_relative_to` / `normalize_path` | **P5** |
| 覆盖度自动核对（`tools/api_inventory.py` 生成 → 与本清单比对） | **P5** |

## 无法映射/不暴露的项（汇总）

- 所有返回 `std::ostream&` 的 `print()`：C++ 流调试输出，无 Lua 语义。
- `headers/detail/*`、`XLXmlParser*`、`XLXmlData/XLXmlFile`、`XLContentTypes`、`XLRelationships`、`XLSharedStrings`、`XLCommandQuery`、`IZipArchive`：库内部基础设施；需要的信息通过公开 API 暴露（如 `cleanup_shared_strings`、`zip_library_name`）。
- `XLProperties`/`XLAppProperties` 类本身：通过 `doc:property()` / `set_property()` 访问。
