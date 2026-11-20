"""pytest 配置：排除「过程式脚本」被误当成测试模块收集。

`test_e2e_full.py` 是端到端演练脚本（模块级顺序执行 + sys.exit），
不是 pytest 用例。若被收集会在 import 阶段跑完 2 分钟全链路再抛
SystemExit，把整个 pytest 会话打成 INTERNALERROR。

它仍可单独运行：./.venv/bin/python test_e2e_full.py
"""

collect_ignore = [
    "test_e2e_full.py",
]
