"""T3.2 静态帧对比：L0 前视快算 vs planner 的 lookahead/carrot。

不依赖车移动（车现在因虚拟雷达判满障碍而停），用**同一批位姿**做几何对比：
  planner 侧：lookahead_point（carrot，base_link 系）→ 模长 = lookahead 距离
  L0  侧   ：用同一地图点云 + 位姿，算视锥内首个命中距离

对比口径：
  · planner lookahead 是"路径上往前多远取点"
  · L0 front_dist 是"视锥内多远有障碍"
两者语义不同！正确对比方式是：
  ① planner 的 carrot 点是否落在 L0 的视锥内（判断"车会不会看到 carrot 方向的障碍"）
  ② planner 选的前视距离是否超过 L0 的首个命中距离（若是 → 前视点已被障碍挡住 = 会切弯）
② 就是用户最初问的："前视距离会不会提前识别到点云，导致拐弯拐不进去"

用法：./.venv/bin/python t32_static.py
"""
import json
import math
import os
import subprocess
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(__file__))
from live_adapter import front_hits_from_points  # noqa: E402

SRC = ("source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
       "source /root/catkin_ws/devel/setup.bash >/dev/null 2>&1; "
       "export ROS_MASTER_URI=http://localhost:11323; ")
SCENE = "/root/catkin_ws/sim/scenes/b_line_20261030_edge.yaml"
PS = "/root/catkin_ws/sim/param_sets/t31_tol_1p0"
PCD = "/root/catkin_ws/scenes/scenario_1793346971223/1793346971223_frames_no_ground.pcd"

stop = threading.Event()
samples = []      # [(t, x, y, yaw, carrot_x, carrot_y, lookahead)]


def run(cmd, timeout=60):
    return subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
                          capture_output=True, text=True, timeout=timeout)


def probe():
    """同时采位姿与 carrot（按时间对齐）。"""
    for _ in range(40):
        if stop.is_set():
            return
        r = run("rostopic list 2>/dev/null | grep -c PursuitLocalPlanner")
        s = r.stdout.strip()
        if s.isdigit() and int(s) > 0:
            break
        time.sleep(2)
    time.sleep(5)
    if stop.is_set():
        return
    # 采集窗口要覆盖剩余回放时间（b_line_run 40s，话题约 6s 出现）
    r = run("timeout 60 python3 /root/catkin_ws/sim/scripts/collect_t32.py 40", timeout=95)
    txt = (r.stdout or "")
    for line in txt.splitlines():
        if line.startswith("DIAG "):
            print(f"  诊断: {line[5:]}", flush=True)
        if line.startswith("SAMPLES "):
            try:
                for d in json.loads(line[8:]):
                    samples.append(d)
            except ValueError as e:
                print(f"  解析失败: {e}", flush=True)
    if not txt.strip():
        print(f"  (无输出) stderr: {(r.stderr or '')[:300]}", flush=True)
    print(f"  采集样本: {len(samples)}", flush=True)


t = threading.Thread(target=probe, daemon=True)
t.start()
cmd = (f"cd /root/catkin_ws && BLINE_WS=/root/catkin_ws "
       f"python3 sim/scripts/b_line_run.py t32_static '' 40 {SCENE} {PS}")
subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc", SRC + cmd],
               capture_output=True, text=True, timeout=400)
stop.set()
t.join(timeout=60)

print(f"\n=== 采集到 {len(samples)} 个样本 ===")
if not samples:
    print("无样本，无法对比")
    sys.exit(1)

# 加载地图点云（容器内读，转成宿主机可用的列表）
print("加载地图点云（用于 L0 计算）...", flush=True)
r = run(f"timeout 120 python3 -c \""
        f"import sys; sys.path.insert(0,'/root/catkin_ws/sim/scripts')\n"
        f"import struct\n"
        f"d=open('{PCD}','rb').read()\n"
        f"hdr=d[:2000].decode('ascii','ignore')\n"
        f"i=hdr.find('DATA binary')\n"
        f"import re\n"
        f"n=int(re.search(r'POINTS (\\d+)',hdr).group(1))\n"
        f"off=hdr.find('\\n',i)+1\n"
        f"pts=[]\n"
        f"for k in range(n):\n"
        f"    b=off+k*16\n"
        f"    x,y,z=struct.unpack_from('fff',d,b)\n"
        f"    pts.append([x,y,z])\n"
        f"print('PTS '+__import__('json').dumps(pts[:60000]))\"", timeout=180)
txt = (r.stdout or "")
map_pts = []
for line in txt.splitlines():
    if line.startswith("PTS "):
        map_pts = json.loads(line[4:])
print(f"  点云点数（抽样）: {len(map_pts)}", flush=True)

# 对比分析
print("\n=== L0 vs planner 对比 ===", flush=True)
rows = []
for s in samples:
    x, y, yaw = s["x"], s["y"], s["yaw"]
    # planner carrot 在 base_link 系 → 转世界
    c, sn = math.cos(yaw), math.sin(yaw)
    wx = x + c * s["cx"] - sn * s["cy"]
    wy = y + sn * s["cx"] + c * s["cy"]
    # L0：视锥内首个命中（用与 UI 相同的参数）
    hits = front_hits_from_points(map_pts, x, y, yaw, front_dist=3.0, half_angle=0.5)
    d0 = min((math.hypot(h[0] - x, h[1] - y) for h in hits), default=None)
    # carrot 是否落在 L0 视锥内
    dx, dy = wx - x, wy - y
    dist_carrot = math.hypot(dx, dy)
    in_cone = False
    if dist_carrot > 1e-6:
        cosang = (dx * math.cos(yaw) + dy * math.sin(yaw)) / dist_carrot
        in_cone = cosang >= math.cos(0.5)
    rows.append({
        "t": s["t"], "la": s["la"], "carrot_in_cone": in_cone,
        "l0_first_hit": d0,
        "la_exceeds_hit": (d0 is not None and s["la"] > d0),
    })

la_vals = [r["la"] for r in rows]
print(f"  planner lookahead: min={min(la_vals):.3f} max={max(la_vals):.3f} "
      f"mean={sum(la_vals)/len(la_vals):.3f}")
n_cone = sum(1 for r in rows if r["carrot_in_cone"])
n_hit = sum(1 for r in rows if r["l0_first_hit"] is not None)
n_exceed = sum(1 for r in rows if r["la_exceeds_hit"])
print(f"  carrot 落在 L0 视锥内: {n_cone}/{len(rows)} "
      f"({100.0*n_cone/len(rows):.1f}%)")
print(f"  L0 有前视命中: {n_hit}/{len(rows)} ({100.0*n_hit/len(rows):.1f}%)")
print(f"  lookahead 超过 L0 首个命中（=前视点被障碍挡住）: {n_exceed}/{len(rows)}")

if n_hit:
    hits_d = [r["l0_first_hit"] for r in rows if r["l0_first_hit"] is not None]
    print(f"  L0 首个命中距离: min={min(hits_d):.3f} max={max(hits_d):.3f} "
          f"mean={sum(hits_d)/len(hits_d):.3f}")

# 落盘
out = os.path.join(os.path.dirname(__file__), "..", "docs", "smoke",
                   "20261119_T3.2_static_compare.json")
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, "w", encoding="utf-8") as f:
    json.dump({"samples": len(rows), "rows": rows}, f, ensure_ascii=False, indent=1)
print(f"\n明细已落盘: {out}")
