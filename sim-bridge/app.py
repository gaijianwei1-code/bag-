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
    """后台执行 run；把 runner 的状态回调写进 _runs（WS 侧读取）。"""
    def on_status(state, progress, msg):
        with _runs_lock:
            r = _runs.get(run_id)
            if r:
                r["state"] = state
                r["progress"] = progress
                r["msg"] = msg

    try:
        import runner
        result = runner.submit(venue, req, run_id, on_status=on_status)
        with _runs_lock:
            r = _runs.get(run_id)
            if r:
                r["init_pose"] = result.get("init_pose")
                r["record"] = result.get("record")
                r["commit"] = result.get("commit")
                rec = result.get("record") or {}
                r["metrics"] = _record_to_metrics(rec)
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


def _record_to_metrics(rec: dict) -> dict:
    """run_record → timeline metrics（T0.1 §3 的 8 项口径，缺的置 null）。"""
    keys = {
        "tracking_rms": ("tracking_rms",),
        "tracking_signed_mean": ("tracking_signed_mean",),
        "planner_avg_dev": ("planner_avg_dev", "planner_mean_dev"),
        "theta_rms": ("theta_rms",),
        "speed_diff_pct": ("speed_diff_pct",),
        "min_clearance_diff": ("min_clearance_diff",),
        "travel_diff_pct": ("travel_diff_pct",),
        "collision_count": ("collision_count", "collisions"),
    }
    flat = {}
    for section in ("metrics", "analysis", "gates", "result"):
        v = rec.get(section)
        if isinstance(v, dict):
            flat.update(v)
    flat.update({k: v for k, v in rec.items() if not isinstance(v, (dict, list))})

    out = {}
    for name, aliases in keys.items():
        out[name] = next((flat[a] for a in aliases if a in flat), None)
    out["gates"] = flat.get("gates")
    return out


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
    """契约 §3：frame(5-10Hz)/front_hits(1-2Hz)/status/metrics/error。
    T2.3 接真实回放流；当前回放内存中的 frames 用于联调。"""
    await ws.accept()
    with _runs_lock:
        run = _runs.get(run_id)
    if not run:
        await ws.send_json({"type": "error", "code": "RUN_NOT_FOUND",
                            "msg": f"run 不存在：{run_id}"})
        await ws.close()
        return
    import asyncio
    last_sent = 0
    while True:
        with _runs_lock:
            run = _runs.get(run_id, {})
            frames = run.get("frames", [])
            state = run.get("state")
            metrics = run.get("metrics")
        for i in range(last_sent, len(frames)):
            await ws.send_json({"type": "frame", **frames[i]})
        last_sent = len(frames)
        if state in ("done", "failed"):
            if metrics and state == "done":
                await ws.send_json({"type": "metrics", **metrics})
            await ws.send_json({"type": "status", "state": state, "progress": 1.0,
                                "msg": "finished"})
            break
        await asyncio.sleep(0.2)  # 5Hz 轮询内存（T2.3 改为 ROS 订阅驱动）
