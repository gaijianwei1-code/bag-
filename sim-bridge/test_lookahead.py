"""T3.1 冒烟：lookahead 观测链路（适配器扩展 + 脚本静态检查）。

跑法：./.venv/bin/python -m pytest test_lookahead.py -v
"""
import os
import sys

import pytest

sys.path.insert(0, os.path.dirname(__file__))
from live_adapter import TimelineAdapter  # noqa: E402


def test_adapter_keeps_lookahead():
    """lookahead 消息进适配器后被记住，并出现在后续 frame 里"""
    a = TimelineAdapter()
    assert a.feed({"type": "lookahead", "dist": 1.766, "x": 1.70, "y": 0.48}) == []
    assert a.lookahead == 1.766
    msgs = a.feed({"type": "state", "t": 1.0, "x": 0, "y": 0, "yaw": 0, "speed": 0.5})
    frame = next(m for m in msgs if m["type"] == "frame")
    assert frame["lookahead"] == 1.766, "frame 必须捎带最近 lookahead"


def test_adapter_keeps_carrot():
    a = TimelineAdapter()
    a.feed({"type": "carrot", "xy": [3.2, -0.4]})
    msgs = a.feed({"type": "state", "t": 1.0, "x": 0, "y": 0, "yaw": 0})
    frame = next(m for m in msgs if m["type"] == "frame")
    assert frame["carrot"] == [3.2, -0.4]


def test_adapter_state_overrides_observer():
    """若 live_bridge 自己带了 lookahead，优先用它的（避免观测节点滞后覆盖）"""
    a = TimelineAdapter()
    a.feed({"type": "lookahead", "dist": 9.9})
    msgs = a.feed({"type": "state", "t": 1, "x": 0, "y": 0, "yaw": 0, "lookahead": 1.5})
    frame = next(m for m in msgs if m["type"] == "frame")
    assert frame["lookahead"] == 1.5


def test_adapter_lookahead_null_before_any_observation():
    """还没有观测时必须是 null，不能假装有值"""
    a = TimelineAdapter()
    msgs = a.feed({"type": "state", "t": 1, "x": 0, "y": 0, "yaw": 0})
    frame = next(m for m in msgs if m["type"] == "frame")
    assert frame["lookahead"] is None
    assert frame["carrot"] is None


def test_observer_script_exists():
    p = os.path.join(os.path.dirname(__file__), "scripts", "lookahead_observer.py")
    assert os.path.isfile(p), "lookahead 观测脚本缺失"
    src = open(p, encoding="utf-8").read()
    # 关键约束：不得改上游源码，必须只订阅/发布
    assert "lookahead_point" in src, "必须订阅 planner 已发布的 lookahead_point"
    assert "json.dumps" in src, "必须输出 JSON Lines 供宿主机解析"
    assert "tf2_ros" in src, "需要 TF 把 carrot 转到 map 系"


@pytest.mark.docker
def test_observer_runs_with_master():
    """真实容器：有 master 时观测节点应能启动（无 master 时优雅退出另测）"""
    import subprocess
    src = ("source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
           "source /root/catkin_ws/devel/setup.bash >/dev/null 2>&1; "
           "export ROS_MASTER_URI=http://localhost:11323; "
           "python3 -c 'import rospy, tf2_ros' && echo DEPS_OK")
    p = subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", src],
                       capture_output=True, text=True, timeout=60)
    assert "DEPS_OK" in p.stdout, f"依赖缺失: {p.stderr[-300:]}"
