"""直接对比：同一时刻，rostopic echo 能看到数据，但 python 订阅收不到？"""
import subprocess
import threading
import time

SRC = ("source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
       "source /root/catkin_ws/devel/setup.bash >/dev/null 2>&1; "
       "export ROS_MASTER_URI=http://localhost:11323; ")
SCENE = "/root/catkin_ws/sim/scenes/b_line_20261030_edge.yaml"
PS = "/root/catkin_ws/sim/param_sets/t31_tol_1p0"

stop = threading.Event()


def run(cmd, timeout=60):
    return subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                          capture_output=True, text=True, timeout=timeout)


def probe():
    for i in range(45):
        if stop.is_set():
            return
        r = run("rostopic list 2>/dev/null | grep -c PursuitLocalPlanner")
        s = r.stdout.strip()
        if s.isdigit() and int(s) > 0:
            break
        time.sleep(2)
    time.sleep(8)
    if stop.is_set():
        return

    print("\n[1] rostopic echo 能拿到吗", flush=True)
    r = run("timeout 6 rostopic echo -n 1 /sweep_base/PursuitLocalPlanner/lookahead_point 2>&1 | head -20", timeout=25)
    print((r.stdout or "").strip()[:400], flush=True)

    print("\n[2] python 订阅能收到吗", flush=True)
    r = run("timeout 30 python3 /root/catkin_ws/sim/scripts/diag_carrot.py 20", timeout=50)
    print((r.stdout or "").strip()[:600], flush=True)
    if not (r.stdout or "").strip():
        print("  stderr:", (r.stderr or "")[:400], flush=True)


t = threading.Thread(target=probe, daemon=True)
t.start()
cmd = (f"cd /root/catkin_ws && BLINE_WS=/root/catkin_ws "
       f"python3 sim/scripts/b_line_run.py diag_carrot '' 45 {SCENE} {PS}")
subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
               capture_output=True, text=True, timeout=450)
stop.set()
t.join(timeout=80)
