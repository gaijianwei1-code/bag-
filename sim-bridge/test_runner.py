"""T2.2/T2.5 冒烟：runner v2（bag_replay 模式）各组件。

真实容器调用用 marker 标记，默认跳过。
跑法：./.venv/bin/python -m pytest test_runner.py -v
"""
import os
import subprocess
import sys
import tempfile

import pytest

sys.path.insert(0, os.path.dirname(__file__))
import runner as R  # noqa: E402

REPO = R.BASELINE_REPO


def _venue():
    return {
        "venue_id": "test-venue", "name": "测试场地", "duration_s": 90,
        "container": {
            "map_dir": "/root/catkin_ws/scenes/scenario_x",
            "bag": "/root/catkin_ws/runs/T08/evidence.bag",
            "world": "/root/catkin_ws/scenes/scenario_x_world/world.pcd",
        },
    }


# ---------- bag_replay 参数构建 ----------
def test_build_replay_args_basic():
    args, base, cand = R.build_replay_args(_venue(), {}, "run1")
    joined = " ".join(args)
    assert "--mode closed" in joined
    assert "--bag /root/catkin_ws/runs/T08/evidence.bag" in joined
    assert "--session-id run1" in joined
    assert base == "baseline_branchB"  # 默认
    assert cand == "baseline_branchB"  # 未指定时相同


def test_build_replay_args_scene_required():
    """bag_replay 在 bag 缺 /odom/imu 时必须有 --scene"""
    args, _, _ = R.build_replay_args(_venue(), {}, "run1")
    assert "--scene" in args, "bag_replay 必须带 --scene（bag 缺 odom/imu 时由场景提供）"


def test_build_replay_args_missing_bag():
    v = _venue()
    del v["container"]["bag"]
    with pytest.raises(R.RunError) as e:
        R.build_replay_args(v, {}, "run1")
    assert e.value.code == "VENUE_CONTAINER_PATHS_MISSING"


def test_build_replay_args_duration():
    args, _, _ = R.build_replay_args(_venue(), {"duration_s": 30}, "run1")
    assert "--duration 30" in " ".join(args)


def test_build_replay_args_custom_param_set():
    args, base, cand = R.build_replay_args(_venue(), {"param_set": "my_set"}, "run1")
    assert "my_set" in " ".join(args)
    assert cand == "my_set"


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
    p = R.extract_init_pose("/root/catkin_ws/runs/T08/evidence.bag")
    assert p["samples"] >= 3
    assert abs(p["x"] - 0.556011438369751) < 1e-6


@pytest.mark.docker
def test_extract_init_pose_missing_topic():
    with pytest.raises(R.RunError) as e:
        R.extract_init_pose("/root/catkin_ws/runs/T05/evidence.bag")
    assert e.value.code in ("INIT_POSE_MISSING", "INIT_POSE_FAILED")
