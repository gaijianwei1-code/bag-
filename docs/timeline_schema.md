# T0.1 时间线 JSON Schema（timeline.schema v1）

> 状态：v1 草案（2026-11-18）。用途：sim-bridge WS 实时推送（L1 直播）与 MapTool 边收边存落盘共用同一格式，事后时间轴回放/逐帧拖动零额外成本。
> 校验：`timeline.schema.json`（JSON Schema Draft 2020-12，样例 `timeline_sample.json`）。

## 1. 顶层结构

```json
{
  "schema": "timeline/v1",
  "run_id": "nana-01_v2_20261118_154300",
  "venue_id": "nana-jingdu-01",
  "bag": "bags/nana_20261117_351s.bag",
  "branch": "develop",
  "commit": "1c01e2c",
  "param_set": "paramsets/run_nana-01_v2.yaml",
  "start_pose": {"x": 0.556, "y": 0.213, "yaw": -0.02},
  "frames": [ ... ],
  "metrics": { ... }
}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| schema | string | ✅ | 固定 `timeline/v1`，格式升级时变更 |
| run_id | string | ✅ | `{venue_id}_{备注}_{yyyymmdd}_{hhmmss}` |
| venue_id | string | ✅ | 引用场地档案（见 venue_schema） |
| bag / branch / commit / param_set | string | ✅ | 本次 run 的四大输入，manifest 同字段一致 |
| start_pose | object | ✅ | bag 起始时刻 `/ndt/current_pose` 自动提取值（审计用，run 时真实注入值以此为准） |
| frames | array | ✅ | 见 §2，直播期逐帧追加 |
| metrics | object | ⬜ | 跑完由 b_line_analyze 填入（8 项 F 门），直播中缺省 |

## 2. frames 逐帧字段

| 字段 | 类型 | 单位 | 来源话题 | 说明 |
|---|---|---|---|---|
| t | float | s | bag 时间戳 | 时间基准，单调不减 |
| x, y | float | m | `/ndt/current_pose` | 世界坐标（与场地 pgm 坐标系一致） |
| yaw | float | rad | `/ndt/current_pose` | 车头朝向 |
| speed | float | m/s | `/vehicle_cmd`（fake_chassis 反馈） | 标量速度 |
| lookahead | float | m | planner `lookahead_dist`（T3.1 补发前为估算值，字段 `lookahead_est: true` 标注） | |
| loc_score | float | 0–1 | 定位得分话题 | 无该话题的场地置 null |
| carrot | [x, y] \| null | m | planner carrot | 纯跟踪前视目标点 |
| front_hits | [[x,y], …] ≤50 | m | 前视探测命中（降采样，1–2 Hz 更新） | 直播时低频推送，回放时按帧插值 |
| err_lat | float ⬜ | m | analyze 侧后补 | 横向偏差（直播时无） |

**帧率约定**：WS 5–10 Hz 推 frame；`front_hits` 1–2 Hz 随最近一帧捎带。单帧 < 2 KB，10 Hz ≈ 20 KB/s。

## 3. metrics 字段（8 项 F 门，跑完填入）

```json
{
  "tracking_rms": 0.0085, "tracking_signed_mean": 0.0038,
  "planner_avg_dev": 0.0464, "theta_rms": 0.0221,
  "speed_diff_pct": -1.34, "min_clearance_diff": 0.012,
  "travel_diff_pct": -0.8, "collision_count": 0,
  "gates": "F 7/8"
}
```

字段名与 B 线 `compare.md` 口径一一对应；值为 null 表示该项未测。

## 4. 精度与取整

- 坐标/距离保留 4 位小数（0.1 mm 级，满足 V1 的 1 cm 重合校验）
- 角度保留 4 位小数；时间戳保留 3 位小数
- 一致性校验（V1）：MapTool 渲染轨迹与录制 `/ndt/current_pose` 逐点差 < 1 cm

## 5. 兼容与演进

- MapTool 收到未知字段**必须保留不丢弃**（落盘原样），渲染忽略
- v1 → v2 只允许加字段不改语义；解析按 `schema` 字段分支
