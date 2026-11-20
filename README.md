# 地图工具 bag 回放 —— B 线可视化回归验证

MapTool × roboneo_sim 融合：毫秒级前视快算预判 + 复跑直播跟车 + 问题分支改参数二次复跑。
产品策划书与任务分解见知识库 04_Projects/B线可视化回归验证/。

## 同步到 GitHub（每次改完必做）

远端：<https://github.com/gaijianwei1-code/bag->（分支 `main`，SSH 免密）

```bash
bash sync.sh                   # 自动生成提交信息
bash sync.sh "修复 xxx"        # 指定提交信息
```

脚本会 `add -A` → `commit` → `push` → 校验远端哈希一致。工作区无改动时只做推送检查，不会产生空提交。


## 结构
- `maptool-sim/mac-tool-src/`：Mac 版 MapTool 源码（自 ~/Documents/Codex/2026-10-12/n-h/修图工具/地图工具/ 引入，CMake 跨平台版，Mac 已有构建成功先例）——开发基础
- `maptool-sim/`（根下新增模块）：L0 前视快算、场地档案向导、回放图层、回归面板
- `sim-bridge/`：Python（FastAPI/WS）桥接层 + 场地档案（venue registry）
- `docs/`：时间线 JSON schema、场地档案 schema、bridge API 契约、属性映射表、构建文档

## 约束
- roboneo_sim 与 MapTool 原工程（~/地图轨迹/map_tool_v1.2.00_src 等）均为只读参考
- 分支切换走 git worktree --detach；参数走参数集副本；初始位姿从 bag 自动提取