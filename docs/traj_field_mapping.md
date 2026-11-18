# T0.4 轨迹属性映射表 v1（MapTool JSON → 任务文件）

> 状态：v1 草案（2026-11-18）。目的：MapTool 导出的轨迹 JSON 直接透传 `custom_path_node`，仅少数字段需要换算/映射。映射表放桥接层，便于迭代；**未知字段原样透传，绝不丢弃**。

## 1. 坐标换算

MapTool 轨迹点自带双坐标（南岸晶都样例实测）：

- `pixel_x/pixel_y`：pgm 像素坐标
- `x/y/yaw`：世界坐标（米/弧度）——**透传时直接用 x/y/yaw，不做二次换算**
- 一致性抽查（换算公式 vs 已有字段）：`x = origin.x + pixel_x * resolution`（参数来自场地档案 `map_pose_ref`）；抽 10 点误差 < 0.01 m 则档案 map_pose_ref 正确（C3/C4 校验的补充）

## 2. 字段映射表（白名单）

### 2.1 直通字段（JSON 字段名 = 任务参数语义，值原样）

| 字段 | 类型 | 语义 | 备注 |
|---|---|---|---|
| x, y, yaw | float | 轨迹点世界坐标 | 唯一参与规划的几何 |
| back | 0/1 | 倒车段 | |
| bypass | 0/1 | 自动绕障 | |
| charge | 0/1 | 充电点 | |
| clean_flag | 0/1 | 清扫标志 | |
| curve_paths | 0/1 | 曲线过渡 | |
| force_pass | 0/1 | 强制通过（忽略临时障碍） | |
| ignore_obstacles | 0/1 | 忽略障碍 | |
| localization_mode | int | 定位模式 | |
| parking_area | 0/1 | 停车区 | |
| reverse_traj | 0/1 | 反向轨迹 | |
| road_id | int | 路段 ID | |
| road_width | float | 轨迹宽度 | |
| seismic | 0/1 | 震动 | |
| trash | 0/1 | 垃圾 | |
| turnAround | 0/1 | 掉头点 | |
| bush_cutwidth | float | 灌木修剪宽 | |

### 2.2 换算/丢弃字段

| 字段 | 处理 | 原因 |
|---|---|---|
| pixel_x, pixel_y | **不透传**（规划端无用） | 仅 MapTool 显示用；换算抽查见 §1 |

### 2.3 待现场确认（v1.1 迭代补）

| 字段 | 疑问 |
|---|---|
| bush_cutwidth 语义与单位 | 样例全 0，需实测样本 |
| road_width=0 的默认行为 | 是"用全局默认宽"还是 0 宽，需对 yaml/源码 |
| localization_mode 的取值枚举 | 样例全 0 |

## 3. 参数覆盖（param_overrides，run 级）

轨迹 JSON 之外的参数走 `POST /runs` 的 `param_overrides`（键值对）→ bridge 生成参数集副本（命名 `paramsets/run_{run_id}.yaml`）注入，不改 `user_config`。首版开放的白名单参数：

`lookahead_dist`、`speed_gain`、`inflation_radius`、`publish_odom_imu`、`steering_tau`、`max_linear_vel`、前视探测距离/视锥角（话题补发后加入）

其余键一律拒绝（422 PARAM_NOT_ALLOWED），防止误改未验证参数。

## 4. 验收（T0.4 完成定义）

- [ ] 用南岸晶都「主路新轨迹，微调版.json」走一遍映射，输出与该场地已知可跑的任务文件逐字段一致（人工比对记录留档 `docs/mapping_check_20261118.md`）
- [ ] 未知字段透传：构造含假字段 `foo: 1` 的 JSON，转换后仍在
- [ ] `road_width=0`、`bush_cutwidth` 等 §2.3 疑问项对完源码/现场后更新本表为 v1.1
