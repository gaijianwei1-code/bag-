// T1.3+T1.4 冒烟测试：grid_projection + fov_raycast
// 构建：见 smoke/CMakeLists.txt；运行退出码 0 = 通过
#include "grid_projection.h"
#include "fov_raycast.h"
#include <cstdio>#include <cstdlib>
#include <cmath>
#include <fstream>

using namespace blz;
static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s (line %d)\n", msg, __LINE__); ++g_fail; } \
                              else printf("ok:   %s\n", msg); } while (0)

int main(int argc, char** argv) {
    // ---- 1. 投影：构造 3 个点（一个被高度过滤掉）----
    std::vector<Vec3> pts = {{0.1f, 0.1f, 0.3f}, {5.0f, 0.2f, 1.0f}, {9.9f, 9.9f, 5.0f}};
    auto g = project_to_grid(pts, 0.4, 0.05, 2.0);
    CHECK(g.width > 0, "projection produced grid");
    CHECK(g.point_count == 2, "height filter kept 2 of 3 points");
    CHECK(g.occupied_at(0.1, 0.1), "point 1 occupied");
    CHECK(g.occupied_at(5.0, 0.2), "point 2 occupied");
    CHECK(!g.occupied_at(9.9, 9.9), "z=5.0 point filtered out");
    CHECK(!g.occupied_at(2.0, 2.0), "empty cell free");

    // ---- 2. raycast：无障碍 ----
    auto r0 = cast_fov(g, 3.0, 3.0, 0.0, {2.0, 0.5, 3.0});
    CHECK(!r0.hit, "no obstacle -> no hit");
    CHECK(std::abs(r0.free_dist - 2.0) < 0.01, "free to front_dist");

    // ---- 3. raycast：正前方 ~1m 有墙（占据一片格子，车在墙后 1m）----
    // 场地：车位于 (3.0, 3.0) 朝 +x，墙在 x=4.0 全行
    std::vector<Vec3> scene;
    for (float yy = -2.0f; yy <= 8.0f; yy += 0.2f) scene.push_back({4.0f, yy, 0.8f});
    auto g2 = project_to_grid(scene, 0.4, 0.05, 2.0);
    CHECK(g2.point_count == scene.size(), "wall scene fully projected");
    auto r1 = cast_fov(g2, 3.0, 3.0, 0.0, {2.0, 0.5, 3.0});
    CHECK(r1.hit, "wall ahead -> hit");
    CHECK(r1.first_hit_dist > 0.6 && r1.first_hit_dist < 1.4, "first hit ~1m");
    CHECK(!r1.hits.empty(), "hit points recorded");

    // ---- 4. raycast：障碍在正后方（yaw 背对）不应命中 ----
    auto r2 = cast_fov(g2, 3.0, 3.0, M_PI, {2.0, 0.5, 3.0});
    CHECK(!r2.hit, "wall behind (yaw=pi) -> no hit");

    // ---- 5. 参数敏感性：前视距离缩小后命中消失 ----
    auto r3 = cast_fov(g2, 3.0, 3.0, 0.0, {0.3, 0.5, 3.0});
    // hit_dist_max 仍 3.0，会命中；把 hit_dist_max 也收小
    auto r4 = cast_fov(g2, 3.0, 3.0, 0.0, {0.3, 0.5, 0.3});
    CHECK(!r4.hit, "shrink front+hit dist -> no early hit");

    // ---- 6. PCD 读取：写一个最小 ascii pcd 再读回 ----
    {
        std::ofstream f("/tmp/blz_smoke.pcd");
        f << "# .PCD v0.7 - Point Cloud Data file format\n"
          << "FIELDS x y z intensity\nPOINTS 3\nDATA ascii\n"
          << "0.1 0.1 0.3 99\n5.0 0.2 1.0 1\n9.9 9.9 5.0 2\n";
    }
    auto back = load_pcd("/tmp/blz_smoke.pcd");
    CHECK(back.size() == 3, "pcd ascii read 3 points");
    CHECK(std::abs(back[1].x - 5.0f) < 1e-5, "pcd x index correct (intensity field skipped)");
    CHECK(std::abs(back[1].z - 1.0f) < 1e-5, "pcd z correct");

    // ---- 7. 性能：10 万点投影 + 1000 次 raycast 基准 ----
    {
        std::vector<Vec3> big; big.reserve(100000);
        for (int i = 0; i < 100000; ++i) {
            float fx = static_cast<float>(i % 3000) * 0.01f;
            float fy = static_cast<float>(i % 7000) * 0.005f;
            big.push_back({fx, fy, 0.8f});
        }
        clock_t t0 = clock();
        auto gb = project_to_grid(big, 0.4, 0.05, 2.0);
        clock_t t1 = clock();
        for (int i = 0; i < 1000; ++i)
            cast_fov(gb, 1.0, 1.0, i * 0.001, {2.0, 0.5, 3.0});
        clock_t t2 = clock();
        double proj_ms = 1000.0 * (t1 - t0) / CLOCKS_PER_SEC;
        double cast_us = 1000.0 * (t2 - t1) / CLOCKS_PER_SEC; // 1000 次 → 微秒/次
        printf("bench: project(1e5 pts)=%.1f ms, raycast=%.1f us/cast\n", proj_ms, cast_us);
        CHECK(cast_us < 1000.0, "raycast < 1 ms/cast");
    }

    if (g_fail == 0) { printf("\nSMOKE PASS\n"); return 0; }
    printf("\nSMOKE FAIL (%d)\n", g_fail);
    return 1;
}
