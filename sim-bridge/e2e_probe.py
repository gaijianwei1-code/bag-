"""端到端验证：真实跑一次 b_line_run，检查执行链贯通 + 直播收帧 + 零污染。

用法：./.venv/bin/python e2e_probe.py [duration_s]
"""
import os
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(__file__))
import runner as R  # noqa: E402
import venues as V  # noqa: E402
from live_stream import LiveStream  # noqa: E402

DURATION = float(sys.argv[1]) if len(sys.argv) > 1 else 40.0
DATA = os.path.join(os.path.dirname(__file__), "data")

venue = V.load_venue(os.path.join(DATA, "venues", "scenario-1793346971223.yaml"))
print(f"[1] 场地档案: {venue['venue_id']}, container={list(venue.get('container', {}))}")

clean0, porcelain0 = R.main_workspace_clean()
print(f"[2] 主工作区初始 clean={clean0}" + ("" if clean0 else f" 脏:{porcelain0[:120]}"))

frames = []
live = None
live_started = threading.Event()


def on_msg(m):
    frames.append(m)


def start_live():
    """在回放启动后（master 已在 11323）再起直播。"""
    global live
    if live is not None:
        return
    live = LiveStream("e2e_probe", duration=DURATION, on_message=on_msg)
    live.start()
    live_started.set()


def on_status(state, progress, msg):
    print(f"    status: {state} {progress:.2f} {msg}")
    if state == "replaying" and not live_started.is_set():
        # 给 roscore 几秒起来
        threading.Timer(4.0, start_live).start()


t0 = time.time()
result = None
try:
    result = R.submit(venue, {"branch": "main", "duration_s": DURATION},
                      "e2e_probe", on_status=on_status)
finally:
    if live is not None:
        live.stop(timeout=5)
elapsed = time.time() - t0

print(f"[4] submit 返回，耗时 {elapsed:.1f}s")
if result:
    print(f"    init_pose: {result['init_pose']}")
    print(f"    commit: {result['commit'][:8]}")
    rec = result.get("record") or {}
    print(f"    run_record keys: {list(rec)[:8]}")
else:
    print("    result 为 None")

nframe = sum(1 for m in frames if m["type"] == "frame")
nhit = sum(1 for m in frames if m["type"] == "front_hits")
print(f"[5] 直播消息: {len(frames)} 条（frame={nframe}, front_hits={nhit}）")
if live is not None:
    print(f"    live 诊断尾巴: {live.stderr_tail()[-3:]}")

clean1, porcelain1 = R.main_workspace_clean()
print(f"[6] 主工作区结束 clean={clean1}（基线 {clean0}）")
if not clean1 and porcelain1:
    print(f"    污染内容: {porcelain1[:300]}")

print("\n=== 判定 ===")
ok = True
if not result:
    print("FAIL: 未返回 run 结果")
    ok = False
if nframe == 0:
    print(f"FAIL: 直播 0 帧（诊断: {live.stderr_tail()[-2:] if live else 'no live'}）")
    ok = False
else:
    print(f"PASS: 直播收到 {nframe} 帧 / {nhit} 次前视命中")
if clean0 != clean1:
    print("FAIL: 主工作区被污染")
    ok = False
else:
    print("PASS: 主工作区零污染")
print("\nE2E PASS" if ok else "\nE2E FAIL")
sys.exit(0 if ok else 1)
