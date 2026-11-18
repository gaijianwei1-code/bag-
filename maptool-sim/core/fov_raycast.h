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

} // namespace blz
