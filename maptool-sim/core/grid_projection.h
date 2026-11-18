#pragma once
// T1.3: 点云加载 + 2D 障碍投影（纯逻辑，无 Qt/ROS 依赖）
// 参考 Map_ext_tool 思路：高度过滤 → 网格化占据
#include <string>
#include <vector>
#include <cstdint>
#include <cmath>
#include <stdexcept>

namespace blz {

struct Vec3 { float x, y, z; };

// 最小 PCD 读取器：支持 DATA ascii / binary（FIELDS 必须含 x y z）
std::vector<Vec3> load_pcd(const std::string& path);

// 2D 占据网格：世界坐标 → 网格索引；只保留 z ∈ [zmin, zmax] 的点
struct GridProjection {
    double resolution = 0.4;   // 米/格
    double min_x = 0, min_y = 0;
    int width = 0, height = 0; // 格数
    std::vector<uint8_t> occupied; // 1 = 有障碍
    size_t point_count = 0;    // 参与投影的点数（诊断用）

    bool occupied_at(double wx, double wy) const {
        if (width <= 0 || height <= 0) return false;
        // epsilon 容差：点坐标以 float 存储，查询常以 double 字面量，
        // 恰在网格边界时差 1 ulp 会导致 floor 出界
        constexpr double kEps = 1e-6;
        const double fx = (wx - min_x) / resolution + kEps;
        const double fy = (wy - min_y) / resolution + kEps;
        if (fx < 0 || fy < 0) return false;
        const int cx = static_cast<int>(std::floor(fx));
        const int cy = static_cast<int>(std::floor(fy));
        if (cx >= width || cy >= height) return false;
        return occupied[static_cast<size_t>(cy) * width + cx] != 0;
    }
    // 光线行进步长（对角线一半，保证不穿格）
    double step() const { return resolution * 0.7071 * 0.5; }
};

GridProjection project_to_grid(const std::vector<Vec3>& pts,
                               double resolution, double zmin, double zmax);

} // namespace blz
