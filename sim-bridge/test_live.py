"""T2.3 冒烟：live_adapter（协议转换 + 前视筛选）+ live_stream（行解析）。

跑法：./.venv/bin/python -m pytest test_live.py -v
"""
import json
import math
import os
import sys

import pytest

sys.path.insert(0, os.path.dirname(__file__))
from live_adapter import (TimelineAdapter, front_hits_from_points,  # noqa: E402
                          first_hit_dist, yaw_diff)


# ---------- 前视筛选（与 L0 语义对齐） ----------
def test_front_hits_basic_cone():
    """正前方 1 m 有点 → 命中；正后方 → 不命中"""
    pts = [[1.0, 0.0, 0.0], [-1.0, 0.0, 0.0]]
    hits = front_hits_from_points(pts, 0.0, 0.0, 0.0, front_dist=3.0, half_angle=0.5)
    assert len(hits) == 1
    assert abs(hits[0][0] - 1.0) < 1e-6


def test_front_hits_outside_angle():
    """侧方 90° 的点在窄视锥外"""
    pts = [[0.0, 1.0, 0.0]]
    hits = front_hits_from_points(pts, 0.0, 0.0, 0.0, front_dist=3.0, half_angle=0.3)
    assert hits == []


def test_front_hits_distance_limit():
    pts = [[5.0, 0.0, 0.0]]
    hits = front_hits_from_points(pts, 0.0, 0.0, 0.0, front_dist=3.0)
    assert hits == []


def test_front_hits_sorted_and_capped():
    """按距离排序，超上限截断到 50"""
    pts = [[float(i) * 0.01 + 0.1, 0.0, 0.0] for i in range(100)]
    hits = front_hits_from_points(pts, 0.0, 0.0, 0.0, front_dist=3.0)
    assert len(hits) == 50
    ds = [math.hypot(h[0], h[1]) for h in hits]
    assert ds == sorted(ds), "命中点必须按距离升序"


def test_front_hits_yaw_rotation():
    """yaw 旋转后，正前方应命中 +y 方向的点"""
    pts = [[0.0, 2.0, 0.0]]
    hits = front_hits_from_points(pts, 0.0, 0.0, math.pi / 2, front_dist=3.0)
    assert len(hits) == 1


def test_first_hit_dist():
    assert first_hit_dist([], 0, 0) is None
    assert first_hit_dist([[1.5, 0.0], [2.0, 0.0]], 0.0, 0.0) == 1.5


def test_yaw_diff_wraps():
    assert abs(yaw_diff(math.pi - 0.1, -math.pi + 0.1) - 0.2) < 1e-9 \
        or abs(yaw_diff(math.pi - 0.1, -math.pi + 0.1) + 0.2) < 1e-9


# ---------- 适配器：live_bridge → timeline ----------
def test_adapter_hello():
    a = TimelineAdapter()
    msgs = a.feed({"type": "hello", "duration": 75, "line": "B"})
    assert msgs[0]["type"] == "status"
    assert "75" in msgs[0]["msg"]


def test_adapter_state_to_frame():
    a = TimelineAdapter()
    msgs = a.feed({"type": "state", "t": 1.5, "x": 1.0, "y": 2.0, "yaw": 0.3,
                   "speed": 0.6, "progress": 42.5})
    frame = next(m for m in msgs if m["type"] == "frame")
    assert frame["t"] == 1.5 and frame["x"] == 1.0 and frame["speed"] == 0.6
    prog = next(m for m in msgs if m["type"] == "status")
    assert abs(prog["progress"] - 0.425) < 1e-9, "progress 百分比须转 0-1"


def test_adapter_state_without_pose_ignored():
    a = TimelineAdapter()
    assert a.feed({"type": "state", "speed": 0.5}) == []


def test_adapter_points_downsampled():
    """命中消息按 hit_every 降频（points 2.5Hz → 命中 ~0.8Hz）"""
    a = TimelineAdapter(hit_every=3)
    a.feed({"type": "state", "t": 0, "x": 0, "y": 0, "yaw": 0})
    outs = [a.feed({"type": "points", "t": i, "pts": [[1.0, 0.0, 0.0]]})
            for i in range(6)]
    hits = [m for o in outs for m in o if m["type"] == "front_hits"]
    assert len(hits) == 2, f"6 帧 points 应产出 2 条命中（每 3 帧），实际 {len(hits)}"


def test_adapter_backfills_first_hit_dist():
    a = TimelineAdapter(hit_every=1)
    a.feed({"type": "state", "t": 0, "x": 0, "y": 0, "yaw": 0})
    msgs = a.feed({"type": "points", "t": 1, "pts": [[2.0, 0.0, 0.0]]})
    hit = next(m for m in msgs if m["type"] == "front_hits")
    assert hit["first_hit_dist"] == 2.0


def test_adapter_path_and_obstacles_pass_through():
    a = TimelineAdapter()
    p = a.feed({"type": "path", "path": [[0, 0], [1, 1]]})
    assert p[0]["type"] == "path" and len(p[0]["path"]) == 2
    o = a.feed({"type": "obstacles", "obstacles": [{"id": 1, "x": 0, "y": 0}]})
    assert o[0]["type"] == "obstacles"


def test_adapter_unknown_type_ignored():
    a = TimelineAdapter()
    assert a.feed({"type": "weird"}) == []
    assert a.feed({}) == []


# ---------- 契约：消息类型白名单 ----------
def test_all_emitted_types_in_schema():
    """所有产出的消息类型必须在 T0.1 契约 + 扩展白名单内"""
    from live_stream import FRAME_TYPES
    a = TimelineAdapter(hit_every=1)
    seq = [{"type": "hello", "line": "B"}, 
           {"type": "state", "t": 0, "x": 0, "y": 0, "yaw": 0, "progress": 10},
           {"type": "points", "t": 1, "pts": [[1, 0, 0]]},
           {"type": "path", "path": [[0, 0]]},
           {"type": "obstacles", "obstacles": []}]
    for obj in seq:
        for m in a.feed(obj):
            assert m["type"] in FRAME_TYPES, f"未登记类型: {m['type']}"


# ---------- 真实容器（默认跳过） ----------
@pytest.mark.docker
def test_live_stream_reports_no_master():
    """无 ROS master 时（未起回放）：live_bridge 应优雅报 error 而非崩溃/静默。

    这是真实边界：直播流的前提是 b_line_run 已在起 ROS master。
    """
    from live_stream import LiveStream
    got = []
    s = LiveStream("smoke_t23_nomaster", duration=3, on_message=got.append)
    try:
        s.start()
        import time
        deadline = time.time() + 80
        while time.time() < deadline and not any(
                m["type"] == "status" and m.get("state") == "error" for m in got):
            time.sleep(1.0)
    finally:
        s.stop()
    assert any(m.get("state") == "error" for m in got), \
        f"无 master 时应报 error；实际收到 {len(got)} 条: {got[:3]}"


@pytest.mark.docker
def test_live_stream_with_real_replay():
    """端到端：起一次真实回放 + live_bridge，须收到 hello + state 帧。

    耗时数分钟，仅在显式 `pytest -m docker` 时跑。
    """
    import runner
    import venues as V
    from live_stream import LiveStream

    vdir = os.path.join(os.path.dirname(__file__), "data")
    venue = V.load_venue(os.path.join(vdir, "venues", "scenario-1793346971223.yaml"))
    got = []
    run_id = "smoke_t23_e2e"
    live = LiveStream(run_id, duration=40, on_message=got.append)
    live.start()
    try:
        runner.submit(venue, {"branch": "main", "duration_s": 40}, run_id)
    finally:
        live.stop()
    assert any("live_bridge" in m.get("msg", "") for m in got), "未收到 hello"
    states = [m for m in got if m["type"] == "frame"]
    assert len(states) >= 10, f"直播帧过少: {len(states)}"
