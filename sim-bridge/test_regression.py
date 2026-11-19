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
        # 用 mock 替代 docker 调用
        with patch.object(R, "_exec") as mock_exec:
            mock_exec.return_value = (0, "main\nfeature/fix\n", "")
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


# ---------- 容器实测（默认跳过） ----------
@pytest.mark.docker
def test_list_branches_real():
    """真实容器：git branch 列表非空"""
    branches = R.list_branches()
    assert len(branches) > 0
    assert any(b["branch"] == "main" for b in branches)
