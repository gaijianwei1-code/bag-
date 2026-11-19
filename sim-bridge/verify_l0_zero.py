"""核实 L0 命中为 0 的原因：坐标系不匹配 or 高度过滤问题。

思路：用 T3.2 采到的位姿 + carrot，检查点云里位姿附近有没有点。
若位姿 (0.556, 0.026) 附近点云为空 → 坐标系不匹配（点云在世界系，位姿在另一系）
若有点但 L0 判无命中 → 高度过滤或视锥参数问题
"""
import json
import math
import os
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))

PCD = "/root/catkin_ws/scenes/scenario_1793346971223/1793346971223_frames_no_ground.pcd"
SRC = "source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "

SCRIPT = '''
import struct, re, json, math
d = open("%s", "rb").read()
hdr = d[:3000].decode("ascii", "ignore")
n = int(re.search(r"POINTS (\\d+)", hdr).group(1))
i = hdr.find("DATA binary")
off = hdr.find("\\n", i) + 1

xs, ys, zs = [], [], []
step = max(1, n // 80000)
for k in range(0, n, step):
    b = off + k * 16
    x, y, z = struct.unpack_from("fff", d, b)
    xs.append(x); ys.append(y); zs.append(z)

print("RANGE " + json.dumps({
    "n": n, "sampled": len(xs),
    "x": [round(min(xs), 2), round(max(xs), 2)],
    "y": [round(min(ys), 2), round(max(ys), 2)],
    "z": [round(min(zs), 2), round(max(zs), 2)],
}))

# 检查位姿附近（车在 0.556, 0.026）
for label, (px, py) in [("car_at_start", (0.556, 0.026)), ("origin", (0.0, 0.0))]:
    near = [(x, y, z) for x, y, z in zip(xs, ys, zs)
            if math.hypot(x - px, y - py) < 2.0]
    print("NEAR_%%s %%s" %% (label, json.dumps({
        "count": len(near),
        "sample": [[round(a, 2) for a in p] for p in near[:5]],
    })))
''' % PCD

open("/tmp/probe_pts.py", "w").write(SCRIPT)
subprocess.run(["docker", "cp", "/tmp/probe_pts.py",
                "roboneo-sim:/root/catkin_ws/sim/scripts/probe_pts.py"],
               check=True, capture_output=True)
r = subprocess.run(["docker", "exec", "roboneo-sim", "bash", "-lc",
                    SRC + "python3 /root/catkin_ws/sim/scripts/probe_pts.py"],
                   capture_output=True, text=True, timeout=180)
print((r.stdout or "").strip()[:1500])
if r.returncode != 0:
    print("stderr:", (r.stderr or "")[:400])
