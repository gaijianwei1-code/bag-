# 标准测试样本（冒烟用）

## scenario_1793346971223 —— 76 秒单场地回归样本

**来源**：用户 2026-11-18 提供（原 `/Users/a11/Downloads/scenario_1793346971223`，对应 B 线默认 bag 场地）
**用途**：所有 UI/桥接冒烟的唯一标准输入，禁止再临时找文件。

| 文件 | 说明 | 是否入 git |
|---|---|---|
| `1793346971223.yaml` | 地图元数据：resolution 0.05、origin [-22.797, -77.407] | ✅ |
| `1793346971223.pgm` | 栅格地图 8.4 MB（MapTool 打开） | ✅ |
| `1793347176294.json` | 轨迹 50 点，含全部 22 个属性字段 | ✅ |
| `1793346971223_frames_no_ground.pcd` | 障碍点云 147,448 点（**L0 前视快算输入**） | ❌ 3.8 MB，从 Downloads 取 |
| `1793346971223_frames.pcd` | 含地面点云 5.4 MB（对照用） | ❌ |
| `1.000000_ndt_model.pcd` | NDT 体素模型 3,219 点（**不要用于前视**） | ❌ |

### 该样本的既知事实（源码+实测核对）
- 轨迹 world 范围 x[0.1, 25.7] y[-1.1, 0.0]；pixel 起点 (458, 1554)
- **road_id = 1（右贴边、向左采样）全 50 点** → 该轨迹是贴边轨迹，L0 渲染应体现出贴边偏移语义
- `bypass = 1` 全段（允许绕障）；`curve_paths = 0`（无弯道标记）
- 其余属性（force_pass / bush_cutwidth / parking_area / reverse_traj / ignore_obstacles / road_edge_distance / next_goal_tolerance_override / road_width / localization_mode / seismic / trash / charge / turnAround / back / clean_flag）**全为 0 且全段一致**
- `post_process = 0`（整数，非布尔，非空对象）——印证 B 线 W16 的结论：必须是 0 才跳过密度后处理
- 点云 z 范围 [-1.82, 2.70]（雷达坐标系，雷达装高 0.8 m，地面约 -0.82）

### 冒烟标准命令
```bash
# core + UI 集成 + 前视回归（三件套一次跑完）
cd "/Users/a11/地图工具bag回放"
export PATH="$HOME/Documents/Codex/2026-10-12/n-h/work/toolchain/env/bin:$PATH"
# 见 docs/smoke/run_smoke.sh
```
