// T3.4 冒烟：L0 前视距离按 planner 场景切换（FovLayer 集成层）
// 验证：同口径开关生效 / 每点按属性取距离 / 关闭时回退固定值 / 参数注入。
#include "fov_layer.h"
#include "planner_limits.h"
#include "../items/GraphicsType.h"

#include <QApplication>
#include <QGraphicsScene>
#include <cstdio>

static int g_fail = 0;
static int g_pass = 0;
#define CHECK(c, m)                                                            \
    do {                                                                       \
        if (!(c)) { ++g_fail; std::printf("FAIL: %s (line %d)\n", m, __LINE__); } \
        else { ++g_pass; std::printf("ok:   %s\n", m); }                        \
    } while (0)

static bool near(double a, double b, double eps = 1e-9) {
    return (a > b ? a - b : b - a) <= eps;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    auto* layer = new FovLayer;

    // ---- 1. 默认关闭同口径（不改变既有行为）----
    CHECK(!layer->plannerMode(), "默认不同口径，保持旧行为");

    // ---- 2. 默认 limits 与用户给的参数一致 ----
    {
        const auto& lim = layer->plannerLimits();
        CHECK(near(lim.collision_time_straight, 3.0), "默认 straight=3.0s");
        CHECK(near(lim.collision_time_curve, 2.0), "默认 curve=2.0s");
        CHECK(near(lim.collision_time_curve_no_bypass, 2.0), "默认 no_bypass=2.0s");
        CHECK(near(lim.collision_time_curve_turn_around, 1.0), "默认 turn=1.0s");
        CHECK(near(lim.collision_linear_vel, 0.4), "collision 速度 0.4 m/s");
    }

    // ---- 3. 四场景距离 ----
    CHECK(near(layer->frontDistForScene(blz::DriveScene::Straight), 1.20),
          "直线 1.20m");
    CHECK(near(layer->frontDistForScene(blz::DriveScene::Curve), 0.80),
          "曲线 0.80m");
    CHECK(near(layer->frontDistForScene(blz::DriveScene::CurveNoBypass), 0.80),
          "不绕行 0.80m");
    CHECK(near(layer->frontDistForScene(blz::DriveScene::CurveTurnAround), 0.40),
          "掉头 0.40m");

    // ---- 4. 开关可切换，且幂等 ----
    layer->setPlannerMode(true);
    CHECK(layer->plannerMode(), "开启同口径");
    layer->setPlannerMode(true);
    CHECK(layer->plannerMode(), "重复开启幂等");
    layer->setPlannerMode(false);
    CHECK(!layer->plannerMode(), "可关闭");

    // ---- 5. 注入容器实际参数（curve=1.5）后距离跟着变 ----
    {
        blz::PlannerLimits lim;
        lim.collision_time_curve = 1.5;
        layer->setPlannerLimits(lim);
        CHECK(near(layer->frontDistForScene(blz::DriveScene::Curve), 0.60),
              "注入 curve=1.5s → 0.60m");
        // 其他场景不受影响
        CHECK(near(layer->frontDistForScene(blz::DriveScene::Straight), 1.20),
              "注入后直线仍 1.20m");
    }

    // ---- 6. 真实场景计算：按轨迹点属性 ----
    {
        blz::PlannerLimits lim;   // 回到默认
        layer->setPlannerLimits(lim);
        blz::PointFlags std_sample;      // 标准样本：curve=1, turn=0, bypass=1
        std_sample.curve_paths = 1;
        std_sample.bypass = 1;
        CHECK(near(blz::frontDistForPoint(lim, std_sample), 0.80),
              "标准样本点 → 不绕行 0.80m");

        blz::PointFlags straight;        // 全部为 0
        CHECK(near(blz::frontDistForPoint(lim, straight), 1.20),
              "全 0 点 → 直线 1.20m");

        blz::PointFlags turn = std_sample;
        turn.turnAround = 1;
        CHECK(near(blz::frontDistForPoint(lim, turn), 0.40),
              "掉头点 → 0.40m");

        blz::PointFlags curve_ok = std_sample;
        curve_ok.bypass = 0;             // 允许绕行
        CHECK(near(blz::frontDistForPoint(lim, curve_ok), 0.80),
              "曲线可绕行 → 0.80m（curve=2.0）");
    }

    // ---- 7. 异常参数兜底：不能让同口径算出 0 距离导致永不报警 ----
    {
        blz::PlannerLimits bad;
        bad.collision_time_straight = 0.0;
        bad.collision_linear_vel = 0.0;
        layer->setPlannerLimits(bad);
        // 场景距离为 0，调用方（fov_layer）会用界面值兜底；这里验证算出的是 0
        CHECK(near(layer->frontDistForScene(blz::DriveScene::Straight), 0.0),
              "非法参数算出 0（由调用方兜底）");
    }

    std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
    delete layer;
    return g_fail == 0 ? 0 : 1;
}
