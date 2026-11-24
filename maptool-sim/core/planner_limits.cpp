// T3.4: L0 前视检测距离 —— 与 PursuitLocalPlanner 同口径
#include "planner_limits.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

namespace blz {

const char* sceneName(DriveScene s) {
    switch (s) {
        case DriveScene::Straight:        return "直线";
        case DriveScene::Curve:           return "一般曲线";
        case DriveScene::CurveNoBypass:   return "曲线不绕行";
        case DriveScene::CurveTurnAround: return "曲线掉头";
    }
    return "未知";
}

DriveScene pickScene(const PointFlags& f) {
    // 与 pursuit_local_planner.cpp:261-275 的 if/else 链逐条对齐：
    // 掉头判断在最前，其次不绕行，再次一般曲线，最后直线兜底。
    if (f.curve_paths == 1) {
        if (f.turnAround == 1) return DriveScene::CurveTurnAround;
        if (f.bypass == 1) return DriveScene::CurveNoBypass;  // 反相：1=不允许绕行
        return DriveScene::Curve;
    }
    return DriveScene::Straight;
}

double sceneCollisionTime(const PlannerLimits& lim, DriveScene s) {
    switch (s) {
        case DriveScene::Straight:        return lim.collision_time_straight;
        case DriveScene::Curve:           return lim.collision_time_curve;
        case DriveScene::CurveNoBypass:   return lim.collision_time_curve_no_bypass;
        case DriveScene::CurveTurnAround: return lim.collision_time_curve_turn_around;
    }
    return lim.collision_time_straight;
}

double sceneFrontDist(const PlannerLimits& lim, DriveScene s) {
    const double v = lim.collision_linear_vel > 0 ? lim.collision_linear_vel
                                                 : kPlannerCollisionVel;
    const double t = sceneCollisionTime(lim, s);
    return t > 0 ? t * v : 0.0;
}

double frontDistForPoint(const PlannerLimits& lim, const PointFlags& f) {
    return sceneFrontDist(lim, pickScene(f));
}

namespace {

// 跳过行首空白
const char* skipSpace(const char* p) {
    while (*p && (*p == ' ' || *p == '\t')) ++p;
    return p;
}

// 解析 "key: value  # comment" 里的 double。
// 只有 key 完全相等才算命中（避免 collision_time_curve 误匹配
// collision_time_curve_no_bypass 这种前缀陷阱）。
bool matchDouble(const char* line, size_t len, const char* key, double* out) {
    const size_t klen = std::strlen(key);
    // 行首（允许缩进）必须是 key，且紧跟冒号
    const char* p = skipSpace(line);
    const size_t off = static_cast<size_t>(p - line);
    if (len < off + klen + 1) return false;
    if (std::strncmp(p, key, klen) != 0) return false;
    const char* q = p + klen;
    if (*q != ':') return false;
    ++q;
    while (*q == ' ' || *q == '\t') ++q;
    if (!(*q == '-' || *q == '+' || *q == '.' ||
          (*q >= '0' && *q <= '9'))) return false;
    char* end = nullptr;
    const double v = std::strtod(q, &end);
    if (end == q) return false;
    *out = v;
    return true;
}

}  // namespace

int parsePlannerLimits(const char* yamlText, PlannerLimits& out) {
    if (!yamlText) return 0;
    int hits = 0;
    bool gotStraight = false, gotCurve = false;
    bool gotNoBypass = false, gotTurn = false;

    const char* p = yamlText;
    while (*p) {
        const char* eol = std::strchr(p, '\n');
        const size_t len = eol ? static_cast<size_t>(eol - p) : std::strlen(p);

        double v = 0.0;
        // 先匹配长 key，避免前缀误命中（curve_no_bypass 比 curve 长）
        if (!gotNoBypass &&
            matchDouble(p, len, "collision_time_curve_no_bypass", &v)) {
            out.collision_time_curve_no_bypass = v; gotNoBypass = true; ++hits;
        } else if (!gotTurn &&
                   matchDouble(p, len, "collision_time_curve_turn_around", &v)) {
            out.collision_time_curve_turn_around = v; gotTurn = true; ++hits;
        } else if (!gotStraight &&
                   matchDouble(p, len, "collision_time_straight", &v)) {
            out.collision_time_straight = v; gotStraight = true; ++hits;
        } else if (!gotCurve &&
                   matchDouble(p, len, "collision_time_curve", &v)) {
            out.collision_time_curve = v; gotCurve = true; ++hits;
        } else if (matchDouble(p, len, "fast_linear_vel", &v)) {
            // 注释醒目：fast_linear_vel 不参与检测距离计算，
            // 只有 collision_linear_vel 才决定前推速度。这里读到就忽略，
            // 仅用于确认"参数确实存在"，不给 PlannerLimits 赋值。
        }

        if (!eol) break;
        p = eol + 1;
    }
    return hits;
}

}  // namespace blz
