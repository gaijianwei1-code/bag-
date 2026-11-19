"""验证 tolerance=1.0 时 lookahead 实际值，并诊断车为何不动。"""
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
    for _ in range(40):
        if stop.is_set():
            return
        r = run("rostopic list 2>/dev/null | grep -c PursuitLocalPlanner")
        s = r.stdout.strip()
        if s.isdigit() and int(s) > 0:
            break
        time.sleep(2)
    time.sleep(14)
    if stop.is_set():
        return

    print("\n=== A. lookahead_point 实际值 ===", flush=True)
    r = run("timeout 10 rostopic echo -n 5 /sweep_base/PursuitLocalPlanner/lookahead_point "
            "2>&1 | grep -E 'x:|y:' | head -12", timeout=30)
    print((r.stdout or "").strip()[:500])

    print("\n=== B. vehicle_cmd 实际内容（车是否在动）===", flush=True)
    r = run("timeout 8 rostopic echo -n 6 /vehicle_cmd 2>&1 | grep -E 'speed|steering|mode' | head -18", timeout=30)
    print((r.stdout or "").strip()[:600])

    print("\n=== C. sweep_base/status 是否还在取消 ===", flush=True)
    r = run("timeout 8 rostopic echo -n 6 /sweep_base/status 2>&1 | grep -E 'status:|text:' | head -14", timeout=30)
    print((r.stdout or "").strip()[:700])

    print("\n=== D. 位姿变化 ===", flush=True)
    r = run("timeout 12 python3 -c \""
            "import rospy;from geometry_msgs.msg import PoseStamped;"
            "p=[];rospy.init_node('m2',anonymous=True);"
            "rospy.Subscriber('/odom/current_pose',PoseStamped,lambda m:p.append((round(m.pose.position.x,3),round(m.pose.position.y,3))));"
            "import time;time.sleep(2);f=p[-1] if p else None;time.sleep(8);"
            "import math;"
            "print('FIRST',f,'LAST',p[-1] if p else None);"
            "print('DIST', math.hypot(p[-1][0]-f[0],p[-1][1]-f[1]) if f and p else 0)\"", timeout=35)
    print((r.stdout or "").strip()[-300:])


t = threading.Thread(target=probe, daemon=True)
t.start()
cmd = (f"cd /root/catkin_ws && BLINE_WS=/root/catkin_ws "
       f"python3 sim/scripts/b_line_run.py t31_check '' 45 {SCENE} {PS}")
p = subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                   capture_output=True, text=True, timeout=400)
print(f"\n[b_line_run rc={p.returncode}]", flush=True)
stop.set()
t.join(timeout=60)
