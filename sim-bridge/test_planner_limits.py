"""T3.4 跟随项：bridge 侧前视分档与 lookahead 理论公式的单元测试。

跑法：./.venv/bin/python -m pytest test_planner_limits.py -v
"""
import math
import os

import pytest

import planner_limits as PL
from live_adapter import TimelineAdapter
import report as R

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASELINE = os.path.join(REPO, "config", "local_planner_params.baseline_20261124.yaml")


# ---------------------------------------------------------------- 场景
def test_pick_scene_priority():
    assert PL.pick_scene(1, 1, 1) == PL.SCENE_CURVE_TURN_AROUND   # 掉头 > 不绕行
    assert PL.pick_scene(1, 0, 1) == PL.SCENE_CURVE_NO_BYPASS
    assert PL.pick_scene(1, 0, 0) == PL.SCENE_CURVE
    assert PL.pick_scene(0, 0, 0) == PL.SCENE_STRAIGHT


def test_bypass_one_means_no_bypass_and_ignored_on_straight():
    assert PL.pick_scene(0, 0, 1) == PL.SCENE_STRAIGHT    # 直线段 bypass 不参与
    assert PL.pick_scene(0, 1, 1) == PL.SCENE_STRAIGHT    # 非曲线时 turnAround 也不参与
    assert PL.pick_scene(1, 0, 1) != PL.SCENE_CURVE       # 反相：1 = 不绕行


def test_scene_front_dist_defaults():
    lim = PL.PlannerLimits()
    d = {s: PL.scene_front_dist(lim, s) for s in PL.SCENE_LABELS}
    assert d[PL.SCENE_STRAIGHT] == pytest.approx(1.20)
    assert d[PL.SCENE_CURVE] == pytest.approx(0.80)
    assert d[PL.SCENE_CURVE_NO_BYPASS] == pytest.approx(0.80)
    assert d[PL.SCENE_CURVE_TURN_AROUND] == pytest.approx(0.40)


def test_fast_linear_vel_does_not_change_distance():
    lim, _ = PL.parse_planner_limits("fast_linear_vel: 0.9\ncollision_time_straight: 3.0\n")
    assert PL.scene_front_dist(lim, PL.SCENE_STRAIGHT) == pytest.approx(1.20)


def test_front_dist_for_point_accepts_turn_around_alias():
    lim = PL.PlannerLimits()
    s, d = PL.front_dist_for_point(lim, {"curve_paths": 1, "turn_around": 1})
    assert s == PL.SCENE_CURVE_TURN_AROUND and d == pytest.approx(0.40)
    s, d = PL.front_dist_for_point(lim, {})
    assert s == PL.SCENE_STRAIGHT


def test_bad_collision_vel_falls_back():
    lim = PL.PlannerLimits(collision_linear_vel=0.0)
    assert PL.scene_front_dist(lim, PL.SCENE_STRAIGHT) == pytest.approx(1.20)


# ---------------------------------------------------------------- 解析
def test_parse_prefix_trap_and_comments():
    txt = ("  collision_time_curve_no_bypass: 5.0 # c\n"
           "  collision_time_curve: 2.5\n"
           "  collision_time_curve_turn_around: 0.5\n"
           "  collision_time_straight: 4.0\n")
    lim, hits = PL.parse_planner_limits(txt)
    assert hits == 4
    assert lim.collision_time_curve == 2.5
    assert lim.collision_time_curve_no_bypass == 5.0
    assert lim.collision_time_curve_turn_around == 0.5
    assert lim.collision_time_straight == 4.0


def test_parse_missing_keeps_defaults_not_zero():
    lim, hits = PL.parse_planner_limits("lookahead_time: 2.0\n")
    assert hits == 1
    assert lim.collision_time_straight == 3.0
    assert lim.lookahead_time == 2.0


def test_parse_real_baseline_yaml():
    with open(BASELINE, encoding="utf-8") as f:
        lim, hits = PL.parse_planner_limits(f.read())
    assert lim.collision_time_straight == 3.0
    assert lim.collision_time_curve_turn_around == 1.0
    assert lim.lookahead_dist == 1.5
    assert lim.use_velocity_scaled_lookahead_dist is True
    assert hits >= 8


def test_load_with_overrides_and_param_changes():
    lim = PL.load_planner_limits(
        overrides={"lookahead_time": 2.0},
        param_changes=[
            {"key": "PursuitLocalPlanner.collision_time_straight", "to": 5.0},
            {"rosparam": "/sweep_base/PursuitLocalPlanner/min_lookahead_dist", "value": 0.5},
            {"key": "PursuitLocalPlanner.use_velocity_scaled_lookahead_dist", "to": False},
            "garbage",
        ])
    assert lim.lookahead_time == 2.0
    assert lim.collision_time_straight == 5.0
    assert lim.min_lookahead_dist == 0.5
    assert lim.use_velocity_scaled_lookahead_dist is False


def test_load_missing_yaml_uses_defaults():
    lim = PL.load_planner_limits(yaml_path="/nonexistent/x.yaml")
    assert lim.collision_time_straight == 3.0


# ---------------------------------------------------------------- lookahead
def test_lookahead_formula():
    lim = PL.PlannerLimits()
    assert PL.compute_lookahead_dist(lim, 0.4) == pytest.approx(2.4)       # 2×0.4×3
    assert PL.compute_lookahead_dist(lim, 0.4, angular_vel=0.3) == pytest.approx(1.5)
    assert PL.compute_lookahead_dist(lim, 0.1) == pytest.approx(1.0)       # 低速 clamp
    assert PL.compute_lookahead_dist(lim, 1.0) == pytest.approx(4.0)       # 高速 clamp
    assert PL.compute_lookahead_dist(lim, 0.0) == pytest.approx(1.0)


def test_lookahead_unscaled_returns_config():
    lim = PL.PlannerLimits(use_velocity_scaled_lookahead_dist=False, lookahead_dist=1.7)
    assert PL.compute_lookahead_dist(lim, 0.4) == 1.7


# ---------------------------------------------------------------- 匹配器
def _pt(x, y, **kw):
    d = {"x": x, "y": y}
    d.update(kw)
    return d


def test_matcher_basic_and_out_of_range():
    pts = [_pt(i * 0.2, 0.0) for i in range(10)]
    m = PL.TrajectorySceneMatcher(pts, PL.PlannerLimits())
    idx, scene, dist = m.match(0.41, 0.1)
    assert idx == 2 and scene == PL.SCENE_STRAIGHT and dist == pytest.approx(1.2)
    assert m.match(50.0, 50.0) is None


def test_matcher_local_window_prevents_jump_on_crossing_path():
    # 往返轨迹：去程 y=0（直线），回程 y=0.5（掉头曲线），两段在 x 上重叠
    fwd = [_pt(i * 0.2, 0.0) for i in range(30)]
    back = [_pt((29 - i) * 0.2, 0.5, curve_paths=1, turnAround=1) for i in range(30)]
    m = PL.TrajectorySceneMatcher(fwd + back, PL.PlannerLimits())
    # 车在去程 x=2.0 附近，y=0.26：全局最近可能是回程点，但局部窗口应锁定去程
    m.match(1.8, 0.0)
    idx, scene, _ = m.match(2.0, 0.26)
    assert idx < 30 and scene == PL.SCENE_STRAIGHT


def test_matcher_global_fallback_when_lost():
    pts = [_pt(i * 0.2, 0.0) for i in range(100)]
    pts[90] = _pt(18.0, 0.0, curve_paths=1, turnAround=1)
    m = PL.TrajectorySceneMatcher(pts, PL.PlannerLimits())
    m.match(0.0, 0.0)
    idx, scene, dist = m.match(18.0, 0.0)     # 离局部窗口很远 → 全局扫描
    assert idx == 90 and scene == PL.SCENE_CURVE_TURN_AROUND and dist == pytest.approx(0.4)


def test_matcher_ignores_bad_points():
    m = PL.TrajectorySceneMatcher([{"x": 1}, "bad", {"x": "a", "y": 0}, _pt(0, 0)],
                                  PL.PlannerLimits())
    assert len(m) == 1


# ---------------------------------------------------------------- 适配器
def _drive(adapter, x, wall_x, t=0.0, speed=0.4):
    adapter.feed({"type": "state", "t": t, "x": x, "y": 0.0, "yaw": 0.0, "speed": speed})
    msgs = adapter.feed({"type": "points", "t": t, "pts": [[wall_x, 0.0, 0.0]]})
    return [m for m in msgs if m["type"] == "front_hits"]


def test_adapter_dynamic_front_dist_straight_vs_turnaround():
    traj = [_pt(0.0, 0.0), _pt(1.0, 0.0, curve_paths=1, turnAround=1)]
    # 障碍在车前 0.6 m：直线档 1.2 m 命中；掉头档 0.4 m 不命中
    a = TimelineAdapter(front_dist=3.0, hit_every=1, traj_points=traj)
    h = _drive(a, 0.0, 0.6)
    assert h and h[0]["hits"] and h[0]["scene"] == PL.SCENE_STRAIGHT
    assert h[0]["front_dist"] == pytest.approx(1.2)

    a2 = TimelineAdapter(front_dist=3.0, hit_every=1, traj_points=traj)
    h2 = _drive(a2, 1.0, 1.6)
    assert h2 and h2[0]["hits"] == [] and h2[0]["scene"] == PL.SCENE_CURVE_TURN_AROUND
    assert h2[0]["front_dist"] == pytest.approx(0.4)


def test_adapter_backward_compatible_without_traj():
    a = TimelineAdapter(front_dist=3.0, hit_every=1)
    h = _drive(a, 0.0, 2.5)
    assert h and len(h[0]["hits"]) == 1       # 固定 3.0 m
    assert "scene" not in h[0] and "front_dist" not in h[0]


def test_adapter_use_planner_false_disables():
    traj = [_pt(0.0, 0.0, curve_paths=1, turnAround=1)]
    a = TimelineAdapter(front_dist=3.0, hit_every=1, traj_points=traj,
                        use_planner_front_dist=False)
    h = _drive(a, 0.0, 2.5)
    assert h and len(h[0]["hits"]) == 1


def test_adapter_point_flags_direct_push_wins():
    a = TimelineAdapter(front_dist=3.0, hit_every=1)
    a.feed({"type": "state", "t": 0, "x": 0, "y": 0, "yaw": 0, "speed": 0.4,
            "point_flags": {"curve_paths": 1, "turnAround": 1}})
    msgs = a.feed({"type": "points", "t": 0, "pts": [[0.6, 0.0, 0.0]]})
    h = next(m for m in msgs if m["type"] == "front_hits")
    assert h["hits"] == [] and h["scene"] == PL.SCENE_CURVE_TURN_AROUND


def test_adapter_lookahead_expected_does_not_override_observed():
    a = TimelineAdapter()
    msgs = a.feed({"type": "state", "t": 0.0, "x": 0, "y": 0, "yaw": 0, "speed": 0.4})
    f = next(m for m in msgs if m["type"] == "frame")
    assert f["lookahead"] is None                       # 未观测就是 None，不拿理论值冒充
    assert f["lookahead_expected"] == pytest.approx(2.4)
    a.feed({"type": "lookahead", "dist": 1.766})
    msgs = a.feed({"type": "state", "t": 0.2, "x": 0, "y": 0, "yaw": 0, "speed": 0.4})
    f = next(m for m in msgs if m["type"] == "frame")
    assert f["lookahead"] == 1.766
    assert f["lookahead_expected"] == pytest.approx(2.4)


def test_adapter_omega_estimated_from_yaw():
    a = TimelineAdapter()
    a.feed({"type": "state", "t": 0.0, "x": 0, "y": 0, "yaw": 0.0, "speed": 0.4})
    msgs = a.feed({"type": "state", "t": 0.2, "x": 0, "y": 0, "yaw": 0.2, "speed": 0.4})
    f = next(m for m in msgs if m["type"] == "frame")
    assert f["lookahead_expected"] == pytest.approx(1.5)   # ω=1 rad/s → 死锁 1.5


def test_adapter_uses_upstream_angular_vel():
    a = TimelineAdapter()
    msgs = a.feed({"type": "state", "t": 0.0, "x": 0, "y": 0, "yaw": 0.0,
                   "speed": 0.4, "angular_vel": 0.5})
    f = next(m for m in msgs if m["type"] == "frame")
    assert f["lookahead_expected"] == pytest.approx(1.5)


def test_adapter_frame_scene_fields_only_when_matched():
    traj = [_pt(0.0, 0.0, curve_paths=1)]
    a = TimelineAdapter(traj_points=traj)
    f = next(m for m in a.feed({"type": "state", "t": 0, "x": 0, "y": 0, "yaw": 0})
             if m["type"] == "frame")
    assert f["scene"] == PL.SCENE_CURVE and f["front_dist"] == pytest.approx(0.8)
    far = next(m for m in a.feed({"type": "state", "t": 1, "x": 99, "y": 0, "yaw": 0})
               if m["type"] == "frame")
    assert "scene" not in far


# ---------------------------------------------------------------- 报告
def test_report_issue_card_shows_scene_and_expected_lookahead():
    frame = {"t": 3.0, "x": 1.0, "y": 2.0, "yaw": 0.0, "speed": 0.4,
             "lookahead": 1.5, "lookahead_expected": 2.4,
             "scene": PL.SCENE_CURVE_TURN_AROUND, "front_dist": 0.4}
    html = R._issues_section(None, [{"frame": frame, "level": "high",
                                     "kind": "前视命中", "why": "测试"}])
    assert "曲线掉头" in html and "0.40" in html
    assert "理论 2.400" in html


def test_report_issue_card_without_new_fields_still_renders():
    frame = {"t": 3.0, "x": 1.0, "y": 2.0, "yaw": 0.0, "speed": 0.4, "lookahead": 1.5}
    html = R._issues_section(None, [{"frame": frame, "level": "mid",
                                     "kind": "k", "why": "w"}])
    assert "前视档位" not in html and "理论" not in html


def test_report_timeseries_caption_present():
    frames = [{"t": float(i), "x": 0, "y": 0, "yaw": 0, "speed": 0.4,
               "lookahead": 1.5 + 0.1 * i} for i in range(5)]
    run = {"state": "done", "frames": frames, "metrics": {}, "req": {"traj_json": {}},
           "record": None, "error": None, "param_changes": [], "param_plan": [],
           "path": []}
    html = R.build_report("cap", run)
    assert "2v × lookahead_time" in html
