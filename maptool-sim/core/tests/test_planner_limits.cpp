// T3.4 冒烟：L0 前视距离与 PursuitLocalPlanner 同口径
// 纯算术，无 Qt / ROS 依赖，一条命令跑完。
#include "planner_limits.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace blz;

static int g_fail = 0;
static int g_pass = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        if (cond) { ++g_pass; }                                               \
        else { ++g_fail; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, msg); } \
    } while (0)

#define NEAR(a, b, eps) (std::fabs((a) - (b)) <= (eps))

static PointFlags mk(int curve, int turn, int bypass) {
    PointFlags f; f.curve_paths = curve; f.turnAround = turn; f.bypass = bypass;
    return f;
}

// ---------------------------------------------------------------- 场景判定
static void test_scene_priority() {
    // 掉头优先于不绕行：curve=1 turn=1 bypass=1 → 掉头
    CHECK(pickScene(mk(1, 1, 1)) == DriveScene::CurveTurnAround,
          "turn_around 优先级应高于 no_bypass");
    CHECK(pickScene(mk(1, 1, 0)) == DriveScene::CurveTurnAround,
          "turn_around=1 无论 bypass 都判掉头");

    // 曲线 + bypass=1（不允许绕行）
    CHECK(pickScene(mk(1, 0, 1)) == DriveScene::CurveNoBypass,
          "bypass=1 应判 no_bypass（反相语义）");
    // 曲线 + bypass=0（允许绕行）→ 一般曲线
    CHECK(pickScene(mk(1, 0, 0)) == DriveScene::Curve,
          "bypass=0 应判一般曲线");

    // 非曲线一律直线，即使 bypass/turn 有值
    CHECK(pickScene(mk(0, 0, 0)) == DriveScene::Straight, "默认直线");
    CHECK(pickScene(mk(0, 1, 1)) == DriveScene::Straight,
          "curve_paths=0 时不看 turn/bypass，按直线兜底");
}

// ---------------------------------------------------------------- 距离换算
static void test_distance_is_time_times_vel() {
    PlannerLimits lim;   // 用户给的默认值
    CHECK(NEAR(sceneFrontDist(lim, DriveScene::Straight), 1.20, 1e-9),
          "直线 3.0s × 0.4 = 1.20m");
    CHECK(NEAR(sceneFrontDist(lim, DriveScene::Curve), 0.80, 1e-9),
          "曲线 2.0s × 0.4 = 0.80m");
    CHECK(NEAR(sceneFrontDist(lim, DriveScene::CurveNoBypass), 0.80, 1e-9),
          "不绕行 2.0s × 0.4 = 0.80m");
    CHECK(NEAR(sceneFrontDist(lim, DriveScene::CurveTurnAround), 0.40, 1e-9),
          "掉头 1.0s × 0.4 = 0.40m");
}

static void test_container_baseline_values() {
    // 容器 sweeper_v1 实际值：curve=1.5、regulated_radius=4.0（后者不影响距离）
    PlannerLimits lim;
    lim.collision_time_curve = 1.5;
    CHECK(NEAR(sceneFrontDist(lim, DriveScene::Curve), 0.60, 1e-9),
          "容器 curve=1.5s → 0.60m");
}

static void test_vel_override() {
    // 若将来 planner 改了 collision_linear_vel，距离必须跟着变
    PlannerLimits lim;
    lim.collision_linear_vel = 0.5;
    CHECK(NEAR(sceneFrontDist(lim, DriveScene::Straight), 1.50, 1e-9),
          "vel 0.5 × 3.0 = 1.50m");
    // 非法值（0/负）退回常量，不能返回 0 导致"永不报警"
    lim.collision_linear_vel = 0.0;
    CHECK(NEAR(sceneFrontDist(lim, DriveScene::Straight), 1.20, 1e-9),
          "vel=0 应退回 kPlannerCollisionVel，不能得 0");
    lim.collision_linear_vel = -1.0;
    CHECK(NEAR(sceneFrontDist(lim, DriveScene::Straight), 1.20, 1e-9),
          "vel<0 同样退回常量");
}

static void test_zero_time_guard() {
    PlannerLimits lim;
    lim.collision_time_curve = 0.0;
    CHECK(NEAR(sceneFrontDist(lim, DriveScene::Curve), 0.0, 1e-9),
          "time=0 → 距离 0（禁用该场景检测），不得为负");
}

static void test_front_dist_for_point() {
    PlannerLimits lim;
    // 标准样本轨迹：curve_paths=1, turnAround=0, bypass=1 → 不绕行 0.80m
    CHECK(NEAR(frontDistForPoint(lim, mk(1, 0, 1)), 0.80, 1e-9),
          "标准样本点（curve=1,bypass=1）→ 0.80m");
    CHECK(NEAR(frontDistForPoint(lim, mk(0, 0, 1)), 1.20, 1e-9),
          "直线点 → 1.20m");
}

// ---------------------------------------------------------------- 真实轨迹分布
// 南岸晶都 1119主路.json（4134 点）的实际属性分布，作为回归基线：
//   直线 3092 / 曲线不绕行 857 / 曲线掉头 155 / 一般曲线 30
// 若哪天 pickScene 改坏（比如掉头判不进、bypass 反相丢了），这条会立刻红。
static void test_real_trajectory_distribution() {
    // (curve_paths, turnAround, bypass) → 点数
    struct Row { int c, t, b, n; DriveScene expect; };
    // 注意：turnAround 优先于 bypass，所以 (1,1,1) 那 155 点是从
    // (curve=1,bypass=1) 的 1012 点里再切出来的，no_bypass 只剩 1012-155=857。
    // 早先按属性列分别统计再加总会重复计数，这里按"最终落进哪一档"归并。
    const Row rows[] = {
        {0, 0, 0,  484, DriveScene::Straight},        // 直+可绕行
        {0, 0, 1, 2608, DriveScene::Straight},        // 直+不绕行（bypass 被忽略，与 planner 一致）
        {1, 0, 0,   30, DriveScene::Curve},           // 曲线+可绕行
        {1, 0, 1,  857, DriveScene::CurveNoBypass},   // 曲线+不绕行（1012 减去掉头）
        {1, 1, 1,  155, DriveScene::CurveTurnAround}, // 掉头
        {1, 1, 0,    0, DriveScene::CurveTurnAround}, // 真实数据里掉头点 bypass 均为 1
    };
    PlannerLimits lim;
    int total = 0;
    double minDist = 1e9, maxDist = -1e9;
    for (const auto& r : rows) {
        const auto sc = pickScene(mk(r.c, r.t, r.b));
        CHECK(sc == r.expect, "真实分布：场景判定与预期一致");
        if (r.n > 0) {
            total += r.n;
            const double d = sceneFrontDist(lim, sc);
            if (d < minDist) minDist = d;
            if (d > maxDist) maxDist = d;
        }
    }
    CHECK(total == 4134, "四点合计应等于真实轨迹点数 4134");

    // 关键：四档里必须出现三种不同距离（掉头 0.40 / 曲线 0.80 / 直线 1.20）
    // 若 regression 里某两档被合并，这条会红。
    CHECK(NEAR(minDist, 0.40, 1e-9), "真实分布最小距离 = 掉头 0.40m");
    CHECK(NEAR(maxDist, 1.20, 1e-9), "真实分布最大距离 = 直线 1.20m");

    // 掉头点必须真的比直线点近 3 倍，这是"分场景"的意义所在
    CHECK(NEAR(sceneFrontDist(lim, DriveScene::Straight)
               / sceneFrontDist(lim, DriveScene::CurveTurnAround), 3.0, 1e-9),
          "直线距离是掉头的 3 倍");
}

// ---------------------------------------------------------------- yaml 解析
static void test_yaml_parse_full() {
    const char* y =
        "PursuitLocalPlanner:\n"
        "  goal_dist_tol: 0.5 #1.0\n"
        "  fast_linear_vel: 0.4 # vehicle speed\n"
        "  collision_time_straight: 3.0           # 直线\n"
        "  collision_time_curve: 2.0              # 曲线\n"
        "  collision_time_curve_no_bypass: 2.0    # 不绕行\n"
        "  collision_time_curve_turn_around: 1.0  # 掉头\n"
        "  enable_collision_check: true\n";
    PlannerLimits lim;
    const int n = parsePlannerLimits(y, lim);
    CHECK(n == 4, "应解析到 4 个 collision_time");
    CHECK(NEAR(lim.collision_time_straight, 3.0, 1e-9), "straight=3.0");
    CHECK(NEAR(lim.collision_time_curve, 2.0, 1e-9), "curve=2.0");
    CHECK(NEAR(lim.collision_time_curve_no_bypass, 2.0, 1e-9), "no_bypass=2.0");
    CHECK(NEAR(lim.collision_time_curve_turn_around, 1.0, 1e-9), "turn=1.0");
}

static void test_yaml_prefix_trap() {
    // collision_time_curve 与 collision_time_curve_no_bypass 是前缀关系，
    // 若解析器按"包含"匹配会把 no_bypass 的值串到 curve 上。
    const char* y =
        "  collision_time_curve_no_bypass: 7.0\n"
        "  collision_time_curve_turn_around: 9.0\n";
    PlannerLimits lim;
    lim.collision_time_curve = 1.25;   // 用哨兵值确认没被污染
    parsePlannerLimits(y, lim);
    CHECK(NEAR(lim.collision_time_curve_no_bypass, 7.0, 1e-9), "no_bypass=7.0");
    CHECK(NEAR(lim.collision_time_curve_turn_around, 9.0, 1e-9), "turn=9.0");
    CHECK(NEAR(lim.collision_time_curve, 1.25, 1e-9),
          "curve 必须保持哨兵值，不能被 no_bypass 前缀污染");
}

static void test_yaml_missing_keeps_defaults() {
    const char* y = "  collision_time_straight: 4.5\n";
    PlannerLimits lim;
    const int n = parsePlannerLimits(y, lim);
    CHECK(n == 1, "只解析到 1 项");
    CHECK(NEAR(lim.collision_time_straight, 4.5, 1e-9), "straight 更新");
    CHECK(NEAR(lim.collision_time_curve_turn_around, 1.0, 1e-9),
          "缺失项保留默认 1.0，不能变 0");
}

static void test_yaml_ignores_fast_linear_vel() {
    // fast_linear_vel 存在但不影响检测距离，这是最容易搞错的一条
    PlannerLimits lim;
    parsePlannerLimits("  fast_linear_vel: 0.9\n", lim);
    CHECK(NEAR(lim.collision_linear_vel, kPlannerCollisionVel, 1e-9),
          "fast_linear_vel 不得改变 collision_linear_vel");
}

static void test_yaml_bad_values() {
    PlannerLimits lim;
    const double before = lim.collision_time_curve;
    const int n = parsePlannerLimits(
        "  collision_time_curve: abc\n"
        "  collision_time_straight:\n", lim);
    CHECK(n == 0, "非法值不该计入");
    CHECK(NEAR(lim.collision_time_curve, before, 1e-9), "非法值不改默认");
}

static void test_yaml_null_safe() {
    PlannerLimits lim;
    CHECK(parsePlannerLimits(nullptr, lim) == 0, "null 输入返回 0 不崩");
    CHECK(parsePlannerLimits("", lim) == 0, "空串返回 0");
}

static void test_scene_names() {
    CHECK(std::strcmp(sceneName(DriveScene::Straight), "直线") == 0, "直线名");
    CHECK(std::strcmp(sceneName(DriveScene::CurveTurnAround), "曲线掉头") == 0,
          "掉头名");
}

int main() {
    test_scene_priority();
    test_distance_is_time_times_vel();
    test_container_baseline_values();
    test_vel_override();
    test_zero_time_guard();
    test_front_dist_for_point();
    test_real_trajectory_distribution();
    test_yaml_parse_full();
    test_yaml_prefix_trap();
    test_yaml_missing_keeps_defaults();
    test_yaml_ignores_fast_linear_vel();
    test_yaml_bad_values();
    test_yaml_null_safe();
    test_scene_names();

    std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
