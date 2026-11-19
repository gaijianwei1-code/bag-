"""最小验证：确认带 tolerance 时 carrot 到底有没有数据。"""
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
    # 等话题出现
    for i in range(45):
        if stop.is_set():
            return
        r = run("rostopic list 2>/dev/null | grep -c PursuitLocalPlanner")
        s = r.stdout.strip()
        if s.isdigit() and int(s) > 0:
            print(f"  [t={i*2}s] 话题出现", flush=True)
            break
        time.sleep(2)
    # 每 5 秒测一次 carrot 有没有数据，持续 40 秒
    for k in range(8):
        if stop.is_set():
            return
        time.sleep(5)
        r = run("timeout 4 rostopic echo -n 1 /sweep_base/PursuitLocalPlanner/lookahead_point "
                "2>&1 | head -16", timeout=20)
        txt = (r.stdout or "").strip()
        # 判定：出现 seq 且 seq 递增即为有数据（head -6 常只截到 header）
        ok = "seq:" in txt
        print(f"  [+{(k+1)*5}s] lookahead_point: {'✓ 有数据' if ok else '✗'} "
              f"{txt[:80].replace(chr(10), ' ')}", flush=True)
        if ok:
            return


t = threading.Thread(target=probe, daemon=True)
t.start()
cmd = (f"cd /root/catkin_ws && BLINE_WS=/root/catkin_ws "
       f"python3 sim/scripts/b_line_run.py min_verify '' 60 {SCENE} {PS}")
p = subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                   capture_output=True, text=True, timeout=500)
print(f"[b_line_run rc={p.returncode}]", flush=True)
stop.set()
t.join(timeout=60)
