# T0.2 场地档案 Schema（venue registry v1）

> 状态：v1 草案（2026-11-18）。用途：把"每个 bag 都有已知的地图和轨迹"的配对关系系统化，杜绝 W16 那类"加载错地图但日志显示成功"。
> 存储：sim-bridge 数据目录 `sim-bridge/data/venues/{venue_id}.yaml`；MapTool 向导只负责生成/编辑，不直接读 ROS。

## 1. 档案样例

```yaml
schema: venue/v1
venue_id: nana-jingdu-01
name: 南岸晶都
created: 2026-11-18
created_by: user            # 用户在向导里录入（既定决定 3）
map_pgm: maps/nana/map.pgm      # 相对 bridge 数据根，显示用
map_pcd: maps/nana/map.pcd      # 点云，L0 快算 + NDT/虚拟雷达用
pcd_height_filter: {min: 0.05, max: 2.0}   # L0 投影的高度过滤（米，可调）
default_traj: trajs/主路新轨迹.json
map_pose_ref:                    # pgm 像素 ↔ 世界 坐标换算参数（MapTool 打开地图即知）
  origin: {x: -10.0, y: -8.0}
  resolution: 0.05
bags:
  - bag: bags/nana_20261117_351s.bag
    traj: trajs/主路新轨迹，微调版.json
    duration_s: 351
    notes: 182° 转向样本
    # init_pose 不在此录入：提交 run 时从 bag 起始时刻 /ndt/current_pose 自动提取（既定决定 2）
```

## 2. 字段定义与校验规则（向导即时校验 + bridge 提交前再校验）

| # | 规则 | 报错文案约定 |
|---|---|---|
| C1 | `venue_id`：小写字母数字与连字符，全局唯一 | 「场地 ID 已存在」 |
| C2 | map_pgm / map_pcd / traj / bag 文件必须存在 | 「文件不存在：{path}」 |
| C3 | pcd 与 pgm 坐标范围相交（用 map_pose_ref 换算后比较包围盒） | 「点云与地图坐标不匹配，请检查 map_pose_ref」 |
| C4 | 每条 traj 的点换算到世界坐标后必须落在 pgm 范围内 | 「轨迹越界：{traj} 第 {n} 点」 |
| C5 | bag 必须在档案 bags[] 里登记才能被 run 引用 | 「bag 未在场地档案登记：{bag}」 |
| C6 | resolution ∈ (0, 0.2]；height_filter min ≥ 0（排除地面） | 「参数超出合理范围」 |

## 3. run 提交时的强制检查（bridge 侧）

1. `venue_id` 必填且档案存在（否则 404 VENUE_NOT_FOUND）
2. 所选 bag ∈ 档案 bags[]（否则 422 BAG_NOT_IN_VENUE）→ **故意选错场地提交被拒（V4 验收）**
3. 从 bag 起始时刻提取 `init_pose`，提取失败（无 `/ndt/current_pose` 前 5 s 数据）→ 422 INIT_POSE_MISSING，不跑
4. map_dir 等参数一律从档案取，**不接受 run 请求里的 map 覆盖**（消灭硬编码通道）

## 4. 初始位姿自动提取规范

- 来源：bag 内 `/ndt/current_pose` 第一条有效消息（起点 5 s 内取中位值以抗抖动）
- 提取值写进 run manifest 的 `start_pose`（审计）；不从档案读取，档案里也**不存** init_pose 字段
- 已知验证点：76 s bag 场地提取值应与 W16 修复后的手动值一致（T2.2 验收项）

## 5. 录入流程（MapTool 向导，T1.2 实现范围）

三步，全程点选：
1. 选 pgm（现有"打开"流程复用）→ 自动读 resolution/origin（yaml 伴随文件）填 map_pose_ref
2. 选 pcd → 后台校验 C3 → 后台预生成 2D 投影缓存
3. 逐条添加 bag↔traj 配对（文件选择器 + C2/C4 即时校验）→ 保存
