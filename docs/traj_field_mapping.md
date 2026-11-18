# T0.4 轨迹属性映射表 v1（MapTool JSON → 任务文件）

> 状态：v1 草案（2026-11-18）。目的：MapTool 导出的轨迹 JSON 直接透传 `custom_path_node`，仅少数字段需要换算/映射。映射表放桥接层，便于迭代；**未知字段原样透传，绝不丢弃**。

## 1. 坐标换算

MapTool 轨迹点自带双坐标（南岸晶都样例实测）：

- `pixel_x/pixel_y`：pgm 像素坐标
- `x/y/yaw`：世界坐标（米/弧度）——**透传时直接用 x/y/yaw，不做二次换算**
- 一致性抽查（换算公式 vs 已有字段）：`x = origin.x + pixel_x * resolution`（参数来自场地档案 `map_pose_ref`）；抽 10 点误差 < 0.01 m 则档案 map_pose_ref 正确（C3/C4 校验的补充）

## 2. 字段映射表（白名单）

### 2.1 直通字段（JSON 字段名 = 任务参数语义，值原样）

> **v1.1 修订（2026-11-18，源码实证）**：依据整车源码（`modules/sweep_path/custom_path/msg/TrajectoryAttribute.msg` 13 字段 + `custom_path.cpp` JSON 解析 + `sweep_base.cpp` 消费链）逐一核对。**车载实际消费的字段集与 MapTool JSON 并不一致**，详见 §2.2/§2.3。

**车载 `TrajectoryAttribute.msg` 13 字段（真正被规划/调度消费的集合）：**

| msg 字段 | MapTool JSON 键 | 语义 | 消费方（源码位置） |
|---|---|---|---|
| turnAround | turnAround | 掉头点 | sweep_base → turn_around_flag 话题 |
| **welt** | ⚠️ **JSON 缺失** | **贴边标记** | sweep_base is_sticking_（special_road 链） |
| bypass | bypass | 允许绕障 | sweep_base update_point_counts |
| **seisimic** | seismic（拼写差！） | 震尘 | msg 拼写为 seisimic |
| trash | trash | 垃圾点 | sweep_base trash_point_count |
| charge | charge | 充电桩 | sweep_base charge_point_count |
| road_id | road_id | 路段号/贴边路 | sweep_base special_road_ |
| back | back | 倒车段 | sweep_base back_point_count |
| curve_paths | curve_paths | 弯道标记 | sweep_base → curve_paths_flag |
| road_width | road_width | **不被消费**（见 §2.3） | — |
| localization_mode | localization_mode | 0=纯激光 1=融合(激光+RTK) 2=纯差分(RTK)，逐段切换 | sweep_base localization_mode_flag |
| **road_edge_distance** | ⚠️ **JSON 缺失** | 贴边距离（cm） | 当前分支未见消费方（grep 零命中，疑似预留） |
| clean_flag | clean_flag | 清扫/循迹 | sweep_base clean_flag_ |

**MapTool JSON 中车载不解析的字段**（`custom_path.cpp` 只解析上表 13 个键 + x/y/yaw 几何，其余全部无视）：`bush_cutwidth`、`force_pass`、`ignore_obstacles`、`parking_area`、`reverse_traj`、`seismic→seisimic` 拼写、`pixel_x/pixel_y`。

### 2.2 换算/丢弃字段

| 字段 | 处理 | 原因 |
|---|---|---|
| pixel_x, pixel_y | **不透传**（规划端无用） | 仅 MapTool 显示用；换算抽查见 §1 |

### 2.3 v1.2 源码核销（原疑问 3 项 + 贴边链路，2026-11-18 全部关闭）

**重要更正（v1.1 → v1.2）**：v1.1 曾判断"贴边标注传不到车上"——**错误**。用户提供的属性对话框截图证实：**贴边功能走 `road_id` 字段编码**，链路完整：

```
MapTool 属性对话框（road_id 单选：右贴边1 / 左贴边2 / (3) / (4) / 不贴边0 …）
  → JSON road_id
  → custom_path 透传 → sweep_base special_road_[i] = road_id
  → 目标点下发时 is_sticking_ = road_id（非 0 即贴边）
      ├─ adjustStickingGoal()：目标点向路沿偏移（1/3 右贴边 +、2/4 左贴边 −）
      └─ /special_road → obstacle_points edge_alignment_state_
            → 贴边侧点云裁剪（右贴边裁右侧/左贴边裁左侧，避免路沿被当障碍停障）
  → /special_road_sampler = road_id + special_road_width × 1000（宽度硬编码 400mm）
```

| 原疑问 | 结论（源码证据） |
|---|---|
| road_width=0 是默认宽还是零宽？ | 车载不消费轨迹里的 road_width：贴边采样宽度在 `sweep_base.cpp:645` 硬编码 `special_road_width[i]=400`（mm），UI 的"采样道路宽度(cm)"只影响 MapTool 显示/绘制。但 **road_id 编码了贴边语义且完整生效** |
| bush_cutwidth 单位？ | 车载无此字段（msg 13 字段无、全仓库无），属 MapTool"自定义属性"透传槽位，当前下发即丢 |
| localization_mode 枚举？ | **0=纯激光（NDT/HDL）1=融合（激光+RTK）2=纯差分（RTK）**；逐段切换；JSON 缺键默认 0（custom_path.cpp:326） |
| road_id 值域语义（新，截图+源码确认） | 0=不贴边；1=右贴边向左采样；2=左贴边向右采样；3/4=贴边不采样（右/左）；10=不贴边向左采样；20=不贴边向右采样。**is_sticking_ 非 0 即触发贴边目标点偏移 + 对应侧点云裁剪（1/3 查右侧、2/4 查左侧）** |

### 2.4 桥接层转换器必做项（T0.4.5）

1. `custom_path.cpp` 对 13 个 msg 键缺键时 `asInt()` 行为不安全 → 转换器**补齐全部 13 键缺省值**（缺省全 0）；
2. `seismic` → msg 字段 `seisimic`（车载拼写）改名映射；
3. `post_process` 须为布尔（W16 结论：false 跳过密度后处理，类型错会静默失败）；
4. `force_pass`/`ignore_obstacles`/`parking_area`/`reverse_traj`/`bush_cutwidth`：车载不解析，转换时**丢弃并在 UI 标注"仅本地"**，避免用户误以为生效；
5. ~~给 MapTool 加 welt/road_edge_distance~~（v1.2 撤销：贴边走 road_id，UI 已有）；`road_edge_distance`/`welt` 在当前分支无消费方，暂不透传。
6. **L0 前视快算联动**：road_id ∈ {1,2,3,4} 的段，前视渲染应叠加"贴边偏移预览"（目标点向路沿偏移 global_sticking_distance_）——贴边段的真实行车线 ≠ 轨迹线。

## 3. 参数覆盖（param_overrides，run 级）

轨迹 JSON 之外的参数走 `POST /runs` 的 `param_overrides`（键值对）→ bridge 生成参数集副本（命名 `paramsets/run_{run_id}.yaml`）注入，不改 `user_config`。首版开放的白名单参数：

`lookahead_dist`、`speed_gain`、`inflation_radius`、`publish_odom_imu`、`steering_tau`、`max_linear_vel`、前视探测距离/视锥角（话题补发后加入）

其余键一律拒绝（422 PARAM_NOT_ALLOWED），防止误改未验证参数。

## 4. 验收（T0.4 完成定义）

- [ ] 用南岸晶都「主路新轨迹，微调版.json」走一遍映射，输出与该场地已知可跑的任务文件逐字段一致（人工比对记录留档 `docs/mapping_check_20261118.md`）
- [ ] 未知字段透传：构造含假字段 `foo: 1` 的 JSON，转换后仍在
- [ ] `road_width=0`、`bush_cutwidth` 等 §2.3 疑问项对完源码/现场后更新本表为 v1.1
