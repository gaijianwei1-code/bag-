"""诊断5：确认 sweep_base 是否真的从未进入 CONTROLLING（这决定整个 B 线回放是否有效）。"""
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


def run(cmd, timeout=40):
    return subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                          capture_output=True, text=True, timeout=timeout)


def probe():
    for _ in range(30):
        r = run("rostopic list 2>/dev/null | grep -c sweep_base")
        if r.stdout.strip().isdigit() and int(r.stdout.strip()) > 0:
            break
        time.sleep(2)
    time.sleep(12)

    print("\n=== A. status 是否为同一个 goal 反复取消（还是不同 goal）===", flush=True)
    r = run("timeout 8 rostopic echo -n 12 /sweep_base/status 2>&1 | grep -E 'id:|status:|text:' | head -30", timeout=30)
    print((r.stdout or "").strip()[:1200])

    print("\n=== B. /sweep_goal 频率（schedule_goal 下发）===", flush=True)
    r = run("timeout 6 rostopic hz /sweep_goal 2>&1 | tail -3", timeout=25)
    print((r.stdout or "").strip())

    print("\n=== C. /vehicle_cmd 的实际内容（速度是否非零）===", flush=True)
    r = run("timeout 6 rostopic echo -n 4 /vehicle_cmd 2>&1 | grep -E 'speed|steering|mode' | head -12", timeout=25)
    print((r.stdout or "").strip())

    print("\n=== D. sweep_base 日志里的状态转移 ===", flush=True)
    r = run("grep -iE 'PLANNING|CONTROLLING|CLEARING|state' /root/.ros/log/latest/sweep_base*.log 2>/dev/null | tail -12")
    print((r.stdout or "").strip()[:900] or "(无日志文件匹配，尝试 rosout)")
    r = run("grep -i 'sweep_base' /root/.ros/log/latest/rosout.log 2>/dev/null | tail -8")
    print((r.stdout or "").strip()[:600] or "(rosout 无匹配)")

    print("\n=== E. 车是否真的位移了（关键：位姿变化）===", flush=True)
    r = run("timeout 10 python3 -c \""
            "import rospy; from geometry_msgs.msg import PoseStamped;"
            "pts=[];rospy.init_node('p',anonymous=True);"
            "rospy.Subscriber('/odom/current_pose',PoseStamped,lambda m: pts.append((round(m.pose.position.x,3),round(m.pose.position.y,3))));"
            "import time;t0=time.time();"
            "\\nwhile time.time()-t0<8 and len(pts)<1: time.sleep(0.1);"
            "\\nfirst=pts[-1] if pts else None;"
            "\\nwhile time.time()-t0<8: time.sleep(0.1);"
            "\\nprint('FIRST',first,'LAST',pts[-1] if pts else None,'N',len(pts));\"", timeout=40)
    print((r.stdout or "").strip()[-400:])


t = threading.Thread(target=probe, daemon=True)
t.start()
try:
    R.submit(venue, {"branch": "main", "duration_s": 55}, "diag5",
             on_status=lambda s, p, m: None)
finally:
    t.join(timeout=60)
print("\n=== 结束 ===")
