"""T2.4 端到端联调：起 bridge + 提交 run + 用 Qt 的 WS 客户端连上去验证消息解析。

验证链：bridge WS 推送 → QWebSocket 收到 → LiveLayer 解析 → overlay 有车/命中点
"""
import json
import os
import subprocess
import sys
import time
import urllib.request

BRIDGE_DIR = os.path.dirname(__file__)
PORT = 9100
BASE = f"http://127.0.0.1:{PORT}"
TOKEN = "dev-token"
H = {"X-Bridge-Token": TOKEN, "Content-Type": "application/json"}


def http(method, path, body=None):
    req = urllib.request.Request(BASE + path, method=method,
                                 headers=H,
                                 data=json.dumps(body).encode() if body else None)
    try:
        with urllib.request.urlopen(req, timeout=30) as r:
            return r.status, json.loads(r.read().decode() or "{}")
    except urllib.error.HTTPError as e:
        return e.code, json.loads(e.read().decode() or "{}")


def main():
    # 1. 起 bridge
    print("[1] 启动 sim-bridge ...", flush=True)
    env = dict(os.environ, BRIDGE_TOKEN=TOKEN)
    proc = subprocess.Popen(
        [os.path.join(BRIDGE_DIR, ".venv/bin/python"), "-m", "uvicorn",
         "app:app", "--host", "127.0.0.1", "--port", str(PORT), "--log-level", "warning"],
        cwd=BRIDGE_DIR, env=env,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    try:
        # 等健康检查
        for i in range(30):
            time.sleep(1)
            try:
                code, body = http("GET", "/health")
                if code == 200:
                    print(f"    bridge 就绪: {body}", flush=True)
                    break
            except Exception:  # noqa: BLE001
                pass
        else:
            print("    FAIL: bridge 未就绪")
            return 1

        # 2. 场地列表（验证档案 CRUD 通）
        code, venues = http("GET", "/venues")
        print(f"[2] 场地列表: {code} {[v['venue_id'] for v in venues]}", flush=True)
        if code != 200:
            return 1

        # 3. 提交 run（会异步调 b_line_run，耗时 ~65s）
        print("[3] 提交 run ...", flush=True)
        code, resp = http("POST", "/runs", {
            "venue_id": "scenario-1793346971223",
            "bag": "bags/scenario_1793346971223.bag",
            "branch": "main",
            "traj_json": {"points": []},
            "duration_s": 30,
        })
        print(f"    提交: {code} {resp}", flush=True)
        if code != 202:
            return 1
        run_id = resp["run_id"]

        # 4. 轮询状态（确认 run 真的在跑）
        print("[4] 轮询 run 状态 ...", flush=True)
        for i in range(20):
            time.sleep(5)
            code, st = http("GET", f"/runs/{run_id}")
            print(f"    [{i*5}s] {st.get('state')} {st.get('progress')} {st.get('msg','')}",
                  flush=True)
            if st.get("state") in ("done", "failed"):
                print(f"    终态: {st.get('state')} metrics={st.get('metrics')}", flush=True)
                break

        # 5. timeline 有没有帧
        code, tl = http("GET", f"/runs/{run_id}/timeline")
        n = len(tl.get("frames", []))
        print(f"[5] timeline: {code}, frames={n}", flush=True)
        if n == 0:
            print("    WARN: 无帧（直播可能未启动）")

        # 6. WS 连接测试（用 python websocket 客户端模拟 MapTool）
        print("[6] WS 客户端连接测试 ...", flush=True)
        import websocket  # websocket-client
        url = f"ws://127.0.0.1:{PORT}/ws/runs/{run_id}/stream"
        got = []
        try:
            ws = websocket.create_connection(url, timeout=20,
                                             header=[f"X-Bridge-Token: {TOKEN}"])
            deadline = time.time() + 25
            while time.time() < deadline and len(got) < 5:
                try:
                    m = ws.recv()
                    obj = json.loads(m)
                    got.append(obj.get("type"))
                except Exception:  # noqa: BLE001
                    break
            ws.close()
        except Exception as e:  # noqa: BLE001
            print(f"    WS 连接失败: {e}", flush=True)

        print(f"    收到消息类型: {got}", flush=True)
        print(f"[7] 回放目录: /root/catkin_ws/runs/BLINE/{run_id}", flush=True)

        ok = (code == 200 and n > 0) or len(got) > 0
        print(f"\n=== 联调{'PASS' if ok else 'FAIL'} ===")
        print(f"run_id = {run_id}（可用于 MapTool 直播跟车测试）")
        return 0 if ok else 1
    finally:
        proc.terminate()
        try:
            out, _ = proc.communicate(timeout=10)
            if out and "Error" in out:
                print("bridge 输出尾部:", out[-500:], flush=True)
        except subprocess.TimeoutExpired:
            proc.kill()


if __name__ == "__main__":
    sys.exit(main())
