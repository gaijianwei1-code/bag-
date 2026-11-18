// 复现器：模拟用户操作序列，定位 T1.5 崩溃与"无显示"问题
// 用法：blz_repro <pcd路径>
#include "fov_layer.h"
#include "../../core/grid_projection.h"
#include "../../core/fov_raycast.h"
#include "../items/GraphicsType.h"
#include <QApplication>
#include <QGraphicsScene>
#include <cstdio>
#include <vector>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    printf("=== 复现器启动 ===\n"); fflush(stdout);
    if (argc < 2) { printf("需要 pcd 路径\n"); return 2; }

    auto* scene = new BQGraphicsScene;
    auto* layer = new FovLayer;
    layer->attachScene(scene);
    printf("[1] FovLayer attachScene OK\n"); fflush(stdout);

    auto* path = new BPathItem;
    scene->addItem(path);
    for (int i = 0; i < 5; ++i) path->addNextPoint(QPointF(300 + i * 40, 400 + i * 20));
    printf("[2] 轨迹点数=%d\n", path->pointsNumber()); fflush(stdout);

    layer->setMapInfo(0.05, -10.0, -8.0, 1000);
    printf("[3] setMapInfo OK\n"); fflush(stdout);

    QList<BPathItem*> paths; paths.push_back(path);
    layer->setPathList(paths);
    printf("[4] setPathList OK\n"); fflush(stdout);

    std::vector<blz::Vec3> cloud;
    try {
        cloud = blz::load_pcd(argv[1]);
        printf("[5] pcd 读取 %zu 点\n", cloud.size()); fflush(stdout);
    } catch (const std::exception& e) { printf("[5] pcd 失败: %s\n", e.what()); return 3; }

    auto grid = blz::project_to_grid(cloud, 0.4, 0.05, layer->zmax());
    printf("[6] grid %dx%d point_count=%zu\n", grid.width, grid.height, grid.point_count); fflush(stdout);

    layer->setGrid(&grid);
    printf("[7] setGrid OK\n"); fflush(stdout);

    printf("[8] 即将 setEnabled(true) ...\n"); fflush(stdout);
    layer->setEnabled(true);
    printf("[9] setEnabled(true) 返回，未崩溃；命中数=%d\n", layer->hitCount()); fflush(stdout);

    delete layer;
    delete scene;
    printf("=== 正常结束 ===\n");
    return 0;
}
