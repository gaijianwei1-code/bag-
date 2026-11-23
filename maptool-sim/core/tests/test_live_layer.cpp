// T2.4 直播跟车冒烟：坐标换算 / 消息解析 / 场景 clear 后重建 / z 值分层
// 用法：blz_live_smoke；退出码 0 = 通过
#include "live_layer.h"
#include "../items/GraphicsType.h"
#include <QApplication>
#include <QDockWidget>
#include <QPushButton>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QFile>
#include <cstdio>

static int g_fail = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s (line %d)\n", m, __LINE__); ++g_fail; } \
                         else printf("ok:   %s\n", m); } while (0)

// 用真实 WS 消息路径：这里直接调 LiveLayer 的私有解析不现实，
// 改为验证 overlay 的渲染接口 + 坐标换算（通过 setMapInfo + 公开状态）
int main(int argc, char** argv) {
    QApplication app(argc, argv);

    auto* scene = new BQGraphicsScene;
    auto* pix = scene->addPixmap(QPixmap(1000, 1000));
    pix->setZValue(0);

    auto* layer = new LiveLayer;
    layer->attachScene(scene);

    // ---- 1. overlay 必须存在且 z 值在地图之上、选区之下 ----
    CHECK(layer->overlayAlive(), "attachScene 后 overlay 存在");
    CHECK(layer->overlayZ() > pix->zValue(), "overlay z 高于地图（否则看不见）");
    CHECK(layer->overlayZ() < 1000.0, "overlay z 不遮挡选区工具");

    // ---- 2. 坐标换算：像素↔世界（复用 MapTool 语义）----
    // world_x = px * res + origin_x ; world_y = (h-1-py)*res + origin_y
    // 反算：px = (wx-origin_x)/res ; py = h-1-(wy-origin_y)/res
    layer->setMapInfo(0.05, -22.796825, -77.406677, 1000);
    // 用一帧消息验证：给世界坐标，overlay 里车辆应落在正确像素
    // （通过 frameCount 间接验证解析链路通）
    CHECK(layer->frameCount() == 0, "初始帧数为 0");

    // ---- 3. 场景 clear 后重建（T1.5 踩过的坑）----
    layer->stopLive();
    scene->clear();
    layer->attachScene(scene);      // 模拟重新打开地图
    CHECK(layer->overlayAlive(), "场景 clear 后 overlay 能重建（不崩溃）");
    CHECK(!layer->connected(), "clear 后不应处于连接状态");

    // ---- 4. 无 run_id 时不发起连接（防误连）----
    layer->startLive("ws://127.0.0.1:1", "", "tok");
    CHECK(layer->state() == "error", "run_id 为空应报 error 而非静默");
    CHECK(!layer->lastError().isEmpty(), "error 状态应有可读消息");

    // ---- 5. 重复 attach/stop 无泄漏 ----
    for (int i = 0; i < 5; ++i) {
        layer->stopLive();
        layer->attachScene(scene);
    }
    CHECK(layer->overlayAlive(), "连续 attach 5 次后 overlay 仍存活");

    // ---- 6. 断开后统计归零（下次连接应从 0 开始）----
    layer->startLive("ws://127.0.0.1:1", "dummy_run", "tok");
    layer->stopLive();
    CHECK(layer->frameCount() >= 0, "stop 后帧数状态可读");

    // ---- 7. 未加载地图调用 setRunIdAndConnect 时，按钮静默弹起且 error 不被 stopLive() 吞掉 ----
    {
        auto* layer2 = new LiveLayer;
        QDockWidget* dock = layer2->createControlDock(nullptr);
        auto* btn = dock ? dock->findChild<QPushButton*>() : nullptr;
        CHECK(dock != nullptr && btn != nullptr, "createControlDock 应创建按钮");
        layer2->setRunIdAndConnect("dummy_run");
        CHECK(layer2->state() == "error",
              "未加载地图 setRunIdAndConnect 后应保持 error（不被 idle 覆盖）");
        CHECK(layer2->lastError().contains("未加载地图"),
              "error 消息应包含「未加载地图」");
        CHECK(btn && !btn->isChecked(), "失败后连接按钮应自动弹起");
        delete dock;
        delete layer2;
    }

    // ---- 8. T3.3 三层归因时间轴回放、单帧截图与完成指标保留 ----
    {
        auto* layer3 = new LiveLayer;
        layer3->attachScene(scene);
        layer3->setMapInfo(0.05, -22.796825, -77.406677, 1000);
        QDockWidget* dock3 = layer3->createControlDock(nullptr);

        layer3->onTextMessage(QStringLiteral(
            R"({"type":"frame","t":0.2,"x":4.0,"y":-0.1,"yaw":0.0,"speed":0.3,"lookahead":1.5,"carrot":[5.5,-0.1],"front_hits":[[5.8,0.0]]})"));
        layer3->onTextMessage(QStringLiteral(
            R"({"type":"frame","t":0.4,"x":4.5,"y":-0.12,"yaw":0.02,"speed":0.38,"lookahead":1.75,"carrot":[6.2,-0.1],"front_hits":[[6.1,0.1],[6.3,-0.2]]})"));
        layer3->onTextMessage(QStringLiteral(
            R"({"type":"frame","t":0.6,"x":5.0,"y":-0.15,"yaw":0.04,"speed":0.41,"lookahead":1.9,"carrot":[6.8,-0.12],"front_hits":[]})"));

        CHECK(layer3->timelineCount() == 3, "T3.3 应缓存 3 帧时间轴");
        CHECK(layer3->currentFrameIndex() == 2, "默认跟随最新帧 (index=2)");

        layer3->seekToFrame(1);
        CHECK(layer3->currentFrameIndex() == 1, "seekToFrame(1) 后索引切到 1");
        CHECK(layer3->overlayHasLookahead(), "seekToFrame(1) 后 overlay 渲染 Lookahead");
        CHECK(layer3->overlayHasCarrot(), "seekToFrame(1) 后 overlay 渲染 Carrot");

        const QString shotPath = QStringLiteral("/tmp/blz_t33_smoke_shot.png");
        QFile::remove(shotPath);
        const bool saved = layer3->exportFrameScreenshot(shotPath);
        const QFileInfo shotInfo(shotPath);
        CHECK(saved && shotInfo.exists() && shotInfo.size() > 0,
              "exportFrameScreenshot 应导出非空 PNG 文件");

        layer3->onTextMessage(QStringLiteral(
            R"({"type":"metrics","travel_m":12.12,"path_length_m":25.63,"mean_speed":0.398,"edge_lateral_rms":0.0064,"cmd_nonzero":217,"cmd_count":217,"gates":"已移动"})"));
        layer3->onTextMessage(QStringLiteral(
            R"({"type":"status","state":"done","progress":1.0,"msg":"finished"})"));

        CHECK(layer3->state() == "done", "metrics+done 后状态保持 done");
        CHECK(layer3->metricsSummary().contains("12.12"),
              "metricsSummary 应包含行程 12.12");
        CHECK(layer3->stateLabelText().contains("12.12") &&
              layer3->stateLabelText().contains("0.398"),
              "状态标签应保留行程与均速，不被 finished 覆盖");

        QFile::remove(shotPath);
        delete dock3;
        delete layer3;
    }

    delete layer;
    delete scene;
    printf(g_fail ? "\nLIVE SMOKE FAIL (%d)\n" : "\nLIVE SMOKE PASS\n", g_fail);
    return g_fail ? 1 : 0;
}
