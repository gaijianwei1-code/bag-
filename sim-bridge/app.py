"""sim-bridge：MapTool ↔ roboneo_sim 的桥接层。
API 契约见 docs/bridge_api.md。启动：uvicorn app:app --host 0.0.0.0 --port 9100
"""
import json
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
        # runnable：缺 container.* 的场地（如 T2.1 的 CRUD 样例）无法真正跑 run，
        # 必须在列表里标出来，否则用户选了它提交成功、连直播却"没反应"。
        c = v.get("container") or {}
        missing = [k for k in ("bag", "map_dir") if not c.get(k)]
        bags = v.get("bags", [])
        dur = v.get("duration_s") or (bags[0].get("duration_s") if bags else None) or 60
        out.append({"venue_id": v.get("venue_id"), "name": v.get("name"),
                    "bag_count": len(bags), "map_pgm": v.get("map_pgm"),
                    "map_pcd": v.get("map_pcd"),
                    "default_param_set": v.get("default_param_set"),
                    "duration_s": dur,
                    "runnable": not missing,
                    "missing": missing})
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


@app.get("/container/assets")
def list_container_assets(x_bridge_token: Optional[str] = Header(None)):
    """T1.2 场地档案向导辅助：扫描容器内可用的 map_dir / bag / pcd / trajectory 路径。"""
    _auth(x_bridge_token)
    import subprocess
    container = os.environ.get("SIM_CONTAINER", "roboneo-sim")
    ws = os.environ.get("SIM_WS_DIR", "/root/catkin_ws")
    py = (
        "import glob, json, os\n"
        f"ws = {ws!r}\n"
        "scenes = sorted([d for d in glob.glob(ws + '/scenes/*') if os.path.isdir(d)])\n"
        "bags = sorted(set(glob.glob('/root/bags/**/*.bag', recursive=True) + glob.glob(ws + '/runs/*/*.bag')))[:30]\n"
        "trajs = sorted(glob.glob(ws + '/scenes/*/*.json'))[:30]\n"
        "worlds = sorted(glob.glob(ws + '/scenes/*/*.pcd'))[:30]\n"
        "print(json.dumps({'map_dirs': scenes, 'bags': bags, 'trajectories': trajs, 'worlds': worlds}))\n"
    )
    try:
        p = subprocess.run(["docker", "exec", container, "python3", "-c", py],
                           capture_output=True, text=True, timeout=6)
        if p.returncode == 0 and p.stdout.strip():
            return json.loads(p.stdout.strip().splitlines()[-1])
    except Exception:  # noqa: BLE001
        pass
    return {"map_dirs": [], "bags": [], "trajectories": [], "worlds": []}


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
    param_changes: list[dict] = []
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
    forbidden_keys = list(req.param_overrides.keys()) + [
        str(c.get("key") or "") for c in req.param_changes if isinstance(c, dict)
    ]
    if any(k.startswith("map") or k in ("init_pose",) for k in forbidden_keys):
        raise HTTPException(422, "地图与初始位姿由场地档案管理，不可覆盖")
    with _runs_lock:
        active = [r for r in _runs.values() if r["state"] in ("queued", "replaying", "analyzing")]
        if active:
            raise HTTPException(409, "已有 run 在跑（首版单并发）")
        run_id = f"{req.venue_id}_{datetime.now().strftime('%Y%m%d_%H%M%S')}_{uuid.uuid4().hex[:4]}"
        _runs[run_id] = {"state": "queued", "progress": 0.0, "frames": [],
                         "metrics": None, "req": req.model_dump(),
                         "init_pose": None, "record": None, "error": None,
                         "param_set_id": None, "param_changes": [], "param_plan": [],
                         "segment_info": None}

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
    stop_wait = threading.Event()
    try:
        import runner
        from live_stream import LiveStream, wait_for_ros

        # T2.3 顺序：回放启动（on_status 报 replaying）后等待 ROS master 就绪再起直播，
        # 避免固定 sleep 过短导致空转或过长丢帧。
        def _start_live_when_ready():
            nonlocal live
            if live_started.is_set() or stop_wait.is_set():
                return
            live_started.set()
            wait_for_ros(timeout_s=30)
            if stop_wait.is_set():
                return
            live = LiveStream(run_id, duration=float(req.get("duration_s") or 120),
                              line="B", on_message=on_message)
            live.start()

        def _status(state, progress, msg):
            on_status(state, progress, msg)
            if state == "replaying" and not live_started.is_set():
                threading.Thread(target=_start_live_when_ready, daemon=True).start()

        result = runner.submit(venue, req, run_id, on_status=_status)
        with _runs_lock:
            r = _runs.get(run_id)
            if r:
                if r.get("state") == "failed":
                    return
                r["init_pose"] = result.get("init_pose")
                r["record"] = result.get("record")
                r["commit"] = result.get("commit")
                r["baseline_param_set"] = result.get("baseline_param_set")
                r["param_set_id"] = result.get("param_set_id")
                r["param_changes"] = result.get("param_changes") or []
                r["param_plan"] = result.get("param_plan") or []
                r["segment_info"] = result.get("segment_info")
                rec = result.get("record") or {}
                r["metrics"] = _record_to_metrics(rec)
                r["state"] = "done"
                r["progress"] = 1.0
    except Exception as e:  # noqa: BLE001
        code = getattr(e, "code", "REPLAY_FAILED")
        detail = getattr(e, "detail", str(e))
        with _runs_lock:
            r = _runs.get(run_id)
            if r and r.get("state") != "failed":
                r["state"] = "failed"
                r["error"] = {"code": code, "msg": detail}
    finally:
        stop_wait.set()
        if live is not None:
            live.stop(timeout=5)
            with _runs_lock:
                r = _runs.get(run_id)
                if r:
                    r["live_stderr_tail"] = live.stderr_tail()[-5:]
        _persist_run(run_id)


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


def _persist_run(run_id: str) -> None:
    with _runs_lock:
        r = _runs.get(run_id)
        if not r:
            return
        snap = dict(r)
    try:
        d = os.path.join(RUNS_DIR, run_id)
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, "run.json"), "w", encoding="utf-8") as f:
            json.dump(snap, f, ensure_ascii=False)
        changes = snap.get("param_changes") or []
        plan = snap.get("param_plan") or []
        if changes or plan:
            manifest = {
                "run_id": run_id,
                "param_set_id": snap.get("param_set_id"),
                "baseline_param_set": snap.get("baseline_param_set"),
                "changes": changes,
                "param_plan": plan,
            }
            with open(os.path.join(d, "param_manifest.json"), "w", encoding="utf-8") as f:
                json.dump(manifest, f, ensure_ascii=False, indent=2)
    except OSError:
        pass


def _load_run_if_needed(run_id: str) -> Optional[dict]:
    with _runs_lock:
        if run_id in _runs:
            return _runs[run_id]
    p = os.path.join(RUNS_DIR, run_id, "run.json")
    if os.path.isfile(p):
        try:
            with open(p, "r", encoding="utf-8") as f:
                data = json.load(f)
            with _runs_lock:
                _runs.setdefault(run_id, data)
                return _runs[run_id]
        except (OSError, ValueError):
            pass
    return None


@app.post("/runs/cancel_active")
def cancel_active(x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    cancelled = []
    with _runs_lock:
        for rid, r in _runs.items():
            if r.get("state") in ("queued", "replaying", "analyzing"):
                r["state"] = "failed"
                r["error"] = {"code": "CANCELLED", "msg": "用户手动中止"}
                r["msg"] = "用户手动中止"
                cancelled.append(rid)
    import runner
    runner.cancel_active_run()
    for rid in cancelled:
        _persist_run(rid)
    return {"cancelled": cancelled}


@app.post("/runs/{run_id}/cancel")
def cancel_run(run_id: str, x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    with _runs_lock:
        r = _runs.get(run_id)
        if not r:
            raise HTTPException(404, f"run 不存在：{run_id}")
        if r.get("state") in ("queued", "replaying", "analyzing"):
            r["state"] = "failed"
            r["error"] = {"code": "CANCELLED", "msg": "用户手动中止"}
            r["msg"] = "用户手动中止"
    import runner
    runner.cancel_active_run()
    _persist_run(run_id)
    return {"run_id": run_id, "state": "failed"}


@app.get("/runs")
def list_runs(x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    merged: dict[str, dict] = {}
    if os.path.isdir(RUNS_DIR):
        for rid in os.listdir(RUNS_DIR):
            p = os.path.join(RUNS_DIR, rid, "run.json")
            if os.path.isfile(p):
                try:
                    with open(p, "r", encoding="utf-8") as f:
                        merged[rid] = json.load(f)
                except (OSError, ValueError):
                    pass
    with _runs_lock:
        for rid, r in _runs.items():
            merged[rid] = dict(r)

    out = []
    for rid in sorted(merged.keys(), reverse=True)[:30]:
        r = merged[rid]
        req = r.get("req") or {}
        changes = r.get("param_changes") or req.get("param_changes") or []
        cnt = len(changes) if changes else len(req.get("param_overrides") or {})
        metrics = r.get("metrics") or {}
        out.append({
            "run_id": rid,
            "state": r.get("state"),
            "venue_id": req.get("venue_id"),
            "branch": req.get("branch"),
            "param_set_id": r.get("param_set_id"),
            "param_changes_count": cnt,
            "travel_m": metrics.get("travel_m"),
            "segment_info": r.get("segment_info"),
            "msg": r.get("msg", ""),
        })
    return out


@app.get("/runs/{run_id}")
def get_run(run_id: str, x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    run = _load_run_if_needed(run_id)
    if not run:
        raise HTTPException(404, f"run 不存在：{run_id}")
    return {"run_id": run_id, "state": run["state"], "progress": run["progress"],
            "metrics": run["metrics"], "init_pose": run.get("init_pose"),
            "commit": run.get("commit"), "error": run.get("error"),
            "msg": run.get("msg", ""),
            "req": run.get("req"),
            "param_set_id": run.get("param_set_id"),
            "param_changes": run.get("param_changes"),
            "param_plan": run.get("param_plan"),
            "segment_info": run.get("segment_info")}


@app.get("/runs/{run_id}/timeline")
def run_timeline(run_id: str, x_bridge_token: Optional[str] = Header(None)):
    _auth(x_bridge_token)
    run = _load_run_if_needed(run_id)
    if not run:
        raise HTTPException(404, f"run 不存在：{run_id}")
    with _runs_lock:
        frames = list(run.get("frames", []))
        metrics = run.get("metrics")
    return {"schema": "timeline/v1", "run_id": run_id, "frames": frames,
            "metrics": metrics}


@app.websocket("/ws/runs/{run_id}/stream")
async def run_stream(ws: WebSocket, run_id: str):
    """契约 §3：frame/front_hits/status/metrics/error。

    直播中：跟随 _runs[run_id]["frames"] 增量推送。
    已完成的 run：按**原始时间轴回放**（pace 到真实速度），
    这样 MapTool 连一个已跑完的 run 也能看到车动起来，而不是瞬间刷完。
    """
    import asyncio

    await ws.accept()
    run = _load_run_if_needed(run_id)
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
                # 失败时必须把原因发出去：否则客户端只看到「远端关闭连接」，
                # 用户反映「点连接直播没响应」正是这样产生的。
                err = run.get("error") if run else None
                if err:
                    await ws.send_json({"type": "error",
                                        "code": err.get("code", "RUN_FAILED"),
                                        "msg": err.get("msg", "run 失败")})
                await ws.send_json({"type": "status", "state": state, "progress": 1.0,
                                    "msg": (err or {}).get("msg") or "finished"})
                break
            await asyncio.sleep(0.2)
        await ws.close(code=1000)
    except Exception:  # noqa: BLE001  客户端断开等异常静默处理
        pass
