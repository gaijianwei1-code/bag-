// T1.4 实现：视锥 raycast
#include "fov_raycast.h"
#include <cmath>
#include <algorithm>

namespace blz {

namespace {
inline bool blocked(const GridProjection& g, double wx, double wy) {
    return g.occupied_at(wx, wy);
}
} // namespace

FovResult cast_fov(const GridProjection& grid, double x, double y, double yaw,
                   const FovParams& p) {
    FovResult r;
    if (grid.width <= 0 || grid.height <= 0) return r;

    const double step = grid.step();
    const int rays = 13; // 视锥内 13 条射线（-half..+half），约 7.5° 分辨率
    double nearest = 1e18;
    bool any_hit = false;

    for (int i = 0; i < rays; ++i) {
        const double a = yaw + p.half_angle * (2.0 * i / (rays - 1) - 1.0);
        const double dx = std::cos(a), dy = std::sin(a);
        const double max_dist = std::max(p.front_dist, p.hit_dist_max);
        for (double d = step; d <= max_dist; d += step) {
            const double wx = x + dx * d, wy = y + dy * d;
            if (blocked(grid, wx, wy)) {
                any_hit = true;
                if (d < nearest) nearest = d;
                if (r.hits.size() < 64) r.hits.push_back({static_cast<float>(wx), static_cast<float>(wy), 0.f});
                break;
            }
        }
    }

    if (any_hit) {
        r.hit = true;
        r.first_hit_dist = nearest;
    } else {
        // 中心射线 free 行进距离
        const double dx = std::cos(yaw), dy = std::sin(yaw);
        double d = step;
        for (; d <= p.front_dist; d += step) {
            if (blocked(grid, x + dx * d, y + dy * d)) break;
        }
        r.free_dist = std::min(d, p.front_dist);
    }
    return r;
}

} // namespace blz
