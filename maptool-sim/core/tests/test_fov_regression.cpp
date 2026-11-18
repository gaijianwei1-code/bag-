// T1.5-bugfix 回归冒烟：锁定"场景 clear 后图层悬垂崩溃"与"z 值被地图盖住"两个真 bug
// 用法：blz_regress <pcd路径>；退出码 0 = 通过
#include "fov_layer.h"
#include "../../core/grid_projection.h"
#include "../items/GraphicsType.h"
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <cstdio>
#include <vector>

static int g_fail = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); ++g_fail; } else printf("ok:   %s\n", m); } while (0)

static BPathItem* makePath(QGraphicsScene* sc) {
    auto* p = new BPathItem; sc->addItem(p);
    for (int i = 0; i < 5; ++i) p->addNextPoint(QPointF(300 + i * 40, 400 + i * 20));
    return p;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    if (argc < 2) { printf("need pcd path\n"); return 2; }

    auto* scene = new BQGraphicsScene;
    // 模拟地图图片（z 默认 0）
    auto* pix = scene->addPixmap(QPixmap(1000, 1000));
    pix->setZValue(0);
    auto* layer = new FovLayer;
    layer->attachScene(scene);

    auto* path = makePath(scene);
    layer->setMapInfo(0.05, -10.0, -8.0, 1000);
    QList<BPathItem*> paths; paths.push_back(path);
    layer->setPathList(paths);

    std::vector<blz::Vec3> cloud = blz::load_pcd(argv[1]);
    auto grid = blz::project_to_grid(cloud, 0.4, 0.05, layer->zmax());
    layer->setGrid(&grid);

    // --- 用例 1：正常开启（修复前此处正常；作为基线）---
    layer->setEnabled(true);
    CHECK(layer->hitCount() > 0, "有 pcd 时应命中（几何链路通）");

    // --- 用例 2：复现崩溃场景 —— 场景 clear() 后再次开启 ---
    // 用户实际操作：打开新 pgm → cleanImage() → pgmScene->clear()
    printf("[场景] 模拟 pgmScene->clear()（打开新图）...\n"); fflush(stdout);
    layer->setEnabled(false);
    scene->clear();                       // 删除所有 item（含 overlay 与 path）
    printf("[场景] clear 完成，重建轨迹并再次开启\n"); fflush(stdout);
    auto* path2 = makePath(scene);
    QList<BPathItem*> paths2; paths2.push_back(path2);
    layer->setPathList(paths2);
    layer->setEnabled(true);              // 修复前：访问已释放 m_overlay → 崩溃
    CHECK(true, "场景 clear 后重新开启未崩溃（悬垂 bug 已修）");

    // --- 用例 3：z 值必须高于地图（否则被盖住看不见）---
    // 注意：scene->clear() 已 delete 上面的 pix，必须查当前场景里新的地图 item
    double mapZ = 0.0;
    for (QGraphicsItem* it : scene->items())
        if (it->type() == QGraphicsPixmapItem::Type) { mapZ = it->zValue(); break; }
    CHECK(layer->overlayZValue() > mapZ,
          "图层 z 值高于地图 pixmap（否则扇形不可见）");
    CHECK(layer->overlayZValue() < 10.0, "图层 z 值不过高（不遮挡工具栏/选区）");

    // --- 用例 4：重复开关无泄漏 ---
    for (int i = 0; i < 5; ++i) { layer->setEnabled(false); layer->setEnabled(true); }
    CHECK(true, "连续开关 5 次未崩溃");

    // --- 用例 5：无 pcd 时开启不崩溃（空 grid）---
    layer->setGrid(nullptr);
    layer->setEnabled(false);
    layer->setEnabled(true);
    CHECK(true, "无点云时开启不崩溃");

    delete layer; delete scene;
    printf(g_fail ? "\nREGRESS FAIL (%d)\n" : "\nREGRESS PASS\n", g_fail);
    return g_fail ? 1 : 0;
}
