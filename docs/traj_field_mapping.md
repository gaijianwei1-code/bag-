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

### 2.3 v1.1 源码核销（原疑问 3 项，全部关闭）

| 原疑问 | 结论（源码证据） |
|---|---|
| road_width=0 是默认宽还是零宽？ | **都不是——车载不消费该字段**。贴边用的路宽在 `sweep_base.cpp:645` 硬编码 `special_road_width[i]=400`（mm，编码时 ×1000 进 road_msg_sample 下发给 sampler）。改轨迹 JSON 的 road_width 无效；要改贴边行为得走参数集/源码 |
| bush_cutwidth 单位？ | **车载无此字段**（msg 与解析代码均无），MapTool 单方面字段，下发即丢。映射表标记为「仅 MapTool 本地」 |
| localization_mode 枚举？ | **0=纯激光（NDT/HDL）1=融合（激光+RTK）2=纯差分（RTK）**；逐段切换；JSON 缺键时 custom_path 默认 0（custom_path.cpp:326） |

### 2.4 ⚠️ 新发现的桥接层必做项（升级为 T0.4.5）

`custom_path.cpp` 解析 JSON 时**缺失键会 asInt() 崩溃或未定义**（如 welt/road_edge_distance 在现有 MapTool JSON 里根本不存在）。桥接层转换器必须：
1. 对 13 个 msg 字段**全部补齐缺省值**（welt=0、road_edge_distance=0、seismic→seisimic 改名）再交给 custom_path；
2. `post_process` 键处理与 B 线 W16 结论一致（须为布尔，false 时跳过密度后处理）；
3. 「在 MapTool 里标贴边」功能需先给 MapTool 加 `welt`/`road_edge_distance` 标注 UI（新需求，与贴边回归场景强相关，报用户决策）。

## 3. 参数覆盖（param_overrides，run 级）

轨迹 JSON 之外的参数走 `POST /runs` 的 `param_overrides`（键值对）→ bridge 生成参数集副本（命名 `paramsets/run_{run_id}.yaml`）注入，不改 `user_config`。首版开放的白名单参数：

`lookahead_dist`、`speed_gain`、`inflation_radius`、`publish_odom_imu`、`steering_tau`、`max_linear_vel`、前视探测距离/视锥角（话题补发后加入）

其余键一律拒绝（422 PARAM_NOT_ALLOWED），防止误改未验证参数。

## 4. 验收（T0.4 完成定义）

- [ ] 用南岸晶都「主路新轨迹，微调版.json」走一遍映射，输出与该场地已知可跑的任务文件逐字段一致（人工比对记录留档 `docs/mapping_check_20261118.md`）
- [ ] 未知字段透传：构造含假字段 `foo: 1` 的 JSON，转换后仍在
- [ ] `road_width=0`、`bush_cutwidth` 等 §2.3 疑问项对完源码/现场后更新本表为 v1.1
