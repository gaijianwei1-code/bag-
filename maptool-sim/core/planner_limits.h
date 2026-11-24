#pragma once
// T3.4: L0 前视检测距离 —— 与 PursuitLocalPlanner 同口径
//
// 由来（2026-11-24，读 pursuit_local_planner.cpp 确认）：
//   碰撞检测距离不是配置里写死的米数，而是
//       distance = collision_linear_vel × max_allowed_time_to_collision
//   其中 collision_linear_vel 在校验函数里硬编码 0.4（`auto collision_linear_vel = 0.4;`），
//   max_allowed_time_to_collision 由四个 collision_time_* 参数按场景切换。
//
//   场景判定（pursuit_local_planner.cpp:261-275）优先级：
//     ① curve_paths == 1 且 turnAround == 1  → collision_time_curve_turn_around
//     ② curve_paths == 1 且 no_bypass == 1   → collision_time_curve_no_bypass
//     ③ curve_paths == 1                     → collision_time_curve
//     ④ 其余（直线）                          → collision_time_straight
//
//   ⚠️ bypass 字段语义是反的：sweep_base.cpp:727 `if (bypass_attr == 1)
//      no_bypass_vector.push_back(index);` —— 轨迹点写 `bypass: 1` 表示
//      **不允许绕行**，对应 no_bypass_flag = 1。这里按真实语义取名，
//      调用方传原始 `bypass` 字段进来，由本模块做反相。
//
// 纯几何/纯算术，无 Qt / ROS 依赖，可单独单测。

namespace blz {

// PursuitLocalPlanner 碰撞检测里硬编码的前推速度（m/s）。
// 注意它**不是** fast_linear_vel：源码 `isCollisionImminent()` 里写死 0.4，
// 改 fast_linear_vel 不会改变检测距离。保留为常量 + 可覆盖，便于将来对齐。
constexpr double kPlannerCollisionVel = 0.4;

// 与 sweep_base::goal_count::Request 的轨迹点字段一一对应。
struct PointFlags {
    int curve_paths = 0;   // 1 = 曲线路段
    int turnAround = 0;    // 1 = 掉头点
    int bypass = 0;        // ⚠️ 语义反相：1 = 不允许绕行（no_bypass）
};

enum class DriveScene {
    Straight = 0,        // 直线
    Curve = 1,           // 一般曲线
    CurveNoBypass = 2,   // 曲线且不允许绕行
    CurveTurnAround = 3, // 曲线且掉头
};

const char* sceneName(DriveScene s);

// 四场景的 collision_time 参数（秒），与 local_planner_params.yaml 同名。
struct PlannerLimits {
    double collision_time_straight = 3.0;
    double collision_time_curve = 2.0;
    double collision_time_curve_no_bypass = 2.0;
    double collision_time_curve_turn_around = 1.0;
    double collision_linear_vel = kPlannerCollisionVel;
};

// 按优先级挑场景（与 pursuit_local_planner.cpp 完全一致）。
DriveScene pickScene(const PointFlags& f);

// 该场景的检测时间（秒）。
double sceneCollisionTime(const PlannerLimits& lim, DriveScene s);

// 该场景的实际检测距离（米）= collision_time × collision_linear_vel。
double sceneFrontDist(const PlannerLimits& lim, DriveScene s);

// 一步到位：给轨迹点属性，返回 planner 真实的前视检测距离。
double frontDistForPoint(const PlannerLimits& lim, const PointFlags& f);

// 从 yaml 文本里解析四个 collision_time_*（不引入 yaml 库，按行匹配）。
// 找不到的项保留默认值；返回成功解析到的项数，便于调用方提示。
int parsePlannerLimits(const char* yamlText, PlannerLimits& out);

}  // namespace blz
