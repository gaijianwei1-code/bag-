# 冒烟测试（节点级强制，2026-11-18 用户要求）

## 一条命令跑完
```bash
bash docs/smoke/run_smoke.sh
```
2026-11-18 实测：**6/6 全绿，0 失败**（主构建 + 3 个 C++ 冒烟 + 21 个 bridge pytest）

## 标准测试样本（唯一指定）
`testdata/scenario_1793346971223/`，详见 testdata/README.md。
用户 2026-11-18 指定：**以后冒烟一律用这套，不临时找文件。**

## 六项内容
| # | 项 | 说明 |
|---|---|---|
| 1 | 主构建 | MapTool.app 全绿 |
| 2 | 冒烟工程构建 | core + UI 集成 + 前视回归 三个目标 |
| 3 | test_core | 投影/raycast 18 断言 + 性能基准 |
| 4 | test_ui_integration | 标准样本 pgm+json 加载与图层 |
| 5 | test_fov_regression | 真实 pcd + 场景 clear 悬垂 + z 值遮挡回归 |
| 6 | bridge pytest | 21 用例（接口正反 + WS 契约） |

## 教训（已固化为规范）
1. **集成层必须测"场景重入"**：`scene->clear()` 后重开是 Qt GraphicsView 最易漏的生命周期点。子代理 11 断言全过，用户点一下闪退——因为没测这个路径。
2. **z 值要写参照系**：不只"低于轨迹"，要写清"高于地图(0)、低于工具栏(1000)"。
3. **core 与集成分开验证**：core 独立复现器跑了命中 4 处证明几何对，才敢断定崩溃在集成层——避免瞎改算法。
4. **崩溃栈优先查系统 .ips**：`~/Library/Logs/DiagnosticReports/*.ips` 解析 threads[0].frames，比无头 lldb 更省事，直接指到 `QGraphicsItem::zValue()` 被调用于已析构对象。
