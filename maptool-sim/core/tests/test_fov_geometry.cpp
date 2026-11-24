// 冒烟：前视检测区的几何口径（车身是否参与 / 裕度策略）
//
// 背景（2026-11-24 用户实机截图 + 南岸晶都 4134 点实测）：
//   1) 检测区原来从车尾 -(半长+裕度) 起算，覆盖整个车身 → 贴墙行驶必然命中
//      车侧/车尾格，贴边清扫全程误报。planner 的 collision_time_* 只做前向
//      预测，从不检查车身两侧与后方，故应只扫车头前方。
//   2) 安全裕度默认 0.25m 是重复保守：那是 costmap 膨胀半径，规划器已经算过；
//      L0 是"净空排雷"，按真实车宽 1.50m 判即可。
//
// 实测对比（南岸晶都 1119主路.json，4134 点）：
//   旧：含车身 + 裕度 0.25  → 命中 1017 点（24.6%）
//   仅车头前方 + 裕度 0.25  → 命中  876 点（21.2%）
//   仅车头前方 + 裕度 0    → 命中  108 点（ 2.6%）  ← 当前口径
//
// 本测试用合成网格锁住几何口径，不依赖外部数据文件。

#include "grid_projection.h"
#include "fov_raycast.h"

#include <cmath>
#include <cstdio>
#include <vector>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m)                                                            \
    do {                                                                       \
        if (c) { ++g_pass; std::printf("ok:   %s\n", m); }                      \
        else { ++g_fail; std::printf("FAIL: %s (line %d)\n", m, __LINE__); }    \
    } while (0)

static constexpr double kHalfL = 0.925;   // 半长（车长 1.85）
static constexpr double kHalfW = 0.750;   // 半宽（车宽 1.50）

// 造一个 12m × 12m 的空网格
static blz::GridProjection makeGrid() {
    blz::GridProjection g;
    g.resolution = 0.05;
    g.min_x = -6.0;
    g.min_y = -6.0;
    g.width = 240;
    g.height = 240;
    g.occupied.assign(static_cast<size_t>(g.width) * g.height, 0);
    return g;
}

static void setOcc(blz::GridProjection& g, double wx, double wy) {
    const int cx = static_cast<int>(std::floor((wx - g.min_x) / g.resolution));
    const int cy = static_cast<int>(std::floor((wy - g.min_y) / g.resolution));
    if (cx < 0 || cy < 0 || cx >= g.width || cy >= g.height) return;
    g.occupied[static_cast<size_t>(cy) * g.width + cx] = 1;
}

// 在车体系 (lx, ly) 处放一段沿 x 的墙
static void wallAtLocalY(blz::GridProjection& g, double lx0, double lx1,
                         double ly, double yaw) {
    const double c = std::cos(yaw), s = std::sin(yaw);
    for (double lx = lx0; lx <= lx1; lx += g.resolution) {
        const double wx = lx * c - ly * s;
        const double wy = lx * s + ly * c;
        setOcc(g, wx, wy);
    }
}

// 车在原点朝向 +x
static blz::FovResult scanFrontOnly(const blz::GridProjection& g, double front,
                                    double margin, double yaw = 0.0) {
    blz::RectParams rp;
    rp.x_min = kHalfL;                       // 只从车头前沿起
    rp.x_max = kHalfL + front;
    rp.y_min = -(kHalfW + margin);
    rp.y_max =  (kHalfW + margin);
    rp.ref_x = kHalfL;                       // 距离以车头前沿为 0
    return blz::cast_rect(g, 0.0, 0.0, yaw, rp);
}

// 旧口径：含整个车身
static blz::FovResult scanWithBody(const blz::GridProjection& g, double front,
                                   double margin, double yaw = 0.0) {
    blz::RectParams rp;
    rp.x_min = -(kHalfL + margin);
    rp.x_max = kHalfL + front;
    rp.y_min = -(kHalfW + margin);
    rp.y_max =  (kHalfW + margin);
    rp.ref_x = kHalfL;
    return blz::cast_rect(g, 0.0, 0.0, yaw, rp);
}

int main() {
    // ---- 1. 空场必须无命中 ----
    {
        auto g = makeGrid();
        auto r = scanFrontOnly(g, 1.20, 0.0);
        CHECK(!r.hit, "空旷场地不报命中");
    }

    // ---- 2. 车身侧墙不该触发（核心回归）----
    {
        auto g = makeGrid();
        // 墙面贴在车体左侧 ly=+0.80（刚好在车宽外 0.05m）
        wallAtLocalY(g, -3.0, 3.0, kHalfW + 0.05, 0.0);
        auto front = scanFrontOnly(g, 1.20, 0.0);
        CHECK(!front.hit, "车侧贴墙但前方通畅 → 不报命中（旧口径会误报）");
        // 对照：旧版真实配置是「含整个车身 + 裕度 0.25」，
        // 此时 y_max 撑到 +1.00m，+0.80m 的侧墙被圈进来 → 命中。
        // 注意不能用裕度 0 做对照：那样 y_max=+0.75，墙在 0.80 反而在界外。
        auto body = scanWithBody(g, 1.20, 0.25);
        CHECK(body.hit, "对照：含车身 + 裕度0.25 的旧口径会把同一堵侧墙算成命中");
    }

    // ---- 3. 车尾后墙不该触发 ----
    {
        auto g = makeGrid();
        wallAtLocalY(g, -3.0, -3.0, 0.0, 0.0);   // 单点墙，放车尾
        setOcc(g, -(kHalfL + 0.10), 0.0);
        auto front = scanFrontOnly(g, 1.20, 0.0);
        CHECK(!front.hit, "车尾后方有障碍 → 不参与前视命中");
    }

    // ---- 4. 正前方障碍必命中，且距离以车头前沿为 0 ----
    {
        auto g = makeGrid();
        // 在车头前沿 +0.50m 处放一堵横向墙
        const double wall_lx = kHalfL + 0.50;
        for (double ly = -1.0; ly <= 1.0; ly += g.resolution)
            setOcc(g, wall_lx, ly);
        auto r = scanFrontOnly(g, 1.20, 0.0);
        CHECK(r.hit, "正前方 0.50m 有墙 → 命中");
        CHECK(std::fabs(r.first_hit_dist - 0.50) < 0.08,
              "命中距离以车头前沿为 0 起算（应≈0.50m）");
    }

    // ---- 5. 障碍在检测距离之外不命中 ----
    {
        auto g = makeGrid();
        const double wall_lx = kHalfL + 1.50;   // 超出 front=1.20
        for (double ly = -1.0; ly <= 1.0; ly += g.resolution)
            setOcc(g, wall_lx, ly);
        auto r = scanFrontOnly(g, 1.20, 0.0);
        CHECK(!r.hit, "障碍在 1.50m、检测距离 1.20m → 不命中");
    }

    // ---- 6. 裕度为 0 时，贴墙 5cm 的车侧不命中；裕度 0.25 撑到 +1.00m ----
    {
        auto g = makeGrid();
        wallAtLocalY(g, -3.0, 3.0, kHalfW + 0.05, 0.0);   // 墙在 +0.80m
        CHECK(!scanFrontOnly(g, 1.20, 0.00).hit, "裕度 0：车侧 5cm 外的墙不命中");
        // 裕度 0.25 → 检测边界 +1.00m，墙面 +0.80m 落在边界内，会命中
        CHECK(scanFrontOnly(g, 1.20, 0.25).hit,
              "裕度 0.25：边界撑到 +1.00m，+0.80m 的墙被圈进来（这就是旧口径误报的原因）");
        // 墙面移到 +1.05m（裕度 0.25 边界之外）
        auto g2 = makeGrid();
        wallAtLocalY(g2, -3.0, 3.0, kHalfW + 0.30, 0.0);
        CHECK(!scanFrontOnly(g2, 1.20, 0.25).hit,
              "裕度 0.25 边界 +1.00m，墙在 +1.05m 外 → 不命中");
    }

    // ---- 7. 掉头档 0.40m 比直线档 1.20m 报警更少（分档确实在起作用）----
    {
        auto g = makeGrid();
        const double wall_lx = kHalfL + 0.80;   // 前方 0.80m
        for (double ly = -1.0; ly <= 1.0; ly += g.resolution)
            setOcc(g, wall_lx, ly);
        CHECK(scanFrontOnly(g, 1.20, 0.0).hit, "直线档 1.20m：0.80m 处有墙 → 命中");
        CHECK(!scanFrontOnly(g, 0.40, 0.0).hit, "掉头档 0.40m：0.80m 处有墙 → 不命中");
    }

    // ---- 8. 斜向车头（yaw=90°）也要正确旋转 ----
    {
        auto g = makeGrid();
        const double yaw = M_PI / 2;
        const double wall_l = kHalfL + 0.60;
        // 沿车体 ly 方向铺墙（世界坐标要按 yaw 旋转）
        const double c = std::cos(yaw), s = std::sin(yaw);
        for (double ly = -1.0; ly <= 1.0; ly += g.resolution)
            setOcc(g, wall_l * c - ly * s, wall_l * s + ly * c);
        auto r = scanFrontOnly(g, 1.20, 0.0, yaw);
        CHECK(r.hit, "yaw=90° 时正前方 0.60m 的墙仍能命中");
        CHECK(std::fabs(r.first_hit_dist - 0.60) < 0.08,
              "yaw=90° 命中距离同样以车头前沿为 0");
    }

    std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
