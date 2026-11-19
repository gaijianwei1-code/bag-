"""诊断4：确认 sweep_base 状态机状态，判定 computeVelocityCommands 是否在跑。"""
import os
import subprocess
import sys
import threading

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
    import time
    for _ in range(30):
        r = run("rostopic list 2>/dev/null | grep -c sweep_base")
        if r.stdout.strip().isdigit() and int(r.stdout.strip()) > 0:
            break
        time.sleep(2)
    time.sleep(10)

    print("\n=== A. sweep_base/status 内容 ===", flush=True)
    r = run("timeout 8 rostopic echo -n 3 /sweep_base/status 2>&1 | head -24", timeout=30)
    print((r.stdout or "").strip()[:900])

    print("\n=== B. /vehicle_cmd 发布者 ===", flush=True)
    r = run("rostopic info /vehicle_cmd 2>&1 | head -12")
    print((r.stdout or "").strip())

    print("\n=== C. 谁在发 vehicle_cmd（看节点列表）===", flush=True)
    r = run("rosnode list 2>/dev/null | grep -i 'sweep\\|planner\\|fake\\|chassis' | head -10")
    print((r.stdout or "").strip())

    print("\n=== D. 是否有 controller 报错 ===", flush=True)
    r = run("grep -i 'controller\\|local_planner\\|computeVelocity\\|no valid\\|failed' "
            "/root/.ros/log/latest/rosout.log 2>/dev/null | tail -10")
    print((r.stdout or "").strip()[:900] or "(无匹配)")

    print("\n=== E. sweep_base 参数实际值 ===", flush=True)
    r = run("rosparam get /sweep_base/base_local_planner 2>&1; "
            "rosparam get /move_base/base_local_planner 2>&1 | head -2")
    print((r.stdout or "").strip())


t = threading.Thread(target=probe, daemon=True)
t.start()
try:
    R.submit(venue, {"branch": "main", "duration_s": 55}, "diag4",
             on_status=lambda s, p, m: None)
finally:
    t.join(timeout=60)
print("\n=== 结束 ===")
