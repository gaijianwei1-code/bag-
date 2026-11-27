"""T3.4 跟随项：前视检测距离与 lookahead 公式，与 PursuitLocalPlanner 同口径。

与 maptool-sim/core/planner_limits.{h,cpp} 保持一致（纯算术，零第三方依赖）：
    检测距离 = collision_linear_vel(硬编码 0.4) × collision_time_*（四场景切换）

场景判定（pursuit_local_planner.cpp:261-275）：
    curve_paths == 1 且 turnAround == 1 → curve_turn_around
    curve_paths == 1 且 bypass == 1     → curve_no_bypass   （⚠️ bypass 语义反相：1=不允许绕行）
    curve_paths == 1                    → curve
    其余                                 → straight          （bypass 不参与）

lookahead（pursuit_local_planner.cpp:558-576，use_velocity_scaled_lookahead_dist=true）：
    有转向 → 1.5；否则 2 × v × lookahead_time；clamp 到 [min, max]
"""
import math
import os
from dataclasses import dataclass, replace
from typing import List, Optional, Tuple

PLANNER_COLLISION_VEL = 0.4

SCENE_STRAIGHT = "straight"
SCENE_CURVE = "curve"
SCENE_CURVE_NO_BYPASS = "curve_no_bypass"
SCENE_CURVE_TURN_AROUND = "curve_turn_around"

SCENE_LABELS = {
    SCENE_STRAIGHT: "直线",
    SCENE_CURVE: "一般曲线",
    SCENE_CURVE_NO_BYPASS: "曲线不绕行",
    SCENE_CURVE_TURN_AROUND: "曲线掉头",
}

_REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASELINE_YAML = os.path.join(_REPO_ROOT, "config",
                             "local_planner_params.baseline_20261124.yaml")


@dataclass
class PlannerLimits:
    collision_time_straight: float = 3.0
    collision_time_curve: float = 2.0
    collision_time_curve_no_bypass: float = 2.0
    collision_time_curve_turn_around: float = 1.0
    collision_linear_vel: float = PLANNER_COLLISION_VEL
    lookahead_dist: float = 1.5
    min_lookahead_dist: float = 1.0
    max_lookahead_dist: float = 4.0
    lookahead_time: float = 3.0
    use_velocity_scaled_lookahead_dist: bool = True


# 可解析/可覆盖的 key（长 key 在前，避免前缀误匹配）
_FLOAT_KEYS = (
    "collision_time_curve_turn_around",
    "collision_time_curve_no_bypass",
    "collision_time_straight",
    "collision_time_curve",
    "collision_linear_vel",
    "min_lookahead_dist",
    "max_lookahead_dist",
    "lookahead_time",
    "lookahead_dist",
)
_BOOL_KEYS = ("use_velocity_scaled_lookahead_dist",)


def scene_label(scene: str) -> str:
    return SCENE_LABELS.get(scene, scene or "")


def _i(v) -> int:
    try:
        return int(float(v))
    except (TypeError, ValueError):
        return 0


def pick_scene(curve_paths=0, turn_around=0, bypass=0) -> str:
    if _i(curve_paths) == 1:
        if _i(turn_around) == 1:
            return SCENE_CURVE_TURN_AROUND
        if _i(bypass) == 1:        # 反相：1 = 不允许绕行
            return SCENE_CURVE_NO_BYPASS
        return SCENE_CURVE
    return SCENE_STRAIGHT


def scene_collision_time(limits: PlannerLimits, scene: str) -> float:
    return {
        SCENE_STRAIGHT: limits.collision_time_straight,
        SCENE_CURVE: limits.collision_time_curve,
        SCENE_CURVE_NO_BYPASS: limits.collision_time_curve_no_bypass,
        SCENE_CURVE_TURN_AROUND: limits.collision_time_curve_turn_around,
    }.get(scene, limits.collision_time_straight)


def scene_front_dist(limits: PlannerLimits, scene: str) -> float:
    v = limits.collision_linear_vel if limits.collision_linear_vel > 0 \
        else PLANNER_COLLISION_VEL
    return max(0.0, scene_collision_time(limits, scene) * v)


def front_dist_for_point(limits: PlannerLimits, pt: dict) -> Tuple[str, float]:
    pt = pt or {}
    turn = pt.get("turnAround", pt.get("turn_around", 0))
    scene = pick_scene(pt.get("curve_paths", 0), turn, pt.get("bypass", 0))
    return scene, scene_front_dist(limits, scene)


def compute_lookahead_dist(limits: PlannerLimits, linear_vel: float,
                           angular_vel: float = 0.0,
                           angular_eps: float = 0.02) -> float:
    if not limits.use_velocity_scaled_lookahead_dist:
        return float(limits.lookahead_dist)
    try:
        av = abs(float(angular_vel or 0.0))
        lv = abs(float(linear_vel or 0.0))
    except (TypeError, ValueError):
        av, lv = 0.0, 0.0
    if av > angular_eps:
        d = 1.5                      # 有转向 → 死锁 1.5
    else:
        d = 2.0 * lv * float(limits.lookahead_time)   # 3/1.5 × v × lookahead_time
    return min(max(d, float(limits.min_lookahead_dist)),
               float(limits.max_lookahead_dist))


def _strip_comment(s: str) -> str:
    i = s.find("#")
    return s[:i] if i >= 0 else s


def parse_planner_limits(yaml_text: str,
                         base: Optional[PlannerLimits] = None
                         ) -> Tuple[PlannerLimits, int]:
    """逐行精确匹配 key（不引 yaml 库）。缺失项保留 base/默认，不归零。"""
    lim = replace(base) if base is not None else PlannerLimits()
    hits = 0
    seen = set()
    for raw in (yaml_text or "").splitlines():
        line = _strip_comment(raw).strip()
        if ":" not in line:
            continue
        key, _, val = line.partition(":")
        key = key.strip()
        val = val.strip()
        if key in seen or not val:
            continue
        if key in _FLOAT_KEYS:
            try:
                setattr(lim, key, float(val))
            except ValueError:
                continue
            seen.add(key)
            hits += 1
        elif key in _BOOL_KEYS:
            low = val.lower()
            if low in ("true", "false"):
                setattr(lim, key, low == "true")
                seen.add(key)
                hits += 1
    return lim, hits


def _norm_key(k: str) -> str:
    k = str(k or "").strip()
    for pre in ("/sweep_base/PursuitLocalPlanner/", "sweep_base/PursuitLocalPlanner/",
                "PursuitLocalPlanner."):
        if k.startswith(pre):
            return k[len(pre):]
    return k


def _apply_one(lim: PlannerLimits, key: str, value) -> None:
    key = _norm_key(key)
    if key in _FLOAT_KEYS:
        try:
            setattr(lim, key, float(value))
        except (TypeError, ValueError):
            pass
    elif key in _BOOL_KEYS:
        if isinstance(value, str):
            value = value.strip().lower() == "true"
        setattr(lim, key, bool(value))


def load_planner_limits(yaml_path: Optional[str] = None,
                        overrides: Optional[dict] = None,
                        param_changes: Optional[list] = None) -> PlannerLimits:
    lim = PlannerLimits()
    path = yaml_path or BASELINE_YAML
    try:
        with open(path, "r", encoding="utf-8") as f:
            lim, _ = parse_planner_limits(f.read(), lim)
    except OSError:
        pass
    for k, v in (overrides or {}).items():
        _apply_one(lim, k, v)
    for c in param_changes or []:
        if not isinstance(c, dict):
            continue
        val = c.get("to", c.get("value"))
        if val is None:
            continue
        _apply_one(lim, c.get("key") or c.get("rosparam") or "", val)
    return lim


class TrajectorySceneMatcher:
    """把车位姿匹配到轨迹最近点，取该点属性对应的场景与检测距离。"""

    def __init__(self, points: List[dict], limits: PlannerLimits):
        self.limits = limits
        self._pts = []
        for p in points or []:
            if not isinstance(p, dict) or "x" not in p or "y" not in p:
                continue
            try:
                x, y = float(p["x"]), float(p["y"])
            except (TypeError, ValueError):
                continue
            scene, dist = front_dist_for_point(limits, p)
            self._pts.append((x, y, scene, dist))
        self._last_idx: Optional[int] = None

    def __len__(self):
        return len(self._pts)

    def _nearest(self, x, y, lo, hi):
        best_i, best_d = -1, float("inf")
        for i in range(lo, hi):
            px, py = self._pts[i][0], self._pts[i][1]
            d = math.hypot(px - x, py - y)
            if d < best_d:
                best_i, best_d = i, d
        return best_i, best_d

    def match(self, x: float, y: float,
              max_dist: float = 3.0) -> Optional[Tuple[int, str, float]]:
        n = len(self._pts)
        if n == 0:
            return None
        idx, d = -1, float("inf")
        if self._last_idx is not None:
            lo = max(0, self._last_idx - 10)
            hi = min(n, self._last_idx + 40)
            i, dd = self._nearest(x, y, lo, hi)
            if i >= 0 and dd <= 1.5:
                idx, d = i, dd
        if idx < 0:
            idx, d = self._nearest(x, y, 0, n)
        if idx < 0 or d > max_dist:
            return None
        self._last_idx = idx
        return idx, self._pts[idx][2], self._pts[idx][3]
