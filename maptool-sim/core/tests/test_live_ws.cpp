// T2.4 直播跟车「真实 WS 连接」冒烟：复刻用户点「连接直播」的完整路径
//
// 之前的 test_live_layer 只测了坐标换算/解析，**从未真正连过 WS**，
// 所以「点连接直播没响应」这类问题测试抓不到。
//
// 用法：test_live_ws <baseUrl> <runId> <token>
//   退出码 0 = 通过；非 0 = 失败
#include "live_layer.h"
#include "../items/GraphicsType.h"

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QDockWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <cstdio>

static int g_fail = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s (line %d)\n", m, __LINE__); ++g_fail; } \
                         else printf("ok:   %s\n", m); } while (0)

static void wait(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

// 等到条件成立或超时
template <typename Fn>
static bool waitFor(Fn cond, int timeoutMs, int stepMs = 100)
{
    for (int t = 0; t < timeoutMs; t += stepMs) {
        if (cond()) return true;
        wait(stepMs);
    }
    return cond();
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    const QString base  = argc > 1 ? QString::fromUtf8(argv[1]) : QStringLiteral("ws://127.0.0.1:9100");
    const QString runId = argc > 2 ? QString::fromUtf8(argv[2]) : QString();
    const QString token = argc > 3 ? QString::fromUtf8(argv[3]) : QStringLiteral("dev-token");

    auto* scene = new BQGraphicsScene;
    auto* pix = scene->addPixmap(QPixmap(2856, 3104));
    pix->setZValue(0);

    auto* layer = new LiveLayer;
    layer->attachScene(scene);
    // 与真实使用一致：先打开地图拿到 resolution/origin
    layer->setMapInfo(0.05, -22.796825, -77.406677, 3104);

    // 收集状态变化（模拟 dock 里的标签更新）
    QStringList states;
    QObject::connect(layer, &LiveLayer::stateChanged,
                     [&states](const QString& st, double, const QString& msg) {
        states << QStringLiteral("%1(%2)").arg(st, msg);
        printf("  [state] %s  %s\n", qPrintable(st), qPrintable(msg));
    });

    // ---------- A. run_id 为空：必须报 error，不能静默 ----------
    printf("\n== A. run_id 为空（防静默）==\n");
    layer->startLive(base, QString(), token);
    wait(300);
    CHECK(layer->state() == QStringLiteral("error"), "run_id 为空 → state=error");
    CHECK(!layer->lastError().isEmpty(), "run_id 为空 → 有可读错误消息");
    printf("  消息 = %s\n", qPrintable(layer->lastError()));

    if (runId.isEmpty()) {
        printf("\n未指定 run_id，跳过真实连接测试。\n");
        printf("用法: test_live_ws ws://127.0.0.1:9100 <runId> dev-token\n");
        return g_fail == 0 ? 0 : 1;
    }

    // ---------- B. 真实连接 ----------
    // 说明：WS 握手成功后 run 可能立即失败并关闭连接，这是**正确行为**
    // （bridge 必须先告知失败原因再关）。所以这里不清算 connected()，而是
    // 判定"要么持续出帧，要么给出明确失败原因"，两者都算通过。
    printf("\n== B. 真实 WS 连接 run_id=%s ==\n", qPrintable(runId));
    states.clear();
    layer->startLive(base, runId, token);

    const bool connected =
        waitFor([layer] { return layer->connected(); }, 8000);
    const QString handshakeState = layer->state();
    printf("  握手后状态 = %s  connected() = %d\n",
           qPrintable(handshakeState), int(connected));
    CHECK(connected || handshakeState == QStringLiteral("failed"),
          "WS 握手成功或立即收到明确失败原因");
    // 状态必须真的变成 "connected"，否则界面会永远显示"连接中…"（= 用户感知的"没响应"）
    if (connected)
        CHECK(layer->state() == QStringLiteral("connected"),
              "连上后 state 变为 connected（不接受停在 connecting）");

    // 连上后必须是「持续出帧」或「给出明确失败原因」。
    // 注意：不能等终态——run 本身可能要跑几十秒，直播的意义就是边跑边看。
    // POST /runs 后需经历 worktree + init_pose 提取 + wait_for_ros + roslaunch 冷启动，
    // 实测首帧约 25–35s，10s 容易误报，故放宽至 45000 ms。
    const bool gotFrames = waitFor([layer] { return layer->frameCount() > 0; }, 45000);
    CHECK(gotFrames, "收到至少 1 帧（frameCount>0）");

    // 确认是持续推送而非只 1 帧（界面才会「动起来」）
    const int f1 = layer->frameCount();
    wait(4000);
    const int f2 = layer->frameCount();
    CHECK(f2 > f1, "帧持续推送（4 秒后帧数增长）");

    const QString finalState = layer->state();
    const QString finalErr   = layer->lastError();
    printf("  状态 = %s  帧数 %d → %d\n", qPrintable(finalState), f1, f2);

    if (finalState == QStringLiteral("failed")) {
        CHECK(!finalState.isEmpty() && !finalErr.isEmpty(), "失败时给出可读原因");
        CHECK(!finalErr.contains("remote host closed"),
              "失败原因不是含糊的『远端关闭连接』");
        return g_fail == 0 ? 0 : 1;
    }

    CHECK(layer->overlayAlive(), "overlay 存活");
    CHECK(layer->overlayHasVehicle(), "overlay 已绘制车辆（界面上能看到车）");

    layer->stopLive();
    CHECK(!layer->connected(), "stopLive 后断开");
    CHECK(layer->state() == QStringLiteral("idle"), "stopLive 后 state=idle");

    printf("\n%s\n", g_fail == 0 ? "全部通过" : "有失败项");
    return g_fail == 0 ? 0 : 1;
}
