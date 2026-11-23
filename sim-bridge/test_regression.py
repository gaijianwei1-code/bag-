"""T4.1 冒烟：回归任务面板的数据层（分支列表、参数集解析、提交验证）。

跑法：./.venv/bin/python -m pytest test_regression.py -v
"""
import os
import sys

import pytest

sys.path.insert(0, os.path.dirname(__file__))
import runner as R  # noqa: E402
import venues as V  # noqa: E402


def _venue():
    return {
        "venue_id": "test-venue", "name": "测试场地", "duration_s": 90,
        "default_param_set": "baseline_branchB",
        "container": {
            "map_dir": "/root/catkin_ws/scenes/scenario_x",
            "bag": "/root/catkin_ws/runs/T08/evidence.bag",
            "world": "/root/catkin_ws/scenes/scenario_x_world/world.pcd",
        },
    }


# ---------- 分支列表（bridge API 层） ----------
def test_branches_api_returns_list():
    """GET /branches 应返回 git branch 列表"""
    from fastapi.testclient import TestClient
    import app as A
    from unittest.mock import patch

    with patch.object(A, "TOKEN", "dev-token"):
        with patch.object(R, "list_branches", return_value=[
            {"branch": "main", "commit": "abc1234", "is_current": True},
            {"branch": "feature/fix", "commit": "def5678", "is_current": False},
        ]):
            client = TestClient(A.app)
            r = client.get("/branches", headers={"X-Bridge-Token": "dev-token"})
            assert r.status_code == 200
            data = r.json()
            assert len(data) >= 1
            assert data[0]["branch"] == "main"


def test_branches_api_requires_auth():
    from fastapi.testclient import TestClient
    import app as A

    client = TestClient(A.app)
    r = client.get("/branches")
    assert r.status_code == 401


# ---------- 参数集解析 ----------
def test_param_set_changes_parsing():
    """参数集的 changes 列表应能解析为 UI 表格数据"""
    ps = {
        "param_set_id": "t31_tol_1p0",
        "changes": [
            {"file": "schedule_goal.yaml", "key": "next_goal_tolerance_normal",
             "from": 6, "to": 1.0},
            {"file": "schedule_goal.yaml", "key": "next_goal_tolerance_increase",
             "from": 6, "to": 1.0},
        ],
    }
    # 模拟 UI 表格需要的数据结构
    rows = [
        {"file": c["file"], "key": c["key"], "from": c["from"], "to": c["to"]}
        for c in ps["changes"]
    ]
    assert len(rows) == 2
    assert rows[0]["key"] == "next_goal_tolerance_normal"
    assert rows[0]["to"] == 1.0


# ---------- 提交验证 ----------
def test_submit_with_param_overrides():
    """提交 run 带 param_overrides 时应生成参数集副本"""
    args, base, cand = R.build_replay_args(
        _venue(), {"param_set": "my_custom"}, "run1")
    assert "my_custom" in " ".join(args)
    assert cand == "my_custom"


def test_submit_without_param_overrides():
    """不带 param_overrides 时用场地默认参数集"""
    args, base, cand = R.build_replay_args(_venue(), {}, "run1")
    assert base == "baseline_branchB"
    assert cand == "baseline_branchB"


def test_build_replay_args_with_param_overrides_uses_ps_run_id():
    """带 param_overrides 或 param_changes 时 candidate 参数集切到 ps_{run_id}"""
    args, base, cand = R.build_replay_args(
        _venue(), {"param_overrides": {"next_goal_tolerance_normal": 1.5}}, "run_xyz")
    assert base == "baseline_branchB"
    assert cand == "ps_run_xyz"
    assert "ps_run_xyz" in " ".join(args)

    args2, base2, cand2 = R.build_replay_args(
        _venue(),
        {"param_changes": [{"file": "schedule_goal.yaml", "key": "next_goal_tolerance_normal", "to": 1.2}]},
        "run_abc")
    assert base2 == "baseline_branchB"
    assert cand2 == "ps_run_abc"
    assert "ps_run_abc" in " ".join(args2)


def test_cancel_active_and_persist(tmp_path, monkeypatch):
    """cancel_active 应将活跃 run 置为 failed(CANCELLED) 并释放单并发锁"""
    from fastapi.testclient import TestClient
    from unittest.mock import patch
    import app as A

    monkeypatch.setattr(A, "RUNS_DIR", str(tmp_path / "runs"))
    monkeypatch.setattr(A, "_runs", {
        "r1": {"state": "replaying", "progress": 0.3, "frames": [], "metrics": None, "error": None}
    })
    with patch.object(A, "TOKEN", "dev-token"), patch.object(R, "cancel_active_run") as m_cancel:
        client = TestClient(A.app)
        res = client.post("/runs/cancel_active", headers={"X-Bridge-Token": "dev-token"})
        assert res.status_code == 200
        assert res.json()["cancelled"] == ["r1"]
        assert m_cancel.called
        assert A._runs["r1"]["state"] == "failed"
        assert A._runs["r1"]["error"]["code"] == "CANCELLED"
        A._runs.clear()
        g = client.get("/runs/r1", headers={"X-Bridge-Token": "dev-token"})
        assert g.status_code == 200
        assert g.json()["error"]["code"] == "CANCELLED"


def test_prepare_param_set_with_overrides_valid_python():
    """参数覆盖脚本必须是合法 Python，写入 param_set.yaml 并用 sim_common.params 预校验"""
    import shlex
    from unittest.mock import patch

    captured = []
    with patch.object(R, "_exec", side_effect=lambda cmd, **kw: (captured.append(cmd) or (0, "", ""))):
        res = R._prepare_param_set_with_overrides(
            "t31_tol_1p0",
            "ps_test_1",
            overrides={"lookahead_dist": 2.0},
            changes=[{
                "file": "schedule_goal.yaml",
                "key": "next_goal_tolerance_normal",
                "from": 6,
                "to": 1.5,
            }],
        )

    assert res["param_set_id"] == "ps_test_1"
    assert len(captured) == 1
    parts = shlex.split(captured[0])
    assert parts[:2] == ["python3", "-c"]
    script = parts[2]
    # 能通过 compile 即无 SyntaxError
    compile(script, "<param_override>", "exec")
    assert "src = '/root/catkin_ws/sim/param_sets/t31_tol_1p0'" in script
    assert "dst = '/root/catkin_ws/sim/param_sets/ps_test_1'" in script
    assert "param_set.yaml" in script
    assert "params.load_param_set" in script


def test_list_runs_and_get_run_param_backfill(tmp_path, monkeypatch):
    """GET /runs 返回历史列表，GET /runs/{id} 应返回 param_set_id/param_changes/param_plan 以供回填"""
    import json
    from fastapi.testclient import TestClient
    from unittest.mock import patch
    import app as A

    runs_dir = tmp_path / "runs"
    d1 = runs_dir / "v_20261123_110000_aaaa"
    d1.mkdir(parents=True)
    changes_sample = [
        {"file": "schedule_goal.yaml", "key": "next_goal_tolerance_normal", "from": 6, "to": 1.5},
        {"file": "sweep_base/local_planner_params.yaml", "key": "PursuitLocalPlanner.lookahead_dist", "from": 1.5, "to": 2.0},
    ]
    plan_sample = [
        {"file": "schedule_goal.yaml", "rosparam": "/schedule_goal_node/next_goal_tolerance_normal", "value": 1.5},
        {"file": "sweep_base/local_planner_params.yaml", "rosparam": "/sweep_base/PursuitLocalPlanner/lookahead_dist", "value": 2.0},
    ]
    (d1 / "run.json").write_text(json.dumps({
        "state": "done",
        "progress": 1.0,
        "frames": [],
        "metrics": {"travel_m": 6.65},
        "req": {"venue_id": "v", "branch": "main", "duration_s": 76, "param_changes": changes_sample},
        "param_set_id": "ps_v_20261123_110000_aaaa",
        "param_changes": changes_sample,
        "param_plan": plan_sample,
        "error": None,
        "msg": "完成",
    }), encoding="utf-8")

    monkeypatch.setattr(A, "RUNS_DIR", str(runs_dir))
    monkeypatch.setattr(A, "_runs", {})
    with patch.object(A, "TOKEN", "dev-token"):
        client = TestClient(A.app)
        lst = client.get("/runs", headers={"X-Bridge-Token": "dev-token"})
        assert lst.status_code == 200
        items = lst.json()
        assert len(items) == 1
        assert items[0]["run_id"] == "v_20261123_110000_aaaa"
        assert items[0]["param_set_id"] == "ps_v_20261123_110000_aaaa"
        assert items[0]["param_changes_count"] == 2
        assert items[0]["travel_m"] == 6.65

        detail = client.get("/runs/v_20261123_110000_aaaa", headers={"X-Bridge-Token": "dev-token"})
        assert detail.status_code == 200
        dj = detail.json()
        assert dj["param_set_id"] == "ps_v_20261123_110000_aaaa"
        assert len(dj["param_changes"]) == 2
        assert dj["param_plan"][1]["rosparam"] == "/sweep_base/PursuitLocalPlanner/lookahead_dist"


# ---------- 容器实测（默认跳过） ----------
@pytest.mark.docker
def test_list_branches_real():
    """真实容器：git branch 列表非空"""
    branches = R.list_branches()
    assert len(branches) > 0
    assert any(b["branch"] == "main" for b in branches)
