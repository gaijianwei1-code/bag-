"""对照实验：同一轨迹，传 bag vs 不传 bag，看 planner 是否产出 lookahead。

假设：scene 里带 bag 字段会让仿真走"回放位姿"路径，planner 不参与控制。
验证：不传 bag 时 lookahead_point 是否出现数据。
"""
import os
import subprocess
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(__file__))
import runner as R
import venues as V

DATA = os.path.join(os.path.dirname(__file__), "data")
venue = V.load_venue(os.path.join(DATA, "venues", "scenario-1793346971223.yaml"))
SRC = ("source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
       "source /root/catkin_ws/devel/setup.bash >/dev/null 2>&1; "
       "export ROS_MASTER_URI=http://localhost:11323; ")

TOPICS = {
    "lookahead_point": "/sweep_base/PursuitLocalPlanner/lookahead_point",
    "theta": "/sweep_base/PursuitLocalPlanner/theta",
    "received_global_plan": "/sweep_base/PursuitLocalPlanner/received_global_plan",
    "vehicle_cmd": "/vehicle_cmd",
}
result = {}


def run(cmd, timeout=40):
    return subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                          capture_output=True, text=True, timeout=timeout)


def probe(stop, tag):
    for _ in range(30):
        if stop.is_set():
            return
        r = run("rostopic list 2>/dev/null | grep -c PursuitLocalPlanner")
        if r.stdout.strip().isdigit() and int(r.stdout.strip()) > 0:
            break
        time.sleep(2)
    time.sleep(12)
    got = {}
    for name, topic in TOPICS.items():
        r = run(f"timeout 8 rostopic hz {topic} 2>&1 | grep -c 'average rate'", timeout=30)
        got[name] = "有数据" if r.stdout.strip() not in ("", "0") else "无数据"
    result[tag] = got
    print(f"  [{tag}] {got}", flush=True)


def trial(tag, pass_bag):
    print(f"\n=== {tag}（bag 字段 {'传' if pass_bag else '不传'}）===", flush=True)
    v = dict(venue)
    if not pass_bag:
        v = {k: val for k, val in venue.items() if k != "container"}
        v["container"] = {k: val for k, val in venue["container"].items() if k != "bag"}
    stop = threading.Event()
    t = threading.Thread(target=probe, args=(stop, tag), daemon=True)
    t.start()
    try:
        R.submit(v, {"branch": "main", "duration_s": 45}, f"exp_{tag}",
                 on_status=lambda s, p, m: None)
    except Exception as e:  # noqa: BLE001
        print(f"  [{tag}] 提交失败: {e}", flush=True)
    finally:
        stop.set()
        t.join(timeout=45)


trial("with_bag", True)
trial("no_bag", False)

print("\n=== 结论 ===")
for tag, got in result.items():
    ok = all(v == "有数据" for v in got.values())
    print(f"{tag}: {'planner 在跑' if got.get('lookahead_point') == '有数据' else 'planner 未参与'} -> {got}")
