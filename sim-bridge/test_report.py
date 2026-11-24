"""T5.1 报告导出的单元测试。

覆盖点（都不依赖 docker / ROS / 网络）：
1. PNG 编码自洽（IHDR/IDAT 可解回原像素）；
2. PGM 解析与坐标换算自洽（world→px→world 往返）；
3. 8 项门只按 compare.json 原样呈现，缺数据判"无数据"而不是 FAIL；
4. 参数 diff 表能同时吃 param_changes / param_overrides / applied；
5. 问题点位规则可复现（前视命中 / 静默停车 / 数据缺口）；
6. 端到端：给一份带地图的 run 生成 HTML，关键区块齐全、无异常字符；
7. `/runs/{id}/report` 接口返回 HTML 而不是 JSON，且落盘一份。
"""
import base64
import json
import os
import struct
import zlib

import pytest
import yaml
from fastapi.testclient import TestClient

import report as R

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SAMPLE_PGM = os.path.join(REPO_ROOT, "testdata", "scenario_1793346971223",
                          "1793346971223.pgm")
SAMPLE_TRAJ = os.path.join(REPO_ROOT, "testdata", "scenario_1793346971223",
                           "1793347176294.json")
SAMPLE_VENUE = os.path.join(REPO_ROOT, "sim-bridge", "data", "venues",
                            "scenario-1793346971223.yaml")
TOKEN = {"X-Bridge-Token": "dev-token"}

HAS_SAMPLE = os.path.isfile(SAMPLE_PGM) and os.path.isfile(SAMPLE_TRAJ)
needs_sample = pytest.mark.skipif(not HAS_SAMPLE, reason="缺少标准样本地图")


# ---------------------------------------------------------------- fixtures
def _decode_png(png: bytes):
    """最小 PNG 解码：只处理本模块产出的 8-bit 灰度、filter=0。"""
    assert png[:8] == b"\x89PNG\r\n\x1a\n"
    pos, idat, w, h = 8, b"", None, None
    while pos < len(png):
        ln = struct.unpack(">I", png[pos:pos + 4])[0]
        tag = png[pos + 4:pos + 8]
        data = png[pos + 8:pos + 8 + ln]
        pos += 12 + ln
        if tag == b"IHDR":
            w, h, depth, ctype = struct.unpack(">IIBB", data[:10])
            assert (depth, ctype) == (8, 0), "只支持 8-bit 灰度"
        elif tag == b"IDAT":
            idat += data
    raw = zlib.decompress(idat)
    stride = w + 1
    assert len(raw) == stride * h
    rows = []
    for i in range(h):
        seg = raw[i * stride:(i + 1) * stride]
        assert seg[0] == 0, "filter 必须为 0"
        rows.append(seg[1:])
    return w, h, rows


def _fake_run(frames=None, path=None, compare=None, **extra):
    run = {
        "state": "done",
        "progress": 1.0,
        "frames": frames or [],
        "metrics": {"travel_m": 9.9091, "mean_speed": 0.41,
                    "cmd_nonzero": 214, "cmd_count": 215,
                    "end_error_m": 0.01, "edge_lateral_rms": 0.0023},
        "req": {"venue_id": "demo", "bag": "bags/demo.bag",
                "branch": "main", "traj_json": {"points": []},
                "param_changes": []},
        "init_pose": None,
        "record": {"compare": compare} if compare else None,
        "error": None,
        "param_set_id": None,
        "param_changes": [],
        "param_plan": [],
        "segment_info": None,
        "msg": "",
        "path": path or [],
    }
    run.update(extra)
    return run


COMPARE_FULL = {
    "screening": {"verdict": "EFFECT"},
    "fidelity": {
        "verdict": "REPLAY_FIDELITY_OK",
        "checks": {"tracking_rms": True, "tracking_signed_mean": True,
                   "clearance_min_diff": True, "travel_diff": False,
                   "speed_diff": True, "planner_mean_dev": True,
                   "theta_rms": True, "collision": True},
        "tracking": {"truth": {"rms": 0.0211},
                     "baseline": {"rms": 0.0085, "mean_signed": 0.0038}},
        "clearance": {"truth": {"brush_clearance": {"min": 0.135}},
                      "baseline": {"brush_clearance": {"min": 0.128}}},
        "travel": {"truth_m": 25.17, "baseline_m": 24.76, "delta_pct": -1.63},
        "speed": {"truth": {"speed_mean": 0.4105},
                  "baseline": {"speed_mean": 0.4569},
                  "mean_delta_pct": 11.29},
        "planner_vs_truth": {"matched": 469,
                             "mean_deviation_m": {"mean": 0.026}},
        "theta_vs_truth": {"rms": 0.011},
        "collisions": {"mentions": 0, "nonzero": 0},
        "pose_delta_m": {"median": 0.2, "mean": 0.21, "p95": 0.34},
    },
    "closed_loop": {
        "verdict": "OK",
        "summary": {"progress_peak": 100.0, "terminal": True,
                    "edge_rms": 0.1098, "collision": False,
                    "travel_m": 24.753, "failure_codes": []},
        "checks": {"progress_100": True, "terminal": True, "collision": True,
                   "edge_rms": False, "brush_min": True,
                   "no_failure_codes": True},
    },
    "applied": {"status": "ok",
                "rows": [{"name": "inflation_radius", "actual": "0.9",
                          "expected": "0.9"}]},
    "verdict": {"overall": "FIDELITY_DRIFT"},
}


# ---------------------------------------------------------------- PNG
def test_png_roundtrip():
    rows = [bytes([(i * 7) % 256 for i in range(23)]) for _ in range(11)]
    png = R.png_gray(23, 11, rows)
    w, h, out = _decode_png(png)
    assert (w, h) == (23, 11)
    assert out == rows


def test_png_data_uri_prefix():
    uri = R.png_data_uri(4, 2, [b"\x00\x01\x02\x03", b"\x04\x05\x06\x07"])
    assert uri.startswith("data:image/png;base64,")
    raw = base64.b64decode(uri.split(",", 1)[1])
    assert raw[:8] == b"\x89PNG\r\n\x1a\n"


# ---------------------------------------------------------------- PGM
@needs_sample
def test_pgm_world_px_roundtrip():
    m = R.PgmMap(SAMPLE_PGM, (-22.796825, -77.406677), 0.05)
    assert m.w > 0 and m.h > 0
    for x, y in ((0.0, 0.0), (10.0, -5.0), (-20.0, 30.0)):
        col, row = m.world_to_px(x, y)
        wx = m.ox + col * m.res
        wy = m.oy + (m.h - 1 - row) * m.res
        assert abs(wx - x) < 1e-6 and abs(wy - y) < 1e-6


@needs_sample
def test_pgm_crop_maps_back_to_world():
    m = R.PgmMap(SAMPLE_PGM, (-22.796825, -77.406677), 0.05)
    crop = m.crop(-10.0, -10.0, 10.0, 10.0, step=4)
    assert crop.w > 0 and crop.h > 0
    px, py = crop.to_px(0.0, 0.0)
    wx, wy = crop.px_to_world(px, py)
    assert abs(wx - 0.0) < crop.m_per_px and abs(wy - 0.0) < crop.m_per_px


@needs_sample
def test_pgm_rejects_ascii():
    bad = _write_tmp(b"P2\n2 2\n255\n0 0 0 0\n")
    with pytest.raises(ValueError):
        R.PgmMap(bad, (0, 0), 0.05)


def _write_tmp(data: bytes, suffix=".pgm") -> str:
    import tempfile
    fd, path = tempfile.mkstemp(suffix=suffix)
    with os.fdopen(fd, "wb") as f:
        f.write(data)
    return path


# ---------------------------------------------------------------- 门禁抽取
def test_gates_present_and_mapped():
    g = R.extract_gates({"compare": COMPARE_FULL})
    v = g["values"]
    assert v["tracking_rms"] == pytest.approx(0.0085)
    assert v["clearance_min_diff"] == pytest.approx(0.128 - 0.135)
    assert v["travel_diff"] == pytest.approx(-1.63)
    assert v["speed_diff"] == pytest.approx(11.29)
    assert v["planner_mean_dev"] == pytest.approx(0.026)
    assert v["theta_rms"] == pytest.approx(0.011)
    assert v["collision"] == 0
    assert g["verdict"] == "REPLAY_FIDELITY_OK"
    assert g["overall"] == "FIDELITY_DRIFT"


def test_gates_missing_compare_is_all_none():
    g = R.extract_gates({})
    assert all(v is None for v in g["values"].values())
    assert g["checks"] == {}
    assert g["verdict"] is None


def test_gates_missing_value_not_reported_as_fail():
    """老 compare.json 没有 not_computable，checks 会给 False；
    值为 None 时必须呈现"无数据"，不能冤枉仿真失败。"""
    cmp = {"fidelity": {"checks": {"theta_rms": False},
                        "theta_vs_truth": None}}
    html = R._gates_table(R.extract_gates({"compare": cmp}))
    row = [ln for ln in html.split("<tr")
           if "转角 RMS" in ln][0]
    assert "无数据" in row
    assert "FAIL" not in row


def test_gates_not_computable_marks_na():
    cmp = {"fidelity": {
        "checks": {"tracking_rms": False, "collision": True},
        "not_computable": ["tracking_rms"],
        "tracking": {"baseline": {"rms": 0.005}},
        "collisions": {"nonzero": 0},
    }}
    g = R.extract_gates({"compare": cmp})
    html = R._gates_table(g)
    row = [ln for ln in html.split("<tr") if "轨迹跟踪 RMS" in ln][0]
    assert "无数据" in row


def test_gates_boundary_waiver_surfaced():
    cmp = {"fidelity": {
        "checks": {"theta_rms": False},
        "theta_vs_truth": {"rms": 0.0221},
        "not_computable": [],
        "known_boundary": {"waived_checks": ["theta_rms"],
                           "reason": "truth 自身跟踪更松"},
    }}
    run = _fake_run(compare=cmp)
    html = R.build_report("r1", run)
    assert "已知边界豁免" in html
    assert "truth 自身跟踪更松" in html


# ---------------------------------------------------------------- 参数 diff
def test_param_table_changes():
    run = _fake_run(param_changes=[
        {"file": "a.yaml", "key": "lookahead_dist", "from": 1.5, "to": 1.2}])
    html = R._param_table(run, {"applied": {}})
    assert "lookahead_dist" in html and "a.yaml" in html
    assert "参数 diff" in html


def test_param_table_overrides_only():
    run = _fake_run()
    run["req"]["param_overrides"] = {"publish_odom_imu": True}
    html = R._param_table(run, {"applied": {}})
    assert "param_overrides" in html and "publish_odom_imu" in html


def test_param_table_applied_mismatch_flagged():
    g = {"applied": {"rows": [{"name": "inflation_radius", "actual": "0.9",
                              "expected": "0.5"}]}}
    html = R._param_table(_fake_run(), g)
    assert "inflation_radius" in html
    assert "FAIL" in html


def test_param_table_none_declared():
    html = R._param_table(_fake_run(), {"applied": {}})
    assert "未修改任何参数" in html


# ---------------------------------------------------------------- 问题点位
def _frames(n=30, speed=0.4, carrot=True):
    return [{"t": i * 0.2, "x": i * 0.1, "y": 0.0, "yaw": 0.0,
             "speed": speed, "lookahead": 0.5,
             "carrot": [i * 0.1 + 0.5, 0.0] if carrot else None,
             "front_hits": None} for i in range(n)]


def test_issue_front_hit_detected():
    fs = _frames()
    fs[10]["front_hits"] = [[1.0, 0.0], [1.1, 0.1]]
    issues = R.find_issues(fs)
    kinds = [i["kind"] for i in issues]
    assert "front_hit" in kinds
    hit = [i for i in issues if i["kind"] == "front_hit"][0]
    assert hit["level"] == "high"


def test_issue_front_hit_far_ignored():
    fs = _frames()
    fs[10]["front_hits"] = [[2.9, 0.0]]     # 2.9 m 之外，不算危险
    assert not [i for i in R.find_issues(fs) if i["kind"] == "front_hit"]


def test_issue_stopped_detected():
    fs = _frames()
    for f in fs[12:]:
        f["speed"] = 0.0
    issues = R.find_issues(fs)
    assert [i for i in issues if i["kind"] == "stopped"]


def test_issue_gap_detected():
    fs = _frames(carrot=False)
    for f in fs:
        f["lookahead"] = None
    issues = R.find_issues(fs)
    assert [i for i in issues if i["kind"] == "gap"]


def test_issue_tail_fallback_when_clean():
    fs = _frames()
    issues = R.find_issues(fs)
    assert issues and issues[-1]["kind"] == "tail"
    assert issues[-1]["level"] == "low"


def test_issue_dedup_by_interval():
    fs = _frames(n=40)
    for f in fs:
        f["front_hits"] = [[f["x"] + 0.2, 0.0]]   # 每帧都命中
    hits = [i for i in R.find_issues(fs) if i["kind"] == "front_hit"]
    assert 1 <= len(hits) <= 3
    ts = [i["t"] for i in hits]
    assert all(b - a >= 1.0 for a, b in zip(ts, ts[1:]))


# ---------------------------------------------------------------- SVG
def test_svg_vehicle_scales_with_resolution():
    a = R._svg_vehicle(100, 100, 0.0, 0.05)
    b = R._svg_vehicle(100, 100, 0.0, 0.5)
    wa = float(a.split('width="')[1].split('"')[0])
    wb = float(b.split('width="')[1].split('"')[0])
    assert wa > wb


def test_svg_vehicle_has_min_floor():
    s = R._svg_vehicle(10, 10, 0.0, 100.0)   # 极端粗分辨率
    assert 'width="' in s and float(s.split('width="')[1].split('"')[0]) >= 3.0


def test_timeseries_skips_all_none():
    frames = [{"t": i, "speed": None} for i in range(5)]
    html = R.render_timeseries(frames, "speed", "#000", "速度", "m/s")
    assert "无可用数据" in html


def test_timeseries_flat_series_ok():
    """全常数序列（车静止）不能让除零把图表搞崩。"""
    frames = [{"t": i, "speed": 0.0} for i in range(5)]
    html = R.render_timeseries(frames, "speed", "#000", "速度", "m/s")
    assert "<svg" in html and "d=\"M" in html


# ---------------------------------------------------------------- 端到端 HTML
@needs_sample
def test_build_report_with_map():
    venue = yaml.safe_load(open(SAMPLE_VENUE, "r", encoding="utf-8"))
    traj = json.load(open(SAMPLE_TRAJ, "r", encoding="utf-8"))
    pts = traj.get("points") or []
    frames = [{"t": i * 0.2, "x": p["x"], "y": p["y"],
               "yaw": p.get("yaw", 0.0), "speed": 0.4,
               "lookahead": 0.5, "carrot": [p["x"], p["y"]],
               "front_hits": None,
               "first_hit_dist": None} for i, p in enumerate(pts[:40])]
    path = [[p["x"], p["y"]] for p in pts]
    run = _fake_run(frames=frames, path=path, compare=COMPARE_FULL)
    run["req"]["venue_id"] = venue["venue_id"]

    html = R.build_report("demo_run", run, venue=venue, base_dir=REPO_ROOT)

    assert html.startswith("<!DOCTYPE html>")
    assert html.rstrip().endswith("</html>")
    for token in ("8 项 F 门指标对照", "轨迹对比", "问题点位帧截图",
                  "参数 diff", "数据溯源", "验收签字"):
        assert token in html
    assert "data:image/png;base64," in html      # 地图内联成功
    assert 'class="overlay"' in html             # 矢量叠加在内
    assert "&lt;" not in html.split("<style>")[0]  # 头部没有转义泄漏


@needs_sample
def test_build_report_filename_safe():
    name = R.report_filename("najd1/20261123 16:55 #1")
    assert "/" not in name and " " not in name and "#" not in name
    assert name.startswith("report_") and name.endswith(".html")


def test_build_report_without_map_still_works():
    run = _fake_run(frames=_frames(5))
    html = R.build_report("no_map", run)
    assert "未找到场地地图" in html or "轨迹图渲染失败" in html
    assert "8 项 F 门" in html


def test_build_report_failed_run_shows_error():
    run = _fake_run()
    run["state"] = "failed"
    run["error"] = {"code": "VENUE_CONTAINER_PATHS_MISSING",
                    "msg": "缺 container.bag"}
    html = R.build_report("bad", run)
    assert "VENUE_CONTAINER_PATHS_MISSING" in html
    assert "缺 container.bag" in html
    assert "门禁数据" in html       # 明确提示 8 项门不可用


def test_build_report_escapes_html_injection():
    run = _fake_run()
    run["msg"] = "<script>alert(1)</script>"
    run["req"]["bag"] = "<img src=x onerror=alert(1)>.bag"
    html = R.build_report("xss", run)
    assert "<script>alert(1)</script>" not in html
    assert "&lt;script&gt;" in html


# ---------------------------------------------------------------- 接口
@pytest.fixture
def client(tmp_path, monkeypatch):
    import app as A
    monkeypatch.setattr(A, "RUNS_DIR", str(tmp_path))
    monkeypatch.setattr(A, "VENUES_DIR", str(tmp_path / "venues"))
    A._runs.clear()
    return TestClient(A.app), A, tmp_path


def test_report_endpoint_returns_html(client):
    c, A, tmp = client
    A._runs["r1"] = _fake_run(frames=_frames(6))
    r = c.get("/runs/r1/report", headers=TOKEN)
    assert r.status_code == 200
    assert r.headers["content-type"].startswith("text/html")
    assert "<!DOCTYPE html>" in r.text
    # 顺手落盘一份
    assert os.path.isfile(os.path.join(str(tmp), "r1", "report.html"))


def test_report_endpoint_download_header(client):
    c, A, _ = client
    A._runs["r2"] = _fake_run(frames=_frames(3))
    r = c.get("/runs/r2/report?download=1", headers=TOKEN)
    assert r.status_code == 200
    assert "attachment" in r.headers.get("content-disposition", "")
    assert ".html" in r.headers.get("content-disposition", "")


def test_report_endpoint_404(client):
    c, A, _ = client
    r = c.get("/runs/nope/report", headers=TOKEN)
    assert r.status_code == 404


def test_report_endpoint_requires_token(client):
    c, A, _ = client
    A._runs["r3"] = _fake_run()
    assert c.get("/runs/r3/report").status_code == 401


def test_report_endpoint_loads_from_disk(client):
    """已落盘的 run 也要能出报告（服务重启后现场补票）。"""
    c, A, tmp = client
    d = tmp / "old_run"
    d.mkdir()
    run = _fake_run(frames=_frames(4))
    (d / "run.json").write_text(json.dumps(run), encoding="utf-8")
    r = c.get("/runs/old_run/report", headers=TOKEN)
    assert r.status_code == 200
    assert "old_run" in r.text
