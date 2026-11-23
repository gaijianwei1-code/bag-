"""T2.5 runner v2：用 bag_replay.py 替代 b_line_run.py 做 bag 回归验证。

核心变更（2026-11-19）：
  b_line_run.py  → 虚拟雷达闭环（W16 判满障碍致车不动）
  bag_replay.py  → bag 回放闭环（用户需求的正确工具，车能动，有 F 门指标）

bag_replay.py 接口：
  --mode open|closed  --bag <bag>  --map-dir <dir>
  --baseline-param-set <dir>  --candidate-param-set <dir>
  [--session-id <id>]  [--duration <s>]
"""
import json
import os
import shutil
import subprocess
import tempfile

CONTAINER = os.environ.get("SIM_CONTAINER", "roboneo-sim")
WS_IN_CONTAINER = os.environ.get("SIM_WS_DIR", "/root/catkin_ws")
BASELINE_REPO = os.environ.get(
    "BASELINE_REPO", os.path.expanduser("~/multi-agent/squad-workspace/roboneo_sim")
)


class RunError(Exception):
    def __init__(self, code, detail=""):
        self.code = code
        self.detail = detail
        super().__init__(f"{code}: {detail}")


def _exec(cmd, timeout=120, check=True):
    p = subprocess.run(["docker", "exec", CONTAINER, "bash", "-lc", cmd],
                       capture_output=True, text=True, timeout=timeout)
    if check and p.returncode != 0:
        raise RunError("CONTAINER_CMD_FAILED",
                       f"{cmd[:120]} -> rc={p.returncode}: {p.stderr[-400:]}")
    return p.returncode, p.stdout, p.stderr


# ---------- 1. 初始位姿：从 bag 自动提取 ----------
def extract_init_pose(bag_path):
    rc, out, err = _exec(
        "source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
        f"source {WS_IN_CONTAINER}/devel/setup.bash >/dev/null 2>&1; "
        f"python3 {WS_IN_CONTAINER}/sim/scripts/extract_init_pose.py {bag_path}",
        timeout=180, check=False)
    try:
        data = json.loads(out.strip().splitlines()[-1])
    except (ValueError, IndexError):
        raise RunError("INIT_POSE_FAILED", f"输出无法解析: {out[-200:]} {err[-200:]}")
    if "error" in data:
        raise RunError(data["error"], data.get("detail", ""))
    return data


# ---------- 2. 场地档案 + 请求 → bag_replay 参数 ----------
def build_replay_args(venue, req, run_id):
    """把场地档案与 run 请求转成 bag_replay.py 的命令行参数。"""
    c = venue.get("container") or {}
    bag = c.get("bag")
    map_dir = c.get("map_dir")
    if not bag:
        raise RunError("VENUE_CONTAINER_PATHS_MISSING", "缺 container.bag")

    baseline_ps = venue.get("default_param_set") or "baseline_branchB"
    candidate_ps = req.get("param_set") or baseline_ps
    if req.get("param_overrides"):
        candidate_ps = f".tmp_{run_id}"

    args = [
        "--mode", "closed",
        "--bag", bag,
        "--map-dir", map_dir,
        "--baseline-param-set", f"{WS_IN_CONTAINER}/sim/param_sets/{baseline_ps}",
        "--candidate-param-set", f"{WS_IN_CONTAINER}/sim/param_sets/{candidate_ps}",
        "--session-id", run_id,
        "--duration", str(int(req.get("duration_s") or venue.get("duration_s") or 60)),
    ]
    # bag_replay 在 bag 缺 /odom/imu 时要求 --scene（场景提供 world，fake_chassis 发 odom）
    # 用场地档案的 container.world 生成 scene 或直接传已有 scene 文件
    scene_path = c.get("scene_yaml") or f"{WS_IN_CONTAINER}/sim/scenes/b_line_20261030_edge.yaml"
    args.extend(["--scene", scene_path])
    return args, baseline_ps, candidate_ps


def _prepare_param_set_with_overrides(base_ps, tmp_ps, overrides):
    """在容器内基于 base_ps 拷贝出 tmp_ps，并将 param_overrides 写入 YAML。"""
    import shlex
    base_dir = f"{WS_IN_CONTAINER}/sim/param_sets/{base_ps}"
    tmp_dir = f"{WS_IN_CONTAINER}/sim/param_sets/{tmp_ps}"
    ov_json = json.dumps(overrides, ensure_ascii=False)
    py_script = (
        "import json, os, shutil, yaml; "
        f"src={base_dir!r}; dst={tmp_dir!r}; "
        "shutil.rmtree(dst, ignore_errors=True); "
        "shutil.copytree(src, dst); "
        f"ov=json.loads({ov_json!r}); "
        "yamls=[f for f in os.listdir(dst) if f.endswith('.yaml')]; "
        "\nfor k, v in ov.items():\n"
        "  written=False\n"
        "  for fn in yamls:\n"
        "    p=os.path.join(dst, fn)\n"
        "    with open(p,'r',encoding='utf-8') as f: d=yaml.safe_load(f) or {}\n"
        "    if isinstance(d, dict) and k in d:\n"
        "      d[k]=v\n"
        "      with open(p,'w',encoding='utf-8') as f: yaml.safe_dump(d, f, allow_unicode=True, sort_keys=False)\n"
        "      written=True\n"
        "  if not written:\n"
        "    p=os.path.join(dst, 'schedule_goal.yaml')\n"
        "    d={}\n"
        "    if os.path.isfile(p):\n"
        "      with open(p,'r',encoding='utf-8') as f: d=yaml.safe_load(f) or {}\n"
        "    d[k]=v\n"
        "    with open(p,'w',encoding='utf-8') as f: yaml.safe_dump(d, f, allow_unicode=True, sort_keys=False)\n"
    )
    _exec(f"python3 -c {shlex.quote(py_script)}", timeout=30, check=True)


def _cleanup_param_set(tmp_ps):
    if tmp_ps and tmp_ps.startswith(".tmp_"):
        _exec(f"rm -rf {WS_IN_CONTAINER}/sim/param_sets/{tmp_ps}", check=False)


def cancel_active_run():
    """中止容器内正在运行的回放与直播进程（用于单并发卡死恢复）。"""
    _exec("pkill -f '[b]ag_replay|[b]_line_run|[l]ive_bridge|[l]ookahead_observer' || true",
          timeout=15, check=False)


# ---------- 3. 分支快照：worktree --detach ----------
def make_worktree(branch, commit=None, repo=BASELINE_REPO, base_dir=None):
    if not os.path.isdir(os.path.join(repo, ".git")):
        raise RunError("REPO_NOT_FOUND", repo)
    ref = commit or branch
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
    p = subprocess.run(["git", "-C", repo, "status", "--porcelain"],
                       capture_output=True, text=True)
    return (p.stdout.strip() == ""), p.stdout.strip()


def list_branches(repo=BASELINE_REPO):
    """列出 git 分支（含当前分支标记）。返回 [{branch, commit, is_current}]"""
    p = subprocess.run(
        ["git", "-C", repo, "for-each-ref", "--format=%(refname:short) %(objectname:short) %(HEAD)",
         "refs/heads/"],
        capture_output=True, text=True)
    if p.returncode != 0:
        return []
    out = []
    for line in p.stdout.strip().splitlines():
        parts = line.split()
        if len(parts) >= 3:
            out.append({"branch": parts[0], "commit": parts[1],
                        "is_current": parts[2] == "*"})
        elif len(parts) == 2:
            out.append({"branch": parts[0], "commit": parts[1], "is_current": False})
    return out


# ---------- 4. 提交执行 ----------
def submit(venue, req, run_id, on_status=None):
    """完整执行一个 run（bag_replay 模式）。"""
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
    tmp_ps = None

    try:
        status("queued", 0.25, "构建 bag_replay 参数")
        args, baseline_ps, candidate_ps = build_replay_args(venue, req, run_id)
        overrides = req.get("param_overrides") or {}
        if overrides:
            base_ps = req.get("param_set") or baseline_ps
            tmp_ps = candidate_ps
            _prepare_param_set_with_overrides(base_ps, tmp_ps, overrides)

        status("replaying", 0.3, "启动 bag 回放闭环")
        # 分支快照同步进容器（只拷 src/sim 子树）
        _exec(f"rm -rf {WS_IN_CONTAINER}/variant_tmp && mkdir -p {WS_IN_CONTAINER}/variant_tmp",
              check=False)
        for sub in ("src", "sim"):
            src_dir = os.path.join(wt, sub)
            if os.path.isdir(src_dir):
                subprocess.run(
                    ["docker", "cp", f"{src_dir}", f"{CONTAINER}:{WS_IN_CONTAINER}/variant_tmp/"],
                    check=True, capture_output=True)

        src = ("source /opt/ros/noetic/setup.bash >/dev/null 2>&1; "
               f"source {WS_IN_CONTAINER}/devel/setup.bash >/dev/null 2>&1; ")
        cmd = (src +
               f"cd {WS_IN_CONTAINER} && "
               f"python3 {WS_IN_CONTAINER}/variant_tmp/sim/scripts/bag_replay.py "
               + " ".join(args))
        rc, out, err = _exec(cmd, timeout=max(600, int(req.get("duration_s", 60)) * 8),
                             check=False)
        if rc != 0:
            raise RunError("REPLAY_FAILED", f"rc={rc}: {err[-600:]}")

        status("analyzing", 0.85, "读取指标")
        record = _read_result(run_id)
        status("done", 1.0, "完成")
        return {"run_id": run_id, "init_pose": init_pose,
                "commit": commit, "worktree": wt, "record": record,
                "baseline_param_set": baseline_ps,
                "candidate_param_set": candidate_ps}
    finally:
        if tmp_ps:
            _cleanup_param_set(tmp_ps)
        cleanup_worktree(wt)


def _read_result(run_id):
    """bag_replay 的结果分布多处，合并为一份 summary 返回。

    实际路径（2026-11-19 实测）：
      runs/BLINE/{run_id}_base/summary.json   → baseline 指标
      runs/BLINE/{run_id}_cand/summary.json   → candidate 指标（通常先取这个）
      runs/BAG_REPLAY/{run_id}/compare.json   → F 门对比
      runs/BAG_REPLAY/{run_id}/summary.json   → 可能存在（直接模式）
    """
    result = {}
    for suffix in ("_cand", "_base", ""):
        for base in ("BLINE", "BAG_REPLAY"):
            path = f"{WS_IN_CONTAINER}/runs/{base}/{run_id}{suffix}/summary.json"
            rc, out, _ = _exec(f"cat {path} 2>/dev/null", check=False)
            if rc == 0 and out.strip():
                try:
                    data = json.loads(out)
                    if not result:
                        result = data
                    elif suffix == "_base" and "motion" in data:
                        result["baseline_summary"] = data
                except ValueError:
                    pass
                if result and suffix != "":
                    break
    # F 门对比
    rc, out, _ = _exec(
        f"cat {WS_IN_CONTAINER}/runs/BAG_REPLAY/{run_id}/compare.json 2>/dev/null",
        check=False)
    if rc == 0 and out.strip():
        try:
            result["compare"] = json.loads(out)
        except ValueError:
            pass
    return result or None
