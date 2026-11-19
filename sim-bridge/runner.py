"""T2.2 run 执行器：场地档案 + 分支 → worktree 快照 → scene yaml → b_line_run。

约束（项目总览 §七）：
- roboneo_sim 容器只 docker exec，不 rm/stop/rebuild/restart
- 分支切换走 git worktree --detach，主工作区零影响
- 参数走参数集副本，不改 user_config；地图与初始位姿只来自场地档案
"""
import json
import os
import shutil
import subprocess
import tempfile

CONTAINER = "roboneo-sim"
WS_IN_CONTAINER = "/root/catkin_ws"
BASELINE_REPO = os.path.expanduser("~/multi-agent/squad-workspace/roboneo_sim")


class RunError(Exception):
    def __init__(self, code, detail=""):
        self.code = code
        self.detail = detail
        super().__init__(f"{code}: {detail}")


def _exec(cmd, timeout=120, check=True):
    """在容器内执行命令，返回 (rc, stdout, stderr)。"""
    p = subprocess.run(["docker", "exec", CONTAINER, "bash", "-lc", cmd],
                       capture_output=True, text=True, timeout=timeout)
    if check and p.returncode != 0:
        raise RunError("CONTAINER_CMD_FAILED",
                       f"{cmd[:120]} -> rc={p.returncode}: {p.stderr[-400:]}")
    return p.returncode, p.stdout, p.stderr


# ---------- 1. 初始位姿：从 bag 自动提取（不设人工步骤） ----------
def extract_init_pose(bag_path):
    """bag_path 为容器内路径。返回 {x,y,yaw,samples,...}。"""
    rc, out, err = _exec(
        "source /opt/ros/noetic/setup.bash >/dev/null 2>&1; source /root/catkin_ws/devel/setup.bash >/dev/null 2>&1; "
        f"python3 {WS_IN_CONTAINER}/sim/scripts/extract_init_pose.py {bag_path}",
        timeout=180, check=False)
    try:
        data = json.loads(out.strip().splitlines()[-1])
    except (ValueError, IndexError):
        raise RunError("INIT_POSE_FAILED", f"输出无法解析: {out[-200:]} {err[-200:]}")
    if "error" in data:
        raise RunError(data["error"], data.get("detail", ""))
    return data


# ---------- 2. 场地档案 + 请求 → scene yaml ----------
def build_scene(venue, req, init_pose, run_id):
    """把场地档案与 run 请求合成 b_line_run 需要的 scene。

    venue['container'] 提供容器内路径（map_dir/trajectory/bag/world），
    因为这些文件在容器里，宿主机路径对 roslaunch 无意义。
    """
    c = venue.get("container") or {}
    missing = [k for k in ("map_dir", "trajectory", "bag") if not c.get(k)]
    if missing:
        raise RunError("VENUE_CONTAINER_PATHS_MISSING",
                       f"场地档案缺 container 段字段: {missing}")

    scene = {
        "scene_id": f"{venue['venue_id']}__{run_id}",
        "line": "B",
        "name": f"{venue.get('name', venue['venue_id'])} · bridge run {run_id}",
        "world": c.get("world") or "",
        "map_dir": c["map_dir"],
        "trajectory": c["trajectory"],
        # 关键：不把 bag 传给 launch。
        # bag 的唯一用途是"提取初始位姿"（既定决定 2），不应让仿真切换成回放位姿模式。
        # 2026-11-19 实测：传 bag 时 planner 从不进入 computeVelocityCommands
        # （lookahead_point/theta/received_global_plan 四个话题全空，只有 vehicle_cmd 有数据），
        # 说明控制链被绕过。历史可用的 scene（b_line_20261030_edge.yaml）也没有这个字段。
        "duration_s": float(req.get("duration_s") or venue.get("duration_s") or 120),
        "seed": 0,
        # 初始位姿来自 bag 自动提取，不来自人
        "initial_x": float(init_pose["x"]),
        "initial_y": float(init_pose["y"]),
        "initial_yaw": float(init_pose["yaw"]),
        "metrics": ["path_completion", "terminal_reached", "command_continuity"],
        "data_quality": {"golden": False, "world": "replay_derived",
                         "localization": "chassis_ground_truth",
                         "dynamic_obstacles": False},
        "notes": (f"bridge 提交；初始位姿由 bag 前 {init_pose.get('window_s')}s "
                  f"{init_pose.get('samples')} 条样本中位值提取"),
    }
    # 参数覆盖走独立参数集副本，scene 只带 id 引用
    # 优先用请求指定的；否则用场地档案的 default_param_set（有的场地必需特定参数才能跑）
    ps = req.get("param_set") or venue.get("default_param_set")
    if ps:
        scene["param_set"] = ps
    if req.get("segment"):
        scene["segment"] = req["segment"]
    return scene


def write_scene(scene, out_dir):
    """在容器内写 scene yaml（b_line_run 从容器路径读）。"""
    os.makedirs(out_dir, exist_ok=True)
    local = os.path.join(out_dir, scene["scene_id"] + ".yaml")
    try:
        import yaml
        with open(local, "w", encoding="utf-8") as f:
            yaml.safe_dump(scene, f, allow_unicode=True, sort_keys=False)
    except ImportError:
        with open(local, "w", encoding="utf-8") as f:
            json.dump(scene, f, ensure_ascii=False, indent=2)
    return local


def push_to_container(local_path, container_path):
    subprocess.run(["docker", "cp", local_path, f"{CONTAINER}:{container_path}"],
                   check=True, capture_output=True)


# ---------- 3. 分支快照：worktree --detach（主工作区零影响） ----------
def make_worktree(branch, commit=None, repo=BASELINE_REPO, base_dir=None):
    """建 git worktree。返回 (path, resolved_commit)。用完必须 cleanup_worktree。"""
    if not os.path.isdir(os.path.join(repo, ".git")):
        raise RunError("REPO_NOT_FOUND", repo)
    ref = commit or branch
    # 校验 ref 存在
    p = subprocess.run(["git", "-C", repo, "rev-parse", "--verify", f"{ref}^{{commit}}"],
                       capture_output=True, text=True)
    if p.returncode != 0:
        raise RunError("BRANCH_NOT_FOUND", f"{ref}: {p.stderr.strip()[:200]}")
    resolved = p.stdout.strip()

    base_dir = base_dir or tempfile.mkdtemp(prefix="blz_worktree_")
    path = os.path.join(base_dir, "ws")
    subprocess.run(["git", "-C", repo, "worktree", "add", "--detach", path, resolved],
                   check=True, capture_output=True, text=True)
    return path, resolved


def cleanup_worktree(path, repo=BASELINE_REPO):
    if not path or not os.path.isdir(path):
        return
    subprocess.run(["git", "-C", repo, "worktree", "remove", "--force", path],
                   capture_output=True, text=True)
    parent = os.path.dirname(path)
    if parent.startswith(tempfile.gettempdir()):
        shutil.rmtree(parent, ignore_errors=True)


def main_workspace_clean(repo=BASELINE_REPO):
    """验证不污染主工作区（V3 验收）。返回 (clean, porcelain)。"""
    p = subprocess.run(["git", "-C", repo, "status", "--porcelain"],
                       capture_output=True, text=True)
    return (p.stdout.strip() == ""), p.stdout.strip()


# ---------- 4. 提交执行 ----------
def submit(venue, req, run_id, on_status=None):
    """完整执行一个 run。on_status(state, progress, msg) 用于回调 bridge。

    返回 {'run_id','init_pose','scene','commit','worktree','record'}。
    """
    def status(state, progress=0.0, msg=""):
        if on_status:
            on_status(state, progress, msg)

    status("queued", 0.0, "校验场地与 bag")
    c = venue.get("container") or {}
    bag = c.get("bag")
    if not bag:
        raise RunError("VENUE_CONTAINER_PATHS_MISSING", "缺 container.bag")

    status("queued", 0.05, "从 bag 提取初始位姿")
    init_pose = extract_init_pose(bag)

    status("queued", 0.15, "建分支快照 worktree")
    wt, commit = make_worktree(req.get("branch", "main"), req.get("commit"))

    try:
        status("queued", 0.25, "生成 scene 并注入容器")
        scene = build_scene(venue, req, init_pose, run_id)
        local_scene = write_scene(scene, tempfile.mkdtemp(prefix="blz_scene_"))
        cpath = f"{WS_IN_CONTAINER}/sim/scenes/{scene['scene_id']}.yaml"
        push_to_container(local_scene, cpath)

        status("replaying", 0.3, "启动闭环回放")
        # 分支快照同步进容器：只拷源码子树（仓库 20G，含 run 产物不能整体拷）
        _exec(f"rm -rf {WS_IN_CONTAINER}/variant_tmp && mkdir -p {WS_IN_CONTAINER}/variant_tmp",
              check=False)
        for sub in ("src", "sim"):
            src_dir = os.path.join(wt, sub)
            if not os.path.isdir(src_dir):
                continue
            subprocess.run(
                ["docker", "cp", f"{src_dir}", f"{CONTAINER}:{WS_IN_CONTAINER}/variant_tmp/"],
                check=True, capture_output=True)

        # 参数集：b_line_run 的第 5 个命令行参数（不是从 scene 读）
        ps_id = scene.get("param_set") or ""
        ps_dir = f"{WS_IN_CONTAINER}/sim/param_sets/{ps_id}" if ps_id else ""
        cmd = (f"source /opt/ros/noetic/setup.bash >/dev/null 2>&1; source /root/catkin_ws/devel/setup.bash >/dev/null 2>&1; "
               f"BLINE_WS={WS_IN_CONTAINER}/variant_tmp "
               f"python3 {WS_IN_CONTAINER}/variant_tmp/sim/scripts/b_line_run.py "
               f"{run_id} '' {scene['duration_s']} {cpath} "
               f"{ps_dir}")
        rc, out, err = _exec(cmd, timeout=max(300, int(scene["duration_s"]) * 4),
                             check=False)
        if rc != 0:
            raise RunError("REPLAY_FAILED", f"rc={rc}: {err[-600:]}")

        status("analyzing", 0.85, "读取 run_record")
        record = _read_record(run_id)
        status("done", 1.0, "完成")
        return {"run_id": run_id, "init_pose": init_pose, "scene": scene,
                "commit": commit, "worktree": wt, "record": record}
    finally:
        cleanup_worktree(wt)


def _read_record(run_id):
    rc, out, _ = _exec(f"cat {WS_IN_CONTAINER}/runs/BLINE/{run_id}/run_record.json",
                       check=False)
    if rc != 0:
        return None
    try:
        return json.loads(out)
    except ValueError:
        return None
