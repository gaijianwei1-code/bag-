"""诊断：回放期间直接检查 observer 进程与话题是否有数据。"""
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


def diag(tag):
    """检查：①话题是否存在 ②话题是否有发布者 ③能否 echo 到数据"""
    print(f"\n--- 诊断 [{tag}] ---", flush=True)
    cmds = {
        "1.话题列表": "rostopic list 2>/dev/null | grep -i lookahead",
        "2.话题信息": "rostopic info /sweep_base/PursuitLocalPlanner/lookahead_point 2>&1 | head -8",
        "3.echo 5条": "timeout 6 rostopic echo -n 3 /sweep_base/PursuitLocalPlanner/lookahead_point 2>&1 | head -20",
        "4.observer直跑": ("timeout 8 python3 -u /root/catkin_ws/sim/scripts/lookahead_observer.py 2>&1 | head -8"),
    }
    for k, c in cmds.items():
        p = subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + c],
                           capture_output=True, text=True, timeout=40)
        out = (p.stdout or "").strip()
        err = (p.stderr or "").strip()
        print(f"  {k}: rc={p.returncode}")
        if out:
            print("    out:", out[:600].replace("\n", "\n         "))
        if err:
            print("    err:", err[:300].replace("\n", "\n         "))


def probe_loop():
    # 等回放真正起来（planner 启动需要时间）
    for _ in range(30):
        if stop.is_set():
            return
        p = subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc",
                            SRC + "rostopic list 2>/dev/null | grep -c lookahead"],
                           capture_output=True, text=True, timeout=30)
        if p.stdout.strip() and int(p.stdout.strip() or 0) > 0:
            time.sleep(8)      # 让 planner 稳定
            diag("回放运行中")
            return
        time.sleep(2)
    print("超时：未见 lookahead 话题", flush=True)


t = threading.Thread(target=probe_loop, daemon=True)
t.start()
try:
    R.submit(venue, {"branch": "main", "duration_s": 50}, "diag_lookahead",
             on_status=lambda s, p, m: print(f"    {s} {p:.2f} {m}", flush=True))
finally:
    stop.set()
    t.join(timeout=10)
print("\n=== 诊断结束 ===")
