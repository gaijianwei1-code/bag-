"""T4.2 L2 分段重跑单元测试（纯函数 + API 契约）。"""
import os
import sys

import pytest

sys.path.insert(0, os.path.dirname(__file__))

import runner  # noqa: E402


def _make_line_points(n=50, step=0.5):
    return [{"x": round(i * step, 4), "y": 0.0, "yaw": 0.0} for i in range(n)]


def test_normalize_segment_index_and_swap():
    seg = runner.normalize_segment({"from_idx": 30, "to_idx": 15, "pad_points": 4})
    assert seg == {"mode": "index", "from_idx": 15, "to_idx": 30, "pad_points": 4}


def test_normalize_segment_time():
    seg = runner.normalize_segment({"from_t": 10.5, "to_t": 22.0})
    assert seg == {"mode": "time", "from_t": 10.5, "to_t": 22.0, "pad_s": 3.0}


def test_normalize_segment_empty():
    assert runner.normalize_segment(None) is None
    assert runner.normalize_segment({}) is None


def test_compute_segment_slice_index_with_safe_pad():
    pts = _make_line_points(50, step=0.5)
    res = runner.compute_segment_slice(
        pts, {"from_idx": 20, "to_idx": 35, "pad_points": 4}
    )
    assert res["from_idx"] == 20
    assert res["to_idx"] == 35
    assert res["safe_start_idx"] == 16
    assert res["safe_end_idx"] == 35
    assert res["point_count"] == 20
    assert abs(res["arc_len_m"] - 9.5) < 0.01
    assert res["init_pose"]["x"] == 8.0
    assert res["init_pose"]["y"] == 0.0
    assert 12 <= res["auto_duration_s"] <= 120


def test_compute_segment_slice_clamps_to_zero():
    pts = _make_line_points(50, step=0.5)
    res = runner.compute_segment_slice(
        pts, {"from_idx": 2, "to_idx": 10, "pad_points": 5}
    )
    assert res["safe_start_idx"] == 0
    assert res["safe_end_idx"] == 10
    assert res["point_count"] == 11


def test_compute_segment_slice_time_mode():
    pts = _make_line_points(50, step=0.5)
    res = runner.compute_segment_slice(
        pts, {"from_t": 38.0, "to_t": 57.0, "pad_s": 4.0}, total_duration_s=76.0
    )
    assert res["safe_start_idx"] < res["from_idx"] < res["to_idx"]
    assert res["point_count"] >= 2


def test_compute_segment_slice_rejects_short_trajectory():
    with pytest.raises(runner.RunError) as exc:
        runner.compute_segment_slice([{"x": 0.0, "y": 0.0}], {"from_idx": 0, "to_idx": 0})
    assert exc.value.code == "SEGMENT_INVALID"
