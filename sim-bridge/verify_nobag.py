"""单次验证：移除 scene.bag 后，planner 是否产出 lookahead。"""
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
    "plan": "/sweep_base/PursuitLocalPlanner/received_global_plan",
    "vehicle_cmd": "/vehicle_cmd",
}
stop = threading.Event()
out = {}


def run(cmd, timeout=40):
    return subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                          capture_output=True, text=True, timeout=timeout)


def probe():
    for _ in range(40):
        if stop.is_set():
            return
        r = run("rostopic list 2>/dev/null | grep -c PursuitLocalPlanner")
        s = r.stdout.strip()
        if s.isdigit() and int(s) > 0:
            break
        time.sleep(2)
    time.sleep(12)
    if stop.is_set():
        return
    for name, topic in TOPICS.items():
        r = run(f"timeout 8 rostopic hz {topic} 2>&1 | tail -2", timeout=30)
        txt = (r.stdout or "").strip().replace("\n", " ")
        out[name] = txt
        mark = "✓" if "average rate" in txt else "✗"
        print(f"  {mark} {name:22s} {txt[:90]}", flush=True)

    # 若 lookahead 有数据，抓实际值
    if "average rate" in out.get("lookahead_point", ""):
        print("\n  lookahead 实际值:", flush=True)
        r = run("timeout 8 rostopic echo -n 3 /sweep_base/PursuitLocalPlanner/lookahead_point "
                "2>&1 | grep -E 'x:|y:' | head -8", timeout=30)
        print((r.stdout or "").strip()[:400])


t = threading.Thread(target=probe, daemon=True)
t.start()
try:
    R.submit(venue, {"branch": "main", "duration_s": 45}, "verify_nobag",
             on_status=lambda s, p, m: None)
finally:
    stop.set()
    t.join(timeout=60)

print("\n=== 结论 ===")
la_ok = "average rate" in out.get("lookahead_point", "")
print("lookahead_point:", "有数据 → planner 在跑，根因确认（scene.bag 导致绕过）" if la_ok
      else "仍无数据 → 还有别的因素")
