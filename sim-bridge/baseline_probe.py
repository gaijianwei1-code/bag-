"""基准对照：直接用历史验证过的 scene（b_line_20261030_edge.yaml）跑，
判断 planner 是否在当前容器环境里能正常产出 lookahead。

若历史 scene 也不行 → 环境问题（不是我的配置）
若历史 scene 正常   → 我的 scene 配置还有差异
"""
import subprocess
import threading
import time

SRC = ("source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
       "source /root/catkin_ws/devel/setup.bash >/dev/null 2>&1; "
       "export ROS_MASTER_URI=http://localhost:11323; ")
SCENE = "/root/catkin_ws/sim/scenes/b_line_20261030_edge.yaml"

TOPICS = {
    "lookahead_point": "/sweep_base/PursuitLocalPlanner/lookahead_point",
    "theta": "/sweep_base/PursuitLocalPlanner/theta",
    "plan": "/sweep_base/PursuitLocalPlanner/received_global_plan",
    "vehicle_cmd": "/vehicle_cmd",
}
stop = threading.Event()
out = {}


def run(cmd, timeout=60):
    return subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                          capture_output=True, text=True, timeout=timeout)


def probe():
    for _ in range(45):
        if stop.is_set():
            return
        r = run("rostopic list 2>/dev/null | grep -c PursuitLocalPlanner")
        s = r.stdout.strip()
        if s.isdigit() and int(s) > 0:
            break
        time.sleep(2)
    time.sleep(15)
    if stop.is_set():
        return
    for name, topic in TOPICS.items():
        r = run(f"timeout 8 rostopic hz {topic} 2>&1 | tail -2", timeout=30)
        txt = (r.stdout or "").strip().replace("\n", " ")
        out[name] = txt
        print(f"  {'✓' if 'average rate' in txt else '✗'} {name:22s} {txt[:90]}", flush=True)

    if "average rate" in out.get("lookahead_point", ""):
        print("\n  lookahead 实际值:", flush=True)
        r = run("timeout 8 rostopic echo -n 3 /sweep_base/PursuitLocalPlanner/lookahead_point "
                "2>&1 | grep -E 'x:|y:' | head -8", timeout=30)
        print((r.stdout or "").strip()[:400])


t = threading.Thread(target=probe, daemon=True)
t.start()
# 直接用 b_line_run 跑历史 scene（不走我的 runner）
# 参数顺序：run_id world duration scene param_set adapter
# 空 adapter 会 FileNotFoundError，用默认 adapter（省略该参数）
cmd = (f"cd /root/catkin_ws && BLINE_WS=/root/catkin_ws "
       f"python3 sim/scripts/b_line_run.py baseline_probe '' 45 {SCENE} ''")
print(f"[启动] b_line_run 历史 scene, 45s", flush=True)
p = subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                   capture_output=True, text=True, timeout=400)
print(f"[b_line_run 退出] rc={p.returncode}", flush=True)
if p.stderr:
    print("stderr 尾部:", p.stderr.strip()[-400:], flush=True)
stop.set()
t.join(timeout=60)

print("\n=== 基准结论 ===")
la = "average rate" in out.get("lookahead_point", "")
print("历史 scene 的 lookahead_point:", "有数据 → 环境正常，是我的配置问题" if la
      else "仍无数据 → 环境/上游问题，与我的配置无关")
