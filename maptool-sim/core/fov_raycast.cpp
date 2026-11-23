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

FovResult cast_rect(const GridProjection& grid, double x, double y, double yaw,
                    const RectParams& p) {
    FovResult r;
    if (grid.width <= 0 || grid.height <= 0 || grid.resolution <= 0.0) return r;
    if (p.x_max <= p.x_min || p.half_width <= 0.0) return r;

    const double c = std::cos(yaw), s = std::sin(yaw);
    // 计算旋转矩形 4 个角点的世界坐标 AABB
    const double xs[4] = {p.x_min, p.x_max, p.x_max, p.x_min};
    const double ys[4] = {-p.half_width, -p.half_width, p.half_width, p.half_width};
    double wx_min = 1e18, wx_max = -1e18, wy_min = 1e18, wy_max = -1e18;
    for (int i = 0; i < 4; ++i) {
        const double wx = x + xs[i] * c - ys[i] * s;
        const double wy = y + xs[i] * s + ys[i] * c;
        if (wx < wx_min) wx_min = wx;
        if (wx > wx_max) wx_max = wx;
        if (wy < wy_min) wy_min = wy;
        if (wy > wy_max) wy_max = wy;
    }

    const double res = grid.resolution;
    const int cx0 = std::max(0, static_cast<int>(std::floor((wx_min - grid.min_x) / res)));
    const int cx1 = std::min(grid.width - 1, static_cast<int>(std::floor((wx_max - grid.min_x) / res)));
    const int cy0 = std::max(0, static_cast<int>(std::floor((wy_min - grid.min_y) / res)));
    const int cy1 = std::min(grid.height - 1, static_cast<int>(std::floor((wy_max - grid.min_y) / res)));
    if (cx0 > cx1 || cy0 > cy1) return r;

    struct HitCand {
        double dist;
        float wx, wy;
    };
    std::vector<HitCand> cands;
    const double ref_x = (p.ref_x > -1e8) ? p.ref_x : p.x_min;
    for (int cy = cy0; cy <= cy1; ++cy) {
        const double wy = grid.min_y + (cy + 0.5) * res;
        const size_t row = static_cast<size_t>(cy) * grid.width;
        for (int cx = cx0; cx <= cx1; ++cx) {
            if (!grid.occupied[row + cx]) continue;
            const double wx = grid.min_x + (cx + 0.5) * res;
            const double dx = wx - x, dy = wy - y;
            // 反变换到车体局部坐标系（严格限定在矩形框内，防止红点飘出框外）
            const double lx =  dx * c + dy * s;
            const double ly = -dx * s + dy * c;
            if (lx < p.x_min || lx > p.x_max) continue;
            if (std::fabs(ly) > p.half_width) continue;
            const double d = std::max(0.0, lx - ref_x);
            cands.push_back({d, static_cast<float>(wx), static_cast<float>(wy)});
        }
    }

    if (cands.empty()) {
        r.free_dist = p.x_max - p.x_min;
        return r;
    }
    std::sort(cands.begin(), cands.end(), [](const HitCand& a, const HitCand& b) {
        return a.dist < b.dist;
    });
    r.hit = true;
    r.first_hit_dist = cands.front().dist;
    const int lim = std::min(static_cast<int>(cands.size()), std::max(1, p.max_hits));
    r.hits.reserve(lim);
    for (int i = 0; i < lim; ++i)
        r.hits.push_back({cands[i].wx, cands[i].wy, 0.f});
    return r;
}

} // namespace blz
