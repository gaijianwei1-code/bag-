"""诊断3：planner 是否执行 computeVelocityCommands。
检查 lookahead_collision_arc（同函数内发布）与 generated_plan、vehicle_cmd 的活跃度。
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

stop = threading.Event()


def run(cmd, timeout=45):
    return subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                          capture_output=True, text=True, timeout=timeout)


def probe():
    for _ in range(30):
        if stop.is_set():
            return
        r = run("rostopic list 2>/dev/null | grep -c PursuitLocalPlanner")
        if r.stdout.strip().isdigit() and int(r.stdout.strip()) > 0:
            break
        time.sleep(2)
    time.sleep(10)
    if stop.is_set():
        return

    # 同函数内的其他发布物：若 collision_arc 有数据则 computeVelocityCommands 在跑
    checks = [
        ("lookahead_collision_arc", "/sweep_base/PursuitLocalPlanner/lookahead_collision_arc"),
        ("theta", "/sweep_base/PursuitLocalPlanner/theta"),
        ("received_global_plan", "/sweep_base/PursuitLocalPlanner/received_global_plan"),
        ("vehicle_cmd", "/vehicle_cmd"),
        ("sweep_base/status", "/sweep_base/status"),
    ]
    print("\n=== 各话题活跃度（每项测 6s）===", flush=True)
    for name, topic in checks:
        r = run(f"timeout 6 rostopic hz {topic} 2>&1 | tail -3", timeout=30)
        out = (r.stdout or "").strip().replace("\n", " | ")
        print(f"  {name:26s} {out[:150]}", flush=True)

    # planner 节点日志里的关键错误
    print("\n=== sweep_base 日志（尾部）===", flush=True)
    r = run("ls -t /root/.ros/log/latest/*.log 2>/dev/null | head -5")
    print((r.stdout or "").strip()[:300])
    r = run("grep -i 'no plan\\|planner.*fail\\|computeVelocity\\|lookahead' "
            "/root/.ros/log/latest/*.log 2>/dev/null | tail -8")
    print((r.stdout or "").strip()[:800] or "(无匹配)")


t = threading.Thread(target=probe, daemon=True)
t.start()
try:
    R.submit(venue, {"branch": "main", "duration_s": 55}, "diag3",
             on_status=lambda s, p, m: None)
finally:
    stop.set()
    t.join(timeout=40)
print("\n=== 结束 ===")
