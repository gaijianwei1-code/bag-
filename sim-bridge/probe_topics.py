"""探针：在回放运行期间抓取所有含 lookahead/carrot 的话题名，定位命名空间。"""
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

found = []
stop = threading.Event()


def probe():
    """回放期间每 3 秒抓一次话题列表。"""
    src = ("source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
           "source /root/catkin_ws/devel/setup.bash >/dev/null 2>&1; "
           "export ROS_MASTER_URI=http://localhost:11323; rostopic list 2>/dev/null")
    seen = set()
    while not stop.is_set():
        p = subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", src],
                           capture_output=True, text=True, timeout=30)
        for line in p.stdout.splitlines():
            line = line.strip()
            if ("lookahead" in line.lower() or "carrot" in line.lower()
                    or "pursuit" in line.lower()) and line not in seen:
                seen.add(line)
                found.append(line)
                print(f"  [发现] {line}", flush=True)
        time.sleep(3.0)


def on_status(state, progress, msg):
    print(f"    {state} {progress:.2f} {msg}")


t = threading.Thread(target=probe, daemon=True)
t.start()
try:
    R.submit(venue, {"branch": "main", "duration_s": 40}, "probe_topics",
             on_status=on_status)
finally:
    stop.set()
    t.join(timeout=5)

print("\n=== 结果 ===")
if found:
    print("找到话题:")
    for f in found:
        print(" ", f)
else:
    print("未找到任何 lookahead/carrot/pursuit 话题")
    print("说明：planner 未在此场景启动，或话题名不含这些关键词")
