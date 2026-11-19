"""sim-bridge：MapTool ↔ roboneo_sim 的桥接层。
API 契约见 docs/bridge_api.md。启动：uvicorn app:app --host 0.0.0.0 --port 9100
"""
import os
import uuid
import threading
from datetime import datetime

from fastapi import FastAPI, HTTPException, Header, WebSocket
from pydantic import BaseModel
from typing import Optional

import venues as V

DATA_DIR = os.path.join(os.path.dirname(__file__), "data")
VENUES_DIR = os.path.join(DATA_DIR, "venues")
RUNS_DIR = os.path.join(DATA_DIR, "runs")
TOKEN = os.environ.get("BRIDGE_TOKEN", "dev-token")  # 部署时配置；不暴露公网

app = FastAPI(title="sim-bridge", version="0.1.0")
_runs: dict[str, dict] = {}          # run_id -> {state, progress, frames[], metrics, ...}
_runs_lock = threading.Lock()


def _auth(token: Optional[str]) -> None:
    if token != TOKEN:
        raise HTTPException(401, "bad token")


# ---------- health ----------
@app.get("/health")
def health():
    ros_ok = _probe_ros()
    return {"status": "ok", "version": "0.1.0", "ros_ok": ros_ok}


@app.get("/branches")
def list_branches(x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    import runner
    return runner.list_branches()


def _probe_ros() -> bool:
    # T2.2 接容器后实现：docker exec 检查 roscore
    return False


# ---------- venues ----------
@app.get("/venues")
def list_venues(x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    out = []
    for v in V.list_venues(VENUES_DIR):
        out.append({"venue_id": v.get("venue_id"), "name": v.get("name"),
                    "bag_count": len(v.get("bags", [])), "map_pgm": v.get("map_pgm")})
    return out


@app.post("/venues", status_code=201)
def create_venue(venue: dict, x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    vid = venue.get("venue_id", "")
    path = os.path.join(VENUES_DIR, f"{vid}.yaml")
    if os.path.exists(path):
        raise HTTPException(409, f"场地 ID 已存在：{vid}")
    _check(venue)
    V.save_venue(path, venue)
    return {"venue_id": vid}


@app.put("/venues/{vid}")
def update_venue(vid: str, venue: dict, x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    path = os.path.join(VENUES_DIR, f"{vid}.yaml")
    if not os.path.exists(path):
        raise HTTPException(404, f"场地不存在：{vid}")
    venue["venue_id"] = vid  # 路径优先于 body
    _check(venue)
    V.save_venue(path, venue)
    return {"venue_id": vid}


@app.get("/venues/{vid}/bags")
def venue_bags(vid: str, x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    path = os.path.join(VENUES_DIR, f"{vid}.yaml")
    if not os.path.exists(path):
        raise HTTPException(404, f"场地不存在：{vid}")
    return V.load_venue(path).get("bags", [])


def _check(venue: dict) -> None:
    try:
        V.validate_venue(venue, DATA_DIR, check_files=False)  # bridge 侧文件可能不在本机
    except V.VenueError as e:
        raise HTTPException(422, str(e))


# ---------- runs ----------
class RunRequest(BaseModel):
    venue_id: str
    bag: str
    branch: str = "main"
    commit: Optional[str] = None
    traj_json: dict
    param_overrides: dict = {}
    segment: Optional[dict] = None   # {"from_t": float, "to_t": float} = L2 分段
    duration_s: Optional[float] = None   # 缺省用场地档案的 duration_s
    live: bool = True                # T2.3：是否起实时直播流
    front_dist: float = 3.0          # 前视命中筛选距离（与 L0 同参数）
    half_angle_deg: float = 28.6     # 视锥半角（与 L0 默认 0.5 rad ≈ 28.6° 对齐）
    notes: str = ""


@app.post("/runs", status_code=202)
def submit_run(req: RunRequest, x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    vpath = os.path.join(VENUES_DIR, f"{req.venue_id}.yaml")
    if not os.path.exists(vpath):
        raise HTTPException(404, f"场地不存在：{req.venue_id}")
    venue = V.load_venue(vpath)
    if not V.bag_in_venue(venue, req.bag):
        raise HTTPException(422, f"bag 未在场地档案登记：{req.bag}")
    if any(k.startswith("map") or k in ("init_pose",) for k in req.param_overrides):
        raise HTTPException(422, "地图与初始位姿由场地档案管理，不可覆盖")
    with _runs_lock:
        active = [r for r in _runs.values() if r["state"] in ("queued", "replaying", "analyzing")]
        if active:
            raise HTTPException(409, "已有 run 在跑（首版单并发）")
        run_id = f"{req.venue_id}_{datetime.now().strftime('%Y%m%d_%H%M%S')}_{uuid.uuid4().hex[:4]}"
        _runs[run_id] = {"state": "queued", "progress": 0.0, "frames": [],
                         "metrics": None, "req": req.model_dump(),
                         "init_pose": None, "record": None, "error": None}

    # T2.2：异步执行（首版单并发已在上面守卫）
    th = threading.Thread(target=_execute_run, args=(run_id, venue, req.model_dump()),
                          daemon=True)
    th.start()
    return {"run_id": run_id}


def _execute_run(run_id: str, venue: dict, req: dict) -> None:
    """后台执行 run；把 runner 的状态回调写进 _runs（WS 侧读取）。

    T2.3：执行期间同步起 live_bridge 直播流，把 timeline 消息写进 run 的 frames。
    """
    def on_status(state, progress, msg):
        with _runs_lock:
            r = _runs.get(run_id)
            if r:
                r["state"] = state
                r["progress"] = progress
                r["msg"] = msg

    def on_message(msg):
        """直播消息 → 写进 run 缓冲（WS 侧轮询发送，天然支持断线重连补帧）。"""
        with _runs_lock:
            r = _runs.get(run_id)
            if not r:
                return
            t = msg.get("type")
            if t == "frame":
                r["frames"].append(msg)
            elif t == "front_hits":
                # 挂到最近一帧上（timeline schema：front_hits 随帧捎带）
                if r["frames"]:
                    r["frames"][-1]["front_hits"] = msg.get("hits")
                    r["frames"][-1]["first_hit_dist"] = msg.get("first_hit_dist")
            elif t == "path":
                r["path"] = msg.get("path")
            elif t == "obstacles":
                r["obstacles"] = msg.get("obstacles")
            elif t == "status" and "progress" in msg:
                r["progress"] = msg["progress"]
                if msg.get("state"):
                    r["state"] = msg["state"]

    live = None
    live_started = threading.Event()
    try:
        import runner
        from live_stream import LiveStream

        # T2.3 顺序：回放启动（on_status 报 replaying）时才起直播，那时 master 已在。
        # 反过来会让 live_bridge 在无 master 下空转数十秒后放弃（实测 52s）。
        def _start_live():
            nonlocal live
            if live is not None:
                return
            live = LiveStream(run_id, duration=float(req.get("duration_s") or 120),
                              line="B", on_message=on_message)
            live.start()
            live_started.set()

        def _status(state, progress, msg):
            on_status(state, progress, msg)
            if state == "replaying" and not live_started.is_set():
                threading.Timer(3.0, _start_live).start()   # 给 master 3s 起来

        result = runner.submit(venue, req, run_id, on_status=_status)
        with _runs_lock:
            r = _runs.get(run_id)
            if r:
                r["init_pose"] = result.get("init_pose")
                r["record"] = result.get("record")
                r["commit"] = result.get("commit")
                rec = result.get("record") or {}
                r["metrics"] = _record_to_metrics(rec)
                print(f"[DEBUG _execute_run] run={run_id} record_keys={list(rec.keys()) if rec else 'None'} motion={rec.get('motion')} metrics_travel={r['metrics'].get('travel_m')}", flush=True)
                r["state"] = "done"
                r["progress"] = 1.0
    except Exception as e:  # noqa: BLE001
        code = getattr(e, "code", "REPLAY_FAILED")
        detail = getattr(e, "detail", str(e))
        with _runs_lock:
            r = _runs.get(run_id)
            if r:
                r["state"] = "failed"
                r["error"] = {"code": code, "msg": detail}
    finally:
        if live is not None:
            live.stop(timeout=5)
            with _runs_lock:
                r = _runs.get(run_id)
                if r:
                    r["live_stderr_tail"] = live.stderr_tail()[-5:]


def _record_to_metrics(rec: dict) -> dict:
    """run 指标 → timeline metrics。

    bag_replay 模式：指标在 rec 的顶层 motion/commands/edge_lateral 下，
    以及 rec["compare"]（F 门对比）里。"""
    if not rec:
        return {k: None for k in (
            "travel_m", "path_length_m", "end_error_m", "progress_peak",
            "edge_lateral_rms", "cmd_nonzero", "mean_speed",
            "moved", "gates")}

    motion = rec.get("motion") or {}
    lat = rec.get("edge_lateral") or {}
    cmds = rec.get("commands") or {}
    compare = rec.get("compare") or {}

    def r(v, nd=4):
        return None if v is None else round(float(v), nd)

    travel = motion.get("travel_m")
    moved = (travel is not None and float(travel) > 0.1)

    # F 门判定：从 compare.json 的 screening 或 param_diffs 提取
    gates = compare.get("gates") or {}
    gate_str = None
    if gates:
        gate_str = ", ".join(f"{k}={'✓' if v else '✗'}" for k, v in gates.items()
                             if isinstance(v, bool))
    if not gate_str:
        gate_str = ("已移动，F 门指标待评估" if moved
                    else "未达标：车未移动")

    return {
        "travel_m": r(travel),
        "path_length_m": r(motion.get("path_length_m")),
        "end_error_m": r(motion.get("end_error_m")),
        "progress_peak": r(motion.get("progress_peak"), 2),
        "edge_lateral_rms": r(lat.get("rms")),
        "cmd_count": cmds.get("count"),
        "cmd_nonzero": cmds.get("nonzero"),
        "mean_speed": r(cmds.get("mean_speed")),
        "moved": moved,
        "gates": gate_str,
    }


@app.get("/runs/{run_id}")
def get_run(run_id: str, x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    with _runs_lock:
        run = _runs.get(run_id)
        if not run:
            raise HTTPException(404, f"run 不存在：{run_id}")
        return {"run_id": run_id, "state": run["state"], "progress": run["progress"],
                "metrics": run["metrics"], "init_pose": run.get("init_pose"),
                "commit": run.get("commit"), "error": run.get("error"),
                "msg": run.get("msg", "")}


@app.get("/runs/{run_id}/timeline")
def run_timeline(run_id: str, x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    with _runs_lock:
        run = _runs.get(run_id)
        if not run:
            raise HTTPException(404, f"run 不存在：{run_id}")
        frames = list(run["frames"])
    return {"schema": "timeline/v1", "run_id": run_id, "frames": frames,
            "metrics": run["metrics"]}


@app.websocket("/ws/runs/{run_id}/stream")
async def run_stream(ws: WebSocket, run_id: str):
    """契约 §3：frame/front_hits/status/metrics/error。

    直播中：跟随 _runs[run_id]["frames"] 增量推送。
    已完成的 run：按**原始时间轴回放**（pace 到真实速度），
    这样 MapTool 连一个已跑完的 run 也能看到车动起来，而不是瞬间刷完。
    """
    import asyncio

    await ws.accept()
    with _runs_lock:
        run = _runs.get(run_id)
    if not run:
        await ws.send_json({"type": "error", "code": "RUN_NOT_FOUND",
                            "msg": f"run 不存在：{run_id}"})
        await ws.close()
        return

    last_sent = 0
    replay_clock = None       # 已完成 run 的回放时钟（秒）
    try:
        while True:
            with _runs_lock:
                run = _runs.get(run_id, {})
                frames = list(run.get("frames", []))
                state = run.get("state")
                metrics = run.get("metrics")

            # 已完成且有缓冲：按真实时间轴回放（1x 速度）
            if state in ("done", "failed") and frames and last_sent < len(frames):
                if replay_clock is None:
                    replay_clock = 0.0
                batch = []
                while last_sent < len(frames):
                    f = frames[last_sent]
                    t = float(f.get("t") or 0.0)
                    if t <= replay_clock + 0.15:
                        batch.append(f)
                        last_sent += 1
                    else:
                        break
                for f in batch:
                    await ws.send_json({"type": "frame", **f})
                # 每批发一条回放进度
                if batch:
                    pct = min(0.99, last_sent / max(1, len(frames)))
                    await ws.send_json({"type": "status", "state": "replaying",
                                        "progress": round(pct, 3),
                                        "msg": f"回放 {last_sent}/{len(frames)}"})
                if last_sent < len(frames):
                    next_t = float(frames[last_sent].get("t") or 0.0)
                    gap = max(0.05, min(1.0, next_t - replay_clock))
                    await asyncio.sleep(gap)
                    replay_clock = next_t
                    continue
                # 全部推完
                if metrics and state == "done":
                    await ws.send_json({"type": "metrics", **metrics})
                await ws.send_json({"type": "status", "state": state, "progress": 1.0,
                                    "msg": "replay finished"})
                break

            # 直播中：增量推送
            for i in range(last_sent, len(frames)):
                await ws.send_json({"type": "frame", **frames[i]})
            last_sent = len(frames)
            if state in ("done", "failed"):
                if metrics and state == "done":
                    await ws.send_json({"type": "metrics", **metrics})
                await ws.send_json({"type": "status", "state": state, "progress": 1.0,
                                    "msg": "finished"})
                break
            await asyncio.sleep(0.2)
    except Exception:  # noqa: BLE001  客户端断开等异常静默处理
        pass
