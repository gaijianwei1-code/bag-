#pragma once
// T1.4: L0 前视快算 —— 视锥 raycast（纯几何，无 Qt/ROS 依赖）
#include "grid_projection.h"

namespace blz {

struct FovParams {
    double front_dist = 2.0;   // 前视距离（米）
    double half_angle = 0.5;   // 视锥半角（rad）
    double hit_dist_max = 3.0; // 命中报告距离上限（米）
};

struct FovResult {
    bool hit = false;                     // 视锥内是否命中障碍
    double first_hit_dist = -1.0;         // 沿中心方向首个命中距离（-1 无命中）
    std::vector<Vec3> hits;               // 命中点（世界坐标，z 置 0）
    // 诊断：中心射线上未命中时返回 free 行进距离
    double free_dist = -1.0;
};

// 位姿 (x, y, yaw)：车头朝向 yaw。在占据网格上沿视锥内 N 条射线步进。
FovResult cast_fov(const GridProjection& grid, double x, double y, double yaw,
                   const FovParams& p);

// 实车矩形包络/前视检测框参数（局部车体系：+x 车头方向，+y 左侧）
struct RectParams {
    double x_min = -0.925;     // 纵向起点（米，车中心为 0；-0.925 为 1.85m 车尾，+0.925 为车头前沿）
    double x_max = 2.925;      // 纵向终点（米，如车头前沿 0.925 + 前视距离 2.0）
    double half_width = 0.75;  // 半宽（米，1.50m 车身半宽 = 0.75）
    int max_hits = 64;         // 最多返回命中点数（按距离最近优先）
    double ref_x = -1e9;       // 距离参考面（<= -1e8 时默认取 x_min；传 +0.925 表示以车头前沿为 0，车身内碰撞返回 0.0）
};

// 位姿 (x, y, yaw)：扫描旋转矩形 [x_min, x_max] × [-half_width, +half_width] 内的占据网格点。
// first_hit_dist 返回最近命中点相对 x_min 的纵向距离（>= 0）。
FovResult cast_rect(const GridProjection& grid, double x, double y, double yaw,
                    const RectParams& p);

} // namespace blz
