"""T2.1 冒烟：venues 模块 + bridge API，每接口 1 正 1 反。跑法：pytest test_bridge.py -v"""
import os
import shutil
import sys
import tempfile

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "sim-bridge"))
import venues as V  # noqa: E402


@pytest.fixture()
def tmp_data():
    d = tempfile.mkdtemp(prefix="blz_bridge_")
    os.makedirs(os.path.join(d, "venues"))
    yield d
    shutil.rmtree(d, ignore_errors=True)


def _venue(vid="test-site-01"):
    return {
        "schema": "venue/v1", "venue_id": vid, "name": "测试场地",
        "created": "2026-11-18", "created_by": "test",
        "map_pgm": "maps/t/map.pgm", "map_pcd": "maps/t/map.pcd",
        "pcd_height_filter": {"min": 0.05, "max": 2.0},
        "default_traj": "trajs/t.json",
        "map_pose_ref": {"origin": {"x": -10.0, "y": -8.0}, "resolution": 0.05},
        "bags": [{"bag": "bags/t.bag", "traj": "trajs/t.json", "duration_s": 76,
                  "notes": ""}],
    }


# ---------- venues.py 单元 ----------
def test_validate_ok(tmp_data):
    V.validate_venue(_venue(), tmp_data, check_files=False)  # 不抛即过

def test_validate_bad_id(tmp_data):
    with pytest.raises(V.VenueError, match="不合法"):
        V.validate_venue(_venue("Bad_ID"), tmp_data, check_files=False)

def test_validate_bad_resolution(tmp_data):
    v = _venue(); v["map_pose_ref"]["resolution"] = 0.5
    with pytest.raises(V.VenueError, match="resolution"):
        V.validate_venue(v, tmp_data, check_files=False)

def test_save_load_roundtrip(tmp_data):
    p = os.path.join(tmp_data, "venues", "t.yaml")
    V.save_venue(p, _venue())
    assert V.load_venue(p)["venue_id"] == "test-site-01"

def test_bag_in_venue():
    v = _venue()
    assert V.bag_in_venue(v, "bags/t.bag")
    assert not V.bag_in_venue(v, "bags/other.bag")


# ---------- FastAPI 接口 ----------
@pytest.fixture()
def client(tmp_data, monkeypatch):
    from fastapi.testclient import TestClient
    import app as A
    monkeypatch.setattr(A, "DATA_DIR", tmp_data)
    monkeypatch.setattr(A, "VENUES_DIR", os.path.join(tmp_data, "venues"))
    monkeypatch.setattr(A, "RUNS_DIR", os.path.join(tmp_data, "runs"))
    monkeypatch.setattr(A, "_runs", {})  # 隔离 run 状态（防测试间泄漏）
    # T2.2：run 提交后异步执行。单测无容器，桩掉执行器让它停在 queued，
    # 否则 run 立即 failed，并发守卫（只拦 queued/replaying/analyzing）失效。
    monkeypatch.setattr(A, "_execute_run", lambda *a, **k: None)
    with TestClient(A.app) as c:
        yield c


H = {"X-Bridge-Token": "dev-token"}


def test_health(client):
    r = client.get("/health").json()
    assert r["status"] == "ok"

def test_auth_reject(client):
    assert client.get("/venues").status_code == 401

def test_venue_create_list(client):
    r = client.post("/venues", json=_venue(), headers=H)
    assert r.status_code == 201
    names = [v["venue_id"] for v in client.get("/venues", headers=H).json()]
    assert "test-site-01" in names

def test_venue_duplicate_rejected(client):
    client.post("/venues", json=_venue(), headers=H)
    assert client.post("/venues", json=_venue(), headers=H).status_code == 409

def test_venue_bad_payload_rejected(client):
    v = _venue(); v["schema"] = "wrong"
    assert client.post("/venues", json=v, headers=H).status_code == 422

def test_venue_bags(client):
    client.post("/venues", json=_venue(), headers=H)
    bags = client.get("/venues/test-site-01/bags", headers=H).json()
    assert bags[0]["bag"] == "bags/t.bag"

def test_venue_bags_404(client):
    assert client.get("/venues/no-such/bags", headers=H).status_code == 404


def _run_req(vid="test-site-01"):
    return {"venue_id": vid, "bag": "bags/t.bag", "branch": "main",
            "traj_json": {"points": []}, "param_overrides": {}}


def test_run_submit_accepted(client):
    client.post("/venues", json=_venue(), headers=H)
    r = client.post("/runs", json=_run_req(), headers=H)
    assert r.status_code == 202
    assert r.json()["run_id"].startswith("test-site-01_")

def test_run_bag_not_in_venue(client):
    client.post("/venues", json=_venue(), headers=H)
    bad = _run_req(); bad["bag"] = "bags/not-registered.bag"
    assert client.post("/runs", json=bad, headers=H).status_code == 422

def test_run_unknown_venue(client):
    assert client.post("/runs", json=_run_req("no-such"), headers=H).status_code == 404

def test_run_map_override_rejected(client):
    """铁律：地图/初始位姿不可经 run 覆盖（venue_schema §3.4）"""
    client.post("/venues", json=_venue(), headers=H)
    bad = _run_req(); bad["param_overrides"] = {"map_dir": "/evil"}
    assert client.post("/runs", json=bad, headers=H).status_code == 422

def test_run_conflict_single_concurrency(client):
    client.post("/venues", json=_venue(), headers=H)
    r1 = client.post("/runs", json=_run_req(), headers=H)
    assert r1.status_code == 202
    r2 = client.post("/runs", json=_run_req(), headers=H)
    assert r2.status_code == 409  # 首版单并发


def test_run_get_and_timeline(client):
    client.post("/venues", json=_venue(), headers=H)
    run_id = client.post("/runs", json=_run_req(), headers=H).json()["run_id"]
    assert client.get(f"/runs/{run_id}", headers=H).json()["state"] == "queued"
    tl = client.get(f"/runs/{run_id}/timeline", headers=H).json()
    assert tl["schema"] == "timeline/v1"

def test_run_get_404(client):
    assert client.get("/runs/nope", headers=H).status_code == 404


def test_ws_stream_pushes_frames(client):
    """WS 契约冒烟：注入两帧 + done → 收 frame、metrics、status 终态"""
    client.post("/venues", json=_venue(), headers=H)
    run_id = client.post("/runs", json=_run_req(), headers=H).json()["run_id"]
    from app import _runs, _runs_lock
    with _runs_lock:
        _runs[run_id]["frames"] = [
            {"t": 1.0, "x": 0.1, "y": 0.2, "yaw": 0.0, "speed": 0.5},
            {"t": 2.0, "x": 0.2, "y": 0.3, "yaw": 0.1, "speed": 0.6}]
        _runs[run_id]["state"] = "done"
        _runs[run_id]["metrics"] = {"tracking_rms": 0.008, "gates": "F 8/8"}
    types = []
    with client.websocket_connect(f"/ws/runs/{run_id}/stream") as ws:
        while True:
            m = ws.receive_json()
            types.append(m["type"])
            if m["type"] == "status" and m["state"] in ("done", "failed"):
                break
    assert types.count("frame") == 2
    assert "metrics" in types

def test_ws_stream_unknown_run(client):
    """契约：不存在的 run → WS 先发 error 帧再关闭（客户端收到 error 即视为失败）"""
    with client.websocket_connect("/ws/runs/nope/stream") as ws:
        m = ws.receive_json()
        assert m["type"] == "error" and m["code"] == "RUN_NOT_FOUND"
