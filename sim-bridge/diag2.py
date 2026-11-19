"""诊断2：确认 sim time / /clock 状态，以及 lookahead_point 是否真有数据流。"""
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

stop = threading.Event()


def run(cmd, timeout=40):
    return subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                          capture_output=True, text=True, timeout=timeout)


def probe_loop():
    for _ in range(30):
        if stop.is_set():
            return
        r = run("rostopic list 2>/dev/null | grep -c lookahead")
        if r.stdout.strip().isdigit() and int(r.stdout.strip()) > 0:
            break
        time.sleep(2)
    time.sleep(8)
    if stop.is_set():
        return

    print("\n=== A. /clock 是否发布 ===", flush=True)
    r = run("timeout 6 rostopic hz /clock 2>&1 | head -4")
    print(r.stdout.strip() or r.stderr.strip())

    print("\n=== B. use_sim_time ===", flush=True)
    r = run("rosparam get /use_sim_time 2>&1; echo '--- 各节点 ---'; "
            "rosparam list 2>/dev/null | grep use_sim_time | head -5")
    print(r.stdout.strip() or r.stderr.strip())

    print("\n=== C. lookahead_point 频率（不依赖 sim time 的 hz）===", flush=True)
    r = run("timeout 10 rostopic hz /sweep_base/PursuitLocalPlanner/lookahead_point 2>&1 | head -6", timeout=30)
    print(r.stdout.strip() or r.stderr.strip())

    print("\n=== D. echo 带 -w 0（不等时间）===", flush=True)
    r = run("timeout 8 rostopic echo -n 2 --noarr "
            "/sweep_base/PursuitLocalPlanner/lookahead_point 2>&1 | head -14", timeout=30)
    print(r.stdout.strip() or r.stderr.strip())

    print("\n=== E. 用 python 直接订阅（绕过 rostopic 工具）===", flush=True)
    py = '''
import rospy, sys
from geometry_msgs.msg import PointStamped
got = []
rospy.init_node("probe_carrot", anonymous=True)
def cb(m):
    got.append((m.pose.position.x, m.pose.position.y, m.header.stamp.to_sec()))
    if len(got) >= 3:
        print("GOT", got, flush=True)
        sys.exit(0)
rospy.Subscriber("/sweep_base/PursuitLocalPlanner/lookahead_point", PointStamped, cb)
import time
t0 = time.time()
while time.time() - t0 < 12 and not rospy.is_shutdown():
    if len(got) >= 3:
        break
    time.sleep(0.2)
print("TOTAL", len(got), got[:3], "simtime", rospy.Time.now().to_sec(), flush=True)
'''
    r = run(f"timeout 25 python3 -c '{py}'", timeout=45)
    print(r.stdout.strip() or r.stderr.strip()[:800])


t = threading.Thread(target=probe_loop, daemon=True)
t.start()
try:
    R.submit(venue, {"branch": "main", "duration_s": 50}, "diag2",
             on_status=lambda s, p, m: None)
finally:
    stop.set()
    t.join(timeout=30)
print("\n=== 结束 ===")
