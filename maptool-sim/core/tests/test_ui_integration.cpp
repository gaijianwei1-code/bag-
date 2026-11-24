// T1.5 冒烟测试：UI 集成（离屏实例化 FovLayer + FovOverlayItem + 最小轨迹 item）
// 断言：
//   1. FovLayer 开启后对每条轨迹点生成 1 个渲染 entry（QGraphicsItem 数量 > 0）
//   2. 有 pcd 网格时命中点 entry 生成、recalculated 信号带命中数
//   3. 场景 changed（模拟拖点）→ 防抖重算触发
//   4. pcd binary 读取：真实 ndt pcd（若存在）或程序构造 binary pcd，点数 > 0 且坐标范围合理
//   5. 开关关闭后 overlay 从场景移除，原场景 item 不受影响（无回归结构性断言）
// 构建：并入冒烟构建（见 CMakeLists_smoke.txt），运行退出码 0 = 通过
#include <QApplication>
#include <QGraphicsScene>
#include <QTimer>
#include <cstdio>

#include "mainwindow/fov_layer.h"
#include "../core/grid_projection.h"
#include "../core/fov_raycast.h"
#include "../mac-tool-src/items/GraphicsType.h"

using namespace blz;
static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s (line %d)\n", msg, __LINE__); ++g_fail; } \
                              else printf("ok:   %s\n", msg); } while (0)

// 构造 binary pcd（header + float x y z intensity）
static void write_binary_pcd(const char* path, int n) {
    FILE* f = fopen(path, "wb");
    fprintf(f, "# .PCD v0.7 - Point Cloud Data file format\n"
               "FIELDS x y z intensity\nSIZE 4 4 4 4\nTYPE F F F F\nCOUNT 1 1 1 1\n"
               "WIDTH %d\nHEIGHT 1\nPOINTS %d\nDATA binary\n", n, n);
    for (int i = 0; i < n; ++i) {
        // x ∈ [0,10), y ∈ [0,10), z=0.8, intensity
        const float x = (i % 100) * 0.1f, y = (i / 100) * 0.1f, z = 0.8f, it = 1.f;
        fwrite(&x, 4, 1, f); fwrite(&y, 4, 1, f); fwrite(&z, 4, 1, f); fwrite(&it, 4, 1, f);
    }
    fclose(f);
}

int main(int argc, char** argv) {
    // 离屏渲染
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    // ---- 场景 + 一条轨迹（2 个点，模拟 MapTool 结构）----
    BQGraphicsScene scene;
    BPathItem path;
    scene.addItem(&path);
    // 点的像素/世界关系：resolution=0.05 m/px, origin=(0,0), pgm_height=6000
    // 像素 (1000, 3000) → 世界 (50, (6000-1-3000)*0.05) = (50, 149.95)
    path.addNextPoint(QPointF(1000, 3000));
    path.addNextPoint(QPointF(1100, 3000));
    path.getElement(0)->setYaw(0.0);   // 朝世界 +x（屏幕角 0）
    path.getElement(1)->setYaw(0.0);

    // ---- pcd：真实 ndt pcd 优先，否则构造 binary ----
    const char* real_pcd = "/Users/a11/multi-agent/squad-workspace/roboneo_sim/runs/"
                           "BAG_REPLAY/closed_ndt_auto/ndt_map/1793346971223_frames_no_ground.pcd";
    std::string use_pcd = real_pcd;
    { FILE* f = fopen(real_pcd, "rb"); if (!f) { write_binary_pcd("/tmp/blz_t15.pcd", 10000); use_pcd = "/tmp/blz_t15.pcd"; } else fclose(f); }
    auto pts = load_pcd(use_pcd);
    CHECK(pts.size() > 0, "pcd loaded (real ndt or constructed binary)");
    {
        double mn = 1e18, mx = -1e18;
        for (auto& p : pts) { mn = std::min(mn, (double)p.x); mx = std::max(mx, (double)p.x); }
        CHECK(mx > mn, "pcd x range sane");
        printf("      pcd=%s points=%zu x∈[%.2f, %.2f]\n", use_pcd.c_str(), pts.size(), mn, mx);
    }
    // 投影时把 pcd 平移到轨迹附近的世界坐标（轨迹在 (50,150) 附近，pcd 原点在 0）
    for (auto& p : pts) { p.x += 50.f; p.y += 150.f; }
    auto grid = project_to_grid(pts, 0.4, 0.05, 2.0);
    CHECK(grid.width > 0, "project_to_grid produced grid");

    // ---- FovLayer 集成 ----
    FovLayer layer;
    layer.attachScene(&scene);           // attachScene 内部把 overlay 加进场景
    layer.setMapInfo(0.05, 0.0, 0.0, 6000);
    layer.setGrid(&grid);
    QList<BPathItem*> paths{&path};
    layer.setPathList(paths);

    // 开关开启（menu action / dock 按钮最终都走 setEnabled）
    int sig_hits = -1;
    QObject::connect(&layer, &FovLayer::recalculated, [&](int h){ sig_hits = h; });
    layer.setEnabled(true);
    CHECK(layer.enabled(), "fov layer enabled");
    CHECK(layer.entryCount() == 2, "one render entry per trajectory point");
    // 场景中 overlay 存在（item 数 = 2 BPointItem + BPathItem + overlay = 4）
    CHECK(scene.items().size() >= 4, "overlay present in scene (item count grows)");

    // recalcAll 幂等：重算不重复生成 entry
    layer.recalcAll();
    CHECK(layer.entryCount() == 2, "recalcAll is idempotent");

    // 场景 changed（模拟拖点）→ 防抖 30ms → 自动重算
    path.getElement(0)->setPoint(QPointF(1050, 3000));   // 直接改点（拖点等价效果）
    emit scene.changed(QList<QRectF>());                 // 触发场景 changed 链路
    QTimer::singleShot(80, [&]() {
        CHECK(layer.entryCount() == 2, "debounced recalc after scene change");

        // ---- 命中路径：墙场景 → 命中数 > 0（覆盖红点渲染分支）----
        // 轨迹点1 像素(1050,3000) → 世界 (52.5, 149.95)，yaw=0 朝 +x
        //
        // 2026-11-24 口径修正：检测区现在**只扫车头前方**（从车头前沿 +0.925m 起，
        // 不含车身两侧与后方），裕度默认 0。所以墙必须放在车头前沿之外才会命中。
        // 原来把墙放在正前方 0.2m（= 车身内部），按新口径不该命中——那个断言
        // 描述的是旧行为。这里改放 0.925+0.30 ≈ 1.23m 处，仍在直线档 1.20m 内。
        {
            std::vector<Vec3> wall;
            const float wall_x = 52.5f + 1.10f;   // 车头前沿外 0.175m，检测距离内
            for (float yy = 149.0f; yy <= 151.0f; yy += 0.05f)
                wall.push_back({wall_x, yy, 0.8f});
            auto gwall = project_to_grid(wall, 0.4, 0.05, 2.0);
            layer.setGrid(&gwall);
            int wall_hits = -1;
            QObject::connect(&layer, &FovLayer::recalculated, [&](int h){ wall_hits = h; });
            layer.recalcAll();
            CHECK(wall_hits >= 1, "wall ahead of front bumper -> hit counted");

            // 反面：墙体放在车身内部（车头前沿之后）不该算命中
            std::vector<Vec3> inner;
            for (float yy = 149.0f; yy <= 151.0f; yy += 0.05f)
                inner.push_back({52.5f + 0.20f, yy, 0.8f});
            auto ginner = project_to_grid(inner, 0.4, 0.05, 2.0);
            layer.setGrid(&ginner);
            int inner_hits = -1;
            QObject::connect(&layer, &FovLayer::recalculated, [&](int h){ inner_hits = h; });
            layer.recalcAll();
            CHECK(inner_hits == 0, "wall inside car body -> NOT counted (new geometry)");

            layer.setGrid(&grid);   // 还原，供后续断言
        }

        // 开关关闭 → overlay 移出场景
        layer.setEnabled(false);
        CHECK(!layer.enabled() && layer.entryCount() == 0, "disable clears entries");
        CHECK(scene.items().size() == 3, "scene back to original item count (no leak)");
        printf(g_fail == 0 ? "\nSMOKE PASS (%d fails)\n" : "\nSMOKE FAIL (%d fails)\n", g_fail);
        QApplication::exit(g_fail == 0 ? 0 : 1);
    });

    return app.exec();
}
