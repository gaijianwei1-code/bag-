"""孤儿仿真进程清理的 pattern 覆盖（2026-11-26 bag_replay_diff.py 挂死事故）。

跑法：./.venv/bin/python -m pytest test_stale_procs.py -v
"""
import runner


def test_stale_patterns_cover_replay_and_diff():
    """两臂回放与对比差分的脚本名都必须在清理名单里。

    事故：bag_replay_diff.py 在两臂跑完后挂死不退，4.6 GB 内存，
    compare.json 从未生成；原 pattern 只含 bag_replay.py，清不掉它。
    """
    pats = " ".join(runner._STALE_PATTERNS)
    assert "bag_replay.py" in pats
    assert "bag_replay_diff.py" in pats
    assert "b_line_run.py" in pats
    assert "b_line_analyze.py" in pats


def test_stale_patterns_cover_recorders():
    """录包与 ros 主节点必须在名单里（防 373 GB evidence.bag 事故重演）。"""
    pats = " ".join(runner._STALE_PATTERNS)
    for p in ("rosbag/record", "roslaunch", "rosmaster", "virtual_lidar", "fake_chassis"):
        assert p in pats, p
