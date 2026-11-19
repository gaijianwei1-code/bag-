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

## ROS 话题"有没有数据"的正确判定方法（2026-11-19 血的教训）

排查 lookahead 话题时连续 5 轮假阴性，全部源于**判定方法不可靠**，不是数据真的没有：

| 方法 | 可靠性 | 问题 |
|---|---|---|
| `rostopic hz` 输出含 "average rate" | ❌ 不可靠 | 有订阅但无数据时也可能打印 |
| `rostopic echo \| head -6 \| grep "x:"` | ❌ 不可靠 | head 截断，字段常在更后面 |
| **`seq` 字段递增** | ✅ **可靠** | ROS 保证序列号单调递增，是硬证据 |
| `rostopic echo -n 1` 拿到完整消息 | ✅ 可靠 | 但要注意字段顺序 |

**推荐做法**：
```bash
# 间隔取两次，比较 seq 是否增长
rostopic echo -n 1 /topic 2>&1 | grep 'seq:'
sleep 3
rostopic echo -n 1 /topic 2>&1 | grep 'seq:'
# seq 增长 = 有数据流
```

## 跨 docker + bash -lc + python 三层嵌套的坑

❌ **不要在命令行内嵌 python 代码**：`docker exec bash -lc "python3 -c '...'"` 的引号会被
shell 层吃掉，脚本可能压根没执行或输出被截断。

✅ **正确做法**：写独立脚本文件 → `docker cp` 进容器 → `python3 /path/script.py`。
输出用固定前缀（如 `DIAG ` / `SAMPLES `）便于宿主机解析。

## 采集窗口 vs 回放窗口的时序

采集脚本启动过早（回放未开始）或窗口过短（回放未完）都会得到 0 样本。
正确做法：**先检测话题出现（轮询 rostopic list）→ 再开始采集 → 采集时长覆盖剩余回放时间**。

## ROS 消息字段访问的致命陷阱（2026-11-19，绕了 2 小时）

**`PointStamped` 用 `msg.point.x`，不是 `msg.pose.position.x`**：

| 消息类型 | 正确访问 |
|---|---|
| `geometry_msgs/PointStamped` | `msg.point.x` / `.y` / `.z` |
| `geometry_msgs/PoseStamped` | `msg.pose.position.x` / `.pose.orientation` |
| `nav_msgs/Path` | `msg.poses[i].pose.position.x` |

**为什么这个错误特别难查**：rospy 的回调里如果抛异常（如 `AttributeError`），
**异常被静默吞掉**，表现为「订阅了但收不到数据」，很容易误判成：
- 话题名错
- 没有发布者
- sim time 问题
- 网络/环境问题

**正确做法（写回调的第一件事）**：
```python
def cb(msg):
    print("收到:", msg)      # 先看一眼真实结构，再写字段访问
    ...
```

或先验证类型：
```python
from geometry_msgs.msg import PointStamped
m = PointStamped()
print([a for a in dir(m) if not a.startswith('_')])
# ['header', 'point', 'serialize', ...]   ← 没有 pose
```

## rospy.Rate 在 sim time 未初始化时会崩

`use_sim_time=true` 但 `/clock` 尚未发布时：
```python
rate = rospy.Rate(5.0)   # ❌ 构造时读 rospy.Time.now() → ROSInitException
rate.sleep()             # ❌ 同样崩
```

**正确做法**：对采集/轮询类脚本用 `time.sleep()`（wall clock），
只在需要与仿真时钟对齐时才用 `rospy.Rate`。

**注意**：这是采集脚本的问题，不是产品代码的——`live_bridge.py` 用 `rospy.Rate`
是合理的（它在 ROS 已就绪的环境里跑）。
