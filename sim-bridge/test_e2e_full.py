#!/usr/bin/env python3
"""T4.1 端到端测试：模拟回归面板的完整操作流程。

测完所有步骤都通过才让用户测。
"""
import json
import subprocess
import sys
import time
import urllib.request

BASE = "http://127.0.0.1:9100"
H = {"X-Bridge-Token": "dev-token", "Content-Type": "application/json"}
PASS = 0
FAIL = 0


def step(name, ok, detail=""):
    global PASS, FAIL
    if ok:
        PASS += 1
        print(f"  ✅ {name} {detail}")
    else:
        FAIL += 1
        print(f"  ❌ {name} {detail}")


def http(method, path, body=None):
    req = urllib.request.Request(BASE + path, method=method, headers=H,
                                 data=json.dumps(body).encode() if body else None)
    try:
        with urllib.request.urlopen(req, timeout=30) as r:
            return r.status, json.loads(r.read() or b"{}")
    except urllib.error.HTTPError as e:
        return e.code, json.loads(e.read() or b"{}")


# ---------- 1. bridge 存活 ----------
print("\n[1] Bridge 存活", flush=True)
try:
    code, body = http("GET", "/health")
    step("health", code == 200, f"→ {body.get('status')}")
except Exception as e:
    step("health", False, f"→ {e}")
    print("\n❌ bridge 未运行，请先 bash sim-bridge/start_bridge.sh")
    sys.exit(1)

# ---------- 2. 分支列表 ----------
print("\n[2] 分支列表", flush=True)
code, branches = http("GET", "/branches")
step("branches API", code == 200 and len(branches) > 0,
     f"→ {len(branches)} 个分支")
has_main = any(b.get("branch") == "main" for b in branches)
step("含 main 分支", has_main)

# ---------- 3. 场地列表 ----------
print("\n[3] 场地列表", flush=True)
code, venues = http("GET", "/venues")
step("venues API", code == 200 and len(venues) > 0,
     f"→ {len(venues)} 个场地")

# ---------- 4. 场地 bags ----------
print("\n[4] 场地 bags", flush=True)
bags_ok = False
for v in venues:
    vid = v["venue_id"]
    code, bags = http("GET", f"/venues/{vid}/bags")
    ok = code == 200 and len(bags) > 0
    step(f"bags[{vid}]", ok, f"→ {len(bags)} 个 bag")
    if ok:
        bags_ok = True
        first_bag = bags[0]["bag"]
        first_vid = vid

step("至少 1 个 bag", bags_ok)

# ---------- 5. 提交 run（POST /runs）----------
print("\n[5] 提交 run", flush=True)
code, resp = http("POST", "/runs", {
    "venue_id": first_vid,
    "bag": first_bag,
    "branch": "main",
    "traj_json": {"points": []},
    "duration_s": 25,
})
step("POST /runs", code == 202, f"→ {resp.get('run_id', resp)}")
if code == 202:
    run_id = resp["run_id"]

    # ---------- 6. 状态轮询 ----------
    print("\n[6] 状态轮询", flush=True)
    for i in range(40):
        time.sleep(4)
        code, st = http("GET", f"/runs/{run_id}")
        state = st.get("state")
        step(f"[{i*3}s] state={state}", state in ("replaying", "analyzing", "done", "failed"))
        if state in ("done", "failed"):
            break

    # ---------- 7. timeline 有帧 ----------
    print("\n[7] timeline 帧数", flush=True)
    code, tl = http("GET", f"/runs/{run_id}/timeline")
    nframes = len(tl.get("frames", []))
    step("timeline 帧数 > 0", nframes > 0, f"→ {nframes} 帧")

    # ---------- 8. metrics 有数据 ----------
    print("\n[8] metrics", flush=True)
    code, st = http("GET", f"/runs/{run_id}")
    metrics = st.get("metrics") or {}
    step("metrics 非空", len(metrics) > 0, f"→ travel={metrics.get('travel_m')}")

    # ---------- 9. WS 连接 ----------
    print("\n[9] WS 流", flush=True)
    try:
        import websocket
        ws = websocket.create_connection(
            f"ws://127.0.0.1:9100/ws/runs/{run_id}/stream",
            timeout=30, header=["X-Bridge-Token: dev-token"])
        got = []
        t0 = time.time()
        while time.time() - t0 < 20 and len(got) < 5:
            try:
                m = json.loads(ws.recv())
                got.append(m.get("type"))
            except Exception:
                break
        ws.close()
        step("WS 收到 frame", "frame" in got, f"→ {got}")
    except Exception as e:
        step("WS 连接", False, f"→ {e}")

print(f"\n{'='*40}")
print(f"测试结果：{PASS} 通过 / {FAIL} 失败")
if FAIL == 0:
    print("✅ 全部通过！MapTool 回归面板可以正常使用。")
else:
    print("❌ 有失败项，请检查上方。")
sys.exit(0 if FAIL == 0 else 1)
