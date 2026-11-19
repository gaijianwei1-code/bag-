"""T2.2 冒烟：runner 各组件（worktree / scene 构建 / 位姿提取 / 主工作区保护）。

真实容器调用用 marker 标记，默认跳过（pytest -m docker 才跑）。
跑法：./.venv/bin/python -m pytest test_runner.py -v
"""
import os
import subprocess
import sys
import tempfile

import pytest
import yaml

sys.path.insert(0, os.path.dirname(__file__))
import runner as R  # noqa: E402

REPO = R.BASELINE_REPO


# ---------- scene 构建（纯逻辑，无容器） ----------
def _venue():
    return {
        "venue_id": "test-venue", "name": "测试场地", "duration_s": 90,
        "container": {
            "map_dir": "/root/catkin_ws/scenes/scenario_x",
            "trajectory": "/root/catkin_ws/scenes/scenario_x/traj.json",
            "bag": "/root/catkin_ws/runs/T08/evidence.bag",
            "world": "/root/catkin_ws/scenes/scenario_x_world/world.pcd",
        },
    }


def _init_pose():
    return {"x": 1.5, "y": -2.5, "yaw": 0.25, "samples": 98, "window_s": 5.0}


def test_build_scene_basic():
    s = R.build_scene(_venue(), {}, _init_pose(), "run1")
    assert s["map_dir"] == "/root/catkin_ws/scenes/scenario_x"
    assert s["line"] == "B"
    # 关键回归锁：bag 只用于提取初始位姿，绝不能进 scene 传给 launch
    # （2026-11-19 实测：传 bag 会让 planner 绕过 computeVelocityCommands）
    assert "bag" not in s, "scene 不得包含 bag 字段（会切换成回放位姿模式）"
    assert s["trajectory"].endswith("traj.json")


def test_build_scene_init_pose_from_bag():
    """既定决定 2：初始位姿必须来自 bag 提取，且落在 scene 里供 b_line_run 消费"""
    s = R.build_scene(_venue(), {}, _init_pose(), "run1")
    assert s["initial_x"] == 1.5
    assert s["initial_y"] == -2.5
    assert s["initial_yaw"] == 0.25
    assert "中位值" in s["notes"]


def test_build_scene_missing_container_paths():
    v = _venue()
    del v["container"]["map_dir"]
    with pytest.raises(R.RunError) as e:
        R.build_scene(v, {}, _init_pose(), "run1")
    assert e.value.code == "VENUE_CONTAINER_PATHS_MISSING"


def test_build_scene_duration_from_request():
    s = R.build_scene(_venue(), {"duration_s": 30}, _init_pose(), "run1")
    assert s["duration_s"] == 30.0


def test_scene_yaml_roundtrip():
    s = R.build_scene(_venue(), {}, _init_pose(), "run1")
    d = tempfile.mkdtemp()
    p = R.write_scene(s, d)
    back = yaml.safe_load(open(p, encoding="utf-8"))
    assert back["initial_x"] == 1.5
    assert back["scene_id"] == s["scene_id"]


# ---------- worktree（真实 git，但不动主工作区） ----------
def test_worktree_creates_and_cleans():
    if not os.path.isdir(os.path.join(REPO, ".git")):
        pytest.skip("roboneo_sim 仓库不存在")
    before, _ = R.main_workspace_clean()
    wt, commit = R.make_worktree("main")
    try:
        assert os.path.isdir(os.path.join(wt, "sim"))
        assert len(commit) == 40
    finally:
        R.cleanup_worktree(wt)
    assert not os.path.isdir(wt), "worktree 未清理"
    after, porcelain = R.main_workspace_clean()
    assert before == after, f"主工作区状态被改变: {porcelain}"


def test_worktree_bad_branch():
    with pytest.raises(R.RunError) as e:
        R.make_worktree("no-such-branch-xyz")
    assert e.value.code == "BRANCH_NOT_FOUND"


# ---------- 容器实测（默认跳过） ----------
@pytest.mark.docker
def test_extract_init_pose_real():
    """真实 bag → 位姿，须与 b_line_run 硬编码默认值一致（T2.2 验收）"""
    p = R.extract_init_pose("/root/catkin_ws/runs/T08/evidence.bag")
    assert p["samples"] >= 3
    assert abs(p["x"] - 0.556011438369751) < 1e-6
    assert abs(p["y"] - 0.025900661945343018) < 1e-6
    assert abs(p["yaw"] - (-0.0005394914218578743)) < 1e-6


@pytest.mark.docker
def test_extract_init_pose_missing_topic():
    """bag 无 /ndt/current_pose → INIT_POSE_MISSING，不静默跑错"""
    with pytest.raises(R.RunError) as e:
        R.extract_init_pose("/root/catkin_ws/runs/T05/evidence.bag")
    assert e.value.code in ("INIT_POSE_MISSING", "INIT_POSE_FAILED")
