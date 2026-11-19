"""验证假设：调小 next_goal_tolerance 能否让 PursuitLocalPlanner 进入 CONTROLLING。

背景：tolerance=6 m 时，25.7 m 短轨迹上 loopSendGoal 每 0.1s 都发新 goal，
sweep_base 反复取消，computeVelocityCommands 从未执行（lookahead 四话题全空）。
已有先例 w1_tolerance_0.3（6→0.3）结果"96% 停车"，所以 0.3 太小。
本脚本测 1.0 / 2.0 两组，看是否能进控制。
"""
import subprocess
import sys
import threading
import time

SRC = ("source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
       "source /root/catkin_ws/devel/setup.bash >/dev/null 2>&1; "
       "export ROS_MASTER_URI=http://localhost:11323; ")
SCENE = "/root/catkin_ws/sim/scenes/b_line_20261030_edge.yaml"

TOPICS = {
    "lookahead_point": "/sweep_base/PursuitLocalPlanner/lookahead_point",
    "plan": "/sweep_base/PursuitLocalPlanner/received_global_plan",
    "vehicle_cmd": "/vehicle_cmd",
}


def run(cmd, timeout=60):
    return subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                          capture_output=True, text=True, timeout=timeout)


def make_param_set(ps_id, tol):
    """在容器内建一个只改 tolerance 的参数集副本。"""
    base = "/root/catkin_ws/sim/param_sets/baseline_branchB"
    dst = f"/root/catkin_ws/sim/param_sets/{ps_id}"
    run(f"rm -rf {dst} && cp -r {base} {dst}")
    meta = f"""param_set_id: {ps_id}
baseline: baseline_branchB
intent: 验证 tolerance {tol} 能否让 planner 进 CONTROLLING（T3.1 排查）
source_branch: feature/force_pass_navigation
commit: 1769b2b5
snapshot_dir: snapshot
changes:
  - file: schedule_goal.yaml
    key: next_goal_tolerance_normal
    from: 6
    to: {tol}
  - file: schedule_goal.yaml
    key: next_goal_tolerance_increase
    from: 6
    to: {tol}
"""
    run(f"cat > {dst}/param_set.yaml << 'XEOF'\n{meta}XEOF")
    print(f"  [参数集] {ps_id}: tolerance 6 → {tol}", flush=True)


def trial(tol):
    ps_id = f"t31_tol_{str(tol).replace('.', 'p')}"
    make_param_set(ps_id, tol)
    stop = threading.Event()
    out = {}

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
        for name, topic in TOPICS.items():
            r = run(f"timeout 7 rostopic hz {topic} 2>&1 | tail -2", timeout=25)
            out[name] = "average rate" in (r.stdout or "")
        # 车是否真的动了
        r = run("timeout 10 python3 -c \""
                "import rospy;from geometry_msgs.msg import PoseStamped;"
                "p=[];rospy.init_node('m',anonymous=True);"
                "rospy.Subscriber('/odom/current_pose',PoseStamped,lambda m:p.append((m.pose.position.x,m.pose.position.y)));"
                "import time;time.sleep(1);f=p[-1] if p else None;time.sleep(6);"
                "print('MOVE',f,p[-1] if p else None);\"", timeout=30)
        out["move"] = (r.stdout or "").strip()[-120:]

    t = threading.Thread(target=probe, daemon=True)
    t.start()
    cmd = (f"cd /root/catkin_ws && BLINE_WS=/root/catkin_ws "
           f"python3 sim/scripts/b_line_run.py {ps_id} '' 45 {SCENE} "
           f"/root/catkin_ws/sim/param_sets/{ps_id}")
    p = subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                       capture_output=True, text=True, timeout=400)
    stop.set()
    t.join(timeout=50)
    print(f"  [结果 tol={tol}] lookahead={'✓' if out.get('lookahead_point') else '✗'} "
          f"plan={'✓' if out.get('plan') else '✗'} "
          f"cmd={'✓' if out.get('vehicle_cmd') else '✗'}", flush=True)
    print(f"  [位移] {out.get('move', '')}", flush=True)
    return out


for tol in (1.0, 2.0):
    print(f"\n=== 试 tolerance={tol} ===", flush=True)
    r = trial(tol)
    if r.get("lookahead_point"):
        print(f"\n>>> tolerance={tol} 让 planner 进入控制了！", flush=True)
        break
    print(f">>> tolerance={tol} 无效，继续下一组", flush=True)
