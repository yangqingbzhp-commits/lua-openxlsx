openxlsx Windows 测试包
=======================

内容
----
  openxlsx.dll   Lua 模块（MinGW-w64 / x64 构建，导入 lua55.dll）
  lua55.dll      Lua 5.5 运行时
  lua.exe        Lua 5.5 解释器
  smoke.lua      最小自检脚本（写→存→读 + 样式/公式/批注）
  run_smoke.cmd  双击运行自检
  test\          完整测试套件（7 个文件，320 项断言）
  run_tests.cmd  双击运行全套测试
  README.txt     本文件

怎么用
------
1) 双击 run_smoke.cmd —— 预期最后输出 "RESULT: PASS"，并生成 smoke-out.xlsx
   （可直接用 Excel 打开检查内容）。
2) 双击 run_tests.cmd —— 预期最后输出 "ALL TESTS PASSED"。

要求
----
* 64 位 Windows 10/11（测试套件中的 .xlsx 解包检查用到系统自带 tar.exe，
  Windows 10 1803+ 内置）。
* 无需安装任何东西：Lua 运行时已打包。
* 若杀毒软件拦截，请允许运行（这是本地测试程序）。

结果反馈
--------
有任何 FAIL 或 FATAL，请把窗口输出（或输出重定向的文件）发回反馈。
