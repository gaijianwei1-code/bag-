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
import math
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
    if req.get("param_changes") or req.get("param_overrides"):
        candidate_ps = f"ps_{run_id}"

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


# ---------- 2.5 L2 分段重跑（T4.2：轨迹裁剪 + 最近安全点起步） ----------
def normalize_segment(segment):
    """规范化 segment 参数。支持按轨迹索引 (from_idx/to_idx) 或时间区间 (from_t/to_t)。"""
    if not segment or not isinstance(segment, dict):
        return None
    if "from_idx" in segment or "to_idx" in segment:
        a = int(segment.get("from_idx", 0))
        b = int(segment.get("to_idx", a))
        if a > b:
            a, b = b, a
        a = max(0, a)
        b = max(a, b)
        pad = max(0, int(segment.get("pad_points", 4)))
        return {"mode": "index", "from_idx": a, "to_idx": b, "pad_points": pad}
    if "from_t" in segment or "to_t" in segment:
        t0 = max(0.0, float(segment.get("from_t", 0.0)))
        t1 = max(t0, float(segment.get("to_t", t0)))
        pad_s = max(0.0, float(segment.get("pad_s", 3.0)))
        return {"mode": "time", "from_t": t0, "to_t": t1, "pad_s": pad_s}
    return None


def compute_segment_slice(points, segment, total_duration_s=76.0):
    """根据 segment 与完整轨迹点列表，计算安全起步点、裁剪子轨迹与起步位姿。"""
    n = len(points or [])
    if n < 2:
        raise RunError("SEGMENT_INVALID", f"轨迹点数不足（{n} < 2），无法分段")
    seg = normalize_segment(segment)
    if not seg:
        return None

    if seg["mode"] == "index":
        from_idx = min(n - 1, seg["from_idx"])
        to_idx = min(n - 1, seg["to_idx"])
        pad_pts = seg["pad_points"]
    else:
        dur = max(1.0, float(total_duration_s or 76.0))
        from_idx = min(n - 1, max(0, int(round((seg["from_t"] / dur) * (n - 1)))))
        to_idx = min(n - 1, max(from_idx, int(round((seg["to_t"] / dur) * (n - 1)))))
        pad_pts = max(1, int(round((seg["pad_s"] / dur) * (n - 1))))

    safe_start = max(0, from_idx - pad_pts)
    safe_end = min(n - 1, to_idx)
    if safe_end <= safe_start:
        safe_end = min(n - 1, safe_start + 1)
    if safe_end <= safe_start and safe_start > 0:
        safe_start = safe_end - 1

    sub = points[safe_start:safe_end + 1]
    p0, p1 = sub[0], sub[1]
    init_x = float(p0.get("x", 0.0))
    init_y = float(p0.get("y", 0.0))
    if "yaw" in p0 and p0["yaw"] is not None:
        init_yaw = float(p0["yaw"])
    else:
        init_yaw = math.atan2(float(p1.get("y", 0.0)) - init_y,
                              float(p1.get("x", 0.0)) - init_x)

    arc_len = 0.0
    for i in range(len(sub) - 1):
        arc_len += math.hypot(
            float(sub[i + 1].get("x", 0.0)) - float(sub[i].get("x", 0.0)),
            float(sub[i + 1].get("y", 0.0)) - float(sub[i].get("y", 0.0)),
        )
    auto_dur = max(12, min(120, int(math.ceil(arc_len / 0.30 + 10.0))))
    return {
        "sub_points": sub,
        "from_idx": from_idx,
        "to_idx": to_idx,
        "safe_start_idx": safe_start,
        "safe_end_idx": safe_end,
        "total_points": n,
        "point_count": len(sub),
        "arc_len_m": round(arc_len, 2),
        "init_pose": {"x": round(init_x, 4), "y": round(init_y, 4), "yaw": round(init_yaw, 5)},
        "auto_duration_s": auto_dur,
    }


def _prepare_segment_scene(base_scene_path, segment, run_id,
                           total_duration_s=76.0, req_duration_s=None,
                           custom_traj_json=None):
    """在容器内支持自定义画布轨迹与/或 L2 分段裁剪，生成临时 scene YAML 并返回元信息。"""
    import shlex
    seg = normalize_segment(segment)
    has_custom = (isinstance(custom_traj_json, dict)
                  and isinstance(custom_traj_json.get("points"), list)
                  and len(custom_traj_json["points"]) >= 2)
    if not seg and not has_custom:
        return None
    custom_path = None
    if has_custom:
        # 先将画布修改后的轨迹通过 docker cp 写入容器临时路径（避免命令行超长）
        tmp_local = tempfile.NamedTemporaryFile("w", suffix=".json", delete=False, encoding="utf-8")
        try:
            json.dump(custom_traj_json, tmp_local, ensure_ascii=False)
            tmp_local.close()
            custom_path = f"/tmp/blz_custom_traj_{run_id}.json"
            subprocess.run(["docker", "cp", tmp_local.name, f"{CONTAINER}:{custom_path}"],
                           check=True, capture_output=True)
        finally:
            try:
                os.unlink(tmp_local.name)
            except OSError:
                pass

    seg_json = json.dumps(seg, ensure_ascii=False)
    seg_traj_path = f"/tmp/blz_seg_traj_{run_id}.json"
    seg_scene_path = f"/tmp/blz_seg_scene_{run_id}.yaml"
    py_script = (
        "import json, math, os, sys, yaml\n"
        f"base_scene = {base_scene_path!r}\n"
        f"seg = json.loads({seg_json!r})\n"
        f"custom_path = {custom_path!r}\n"
        f"out_traj = {seg_traj_path!r}\n"
        f"out_scene = {seg_scene_path!r}\n"
        f"total_dur = float({float(total_duration_s or 76.0)!r})\n"
        f"req_dur = {int(req_duration_s) if req_duration_s else None!r}\n"
        f"run_id = {run_id!r}\n"
        "with open(base_scene, 'r', encoding='utf-8') as f:\n"
        "    scene = yaml.safe_load(f) or {}\n"
        "traj_path = custom_path or scene.get('trajectory') or '/root/catkin_ws/scenes/scenario_1793346971223/1793347176294.json'\n"
        "with open(traj_path, 'r', encoding='utf-8') as f:\n"
        "    traj = json.load(f)\n"
        "pts = traj.get('points') if isinstance(traj, dict) else traj\n"
        "n = len(pts or [])\n"
        "if n < 2:\n"
        "    print(json.dumps({'error': 'SEGMENT_INVALID', 'detail': f'轨迹点数不足: {n}'})); sys.exit(2)\n"
        "if seg:\n"
        "    if seg['mode'] == 'index':\n"
        "        fi = min(n - 1, max(0, int(seg['from_idx'])))\n"
        "        ti = min(n - 1, max(fi, int(seg['to_idx'])))\n"
        "        pad = max(0, int(seg.get('pad_points', 4)))\n"
        "    else:\n"
        "        dur = max(1.0, total_dur)\n"
        "        fi = min(n - 1, max(0, int(round((seg['from_t'] / dur) * (n - 1)))))\n"
        "        ti = min(n - 1, max(fi, int(round((seg['to_t'] / dur) * (n - 1)))))\n"
        "        pad = max(1, int(round((seg.get('pad_s', 3.0) / dur) * (n - 1))))\n"
        "    ss = max(0, fi - pad)\n"
        "    se = min(n - 1, ti)\n"
        "    if se <= ss: se = min(n - 1, ss + 1)\n"
        "    if se <= ss and ss > 0: ss = se - 1\n"
        "else:\n"
        "    fi, ti, ss, se = 0, n - 1, 0, n - 1\n"
        "sub = pts[ss:se + 1]\n"
        "p0, p1 = sub[0], sub[1]\n"
        "ix = float(p0.get('x', 0.0)); iy = float(p0.get('y', 0.0))\n"
        "iyaw = float(p0['yaw']) if p0.get('yaw') is not None else math.atan2(float(p1.get('y', 0.0)) - iy, float(p1.get('x', 0.0)) - ix)\n"
        "arc = sum(math.hypot(float(sub[i+1].get('x',0))-float(sub[i].get('x',0)), float(sub[i+1].get('y',0))-float(sub[i].get('y',0))) for i in range(len(sub)-1))\n"
        "auto_dur = max(12, min(120, int(math.ceil(arc / 0.30 + 10.0))))\n"
        "use_dur = int(req_dur) if req_dur else auto_dur\n"
        "out_traj_obj = dict(traj) if isinstance(traj, dict) else {'points': sub}\n"
        "out_traj_obj['points'] = sub\n"
        "with open(out_traj, 'w', encoding='utf-8') as f:\n"
        "    json.dump(out_traj_obj, f, ensure_ascii=False)\n"
        "scene['scene_id'] = f\"{scene.get('scene_id', 'seg')}_seg_{run_id[-4:]}\"\n"
        "scene['trajectory'] = out_traj\n"
        "scene['initial_x'] = ix\n"
        "scene['initial_y'] = iy\n"
        "scene['initial_yaw'] = iyaw\n"
        "scene['duration_s'] = use_dur\n"
        "with open(out_scene, 'w', encoding='utf-8') as f:\n"
        "    yaml.safe_dump(scene, f, allow_unicode=True, sort_keys=False)\n"
        "print(json.dumps({'ok': True, 'scene_path': out_scene, 'traj_path': out_traj,\n"
        "  'custom_trajectory': bool(custom_path), 'segmented': bool(seg),\n"
        "  'from_idx': fi, 'to_idx': ti, 'safe_start_idx': ss, 'safe_end_idx': se,\n"
        "  'total_points': n, 'point_count': len(sub), 'arc_len_m': round(arc, 2),\n"
        "  'init_pose': {'x': round(ix, 4), 'y': round(iy, 4), 'yaw': round(iyaw, 5)},\n"
        "  'duration_s': use_dur, 'auto_duration_s': auto_dur}, ensure_ascii=False))\n"
    )
    rc, out, err = _exec(f"python3 -c {shlex.quote(py_script)}", timeout=30, check=False)
    lines = [ln for ln in (out or "").splitlines() if ln.strip().startswith("{")]
    data = None
    if lines:
        try:
            data = json.loads(lines[-1])
        except ValueError:
            data = None
    if rc != 0 or not data or not data.get("ok"):
        code = (data or {}).get("error") or "SEGMENT_PREPARE_FAILED"
        detail = (data or {}).get("detail") or (err or out or "")[-300:]
        raise RunError(code, detail)
    return data


def _prepare_param_set_with_overrides(base_ps, new_ps, overrides=None, changes=None):
    """在容器内基于 base_ps 拷贝出合规参数集副本 new_ps，写入 param_set.yaml 的 changes 并用 sim_common/params 预校验。"""
    import shlex
    base_dir = f"{WS_IN_CONTAINER}/sim/param_sets/{base_ps}"
    new_dir = f"{WS_IN_CONTAINER}/sim/param_sets/{new_ps}"

    # 统一收集 changes（[{file, key, from, to}]）+ 兼容旧式 overrides（{key: val}）
    incoming = []
    seen_keys = set()
    for ch in (changes or []):
        if isinstance(ch, dict) and str(ch.get("key") or "").strip():
            k = str(ch["key"]).strip()
            incoming.append({
                "file": str(ch.get("file") or "").strip(),
                "key": k,
                "from": ch.get("from"),
                "to": ch.get("to"),
            })
            seen_keys.add(k)
    for k, v in (overrides or {}).items():
        ks = str(k).strip()
        if ks and ks not in seen_keys:
            incoming.append({"file": "", "key": ks, "from": None, "to": v})

    inc_json = json.dumps(incoming, ensure_ascii=False)
    intent_str = f"MapTool T4.3 参数集副本 ({new_ps})"
    py_script = (
        "import json, os, shutil, sys, yaml\n"
        f"ws_dir = {WS_IN_CONTAINER!r}\n"
        f"src = {base_dir!r}; dst = {new_dir!r}\n"
        f"base_ps = {base_ps!r}; new_ps = {new_ps!r}; intent_str = {intent_str!r}\n"
        f"incoming = json.loads({inc_json!r})\n"
        "shutil.rmtree(dst, ignore_errors=True)\n"
        "shutil.copytree(src, dst)\n"
        "snap_dir = os.path.join(dst, 'snapshot')\n"
        "ps_path = os.path.join(dst, 'param_set.yaml')\n"
        "if os.path.isfile(ps_path):\n"
        "  with open(ps_path, 'r', encoding='utf-8') as f:\n"
        "    ps_data = yaml.safe_load(f) or {}\n"
        "else:\n"
        "  ps_data = {'snapshot_dir': 'snapshot', 'changes': []}\n"
        "ps_data['param_set_id'] = new_ps\n"
        "ps_data['baseline'] = base_ps\n"
        "ps_data['intent'] = intent_str\n"
        "ps_data['snapshot_dir'] = ps_data.get('snapshot_dir') or 'snapshot'\n"
        "def _lookup(mapping, dotted):\n"
        "  cur = mapping\n"
        "  for part in dotted.split('.'):\n"
        "    if not isinstance(cur, dict) or part not in cur:\n"
        "      return False, None\n"
        "    cur = cur[part]\n"
        "  return True, cur\n"
        "def _coerce(val, ref):\n"
        "  if not isinstance(val, str):\n"
        "    return val\n"
        "  s = val.strip()\n"
        "  if isinstance(ref, bool):\n"
        "    if s.lower() in ('true', '1', 'yes'): return True\n"
        "    if s.lower() in ('false', '0', 'no'): return False\n"
        "  elif isinstance(ref, (int, float)):\n"
        "    try:\n"
        "      if isinstance(ref, int) and '.' not in s and 'e' not in s.lower():\n"
        "        return int(s)\n"
        "      return float(s)\n"
        "    except ValueError:\n"
        "      pass\n"
        "  return val\n"
        "def _find_key(dotted):\n"
        "  cands = []\n"
        "  for dp, dns, fns in os.walk(snap_dir):\n"
        "    dns.sort()\n"
        "    for fn in sorted(fns):\n"
        "      if fn.endswith(('.yaml', '.yml')):\n"
        "        fp = os.path.join(dp, fn)\n"
        "        with open(fp, 'r', encoding='utf-8') as f: d = yaml.safe_load(f) or {}\n"
        "        ok, real_v = _lookup(d, dotted)\n"
        "        if ok:\n"
        "          cands.append((os.path.relpath(fp, snap_dir).replace('\\\\', '/'), real_v))\n"
        "  if not cands:\n"
        "    print(json.dumps({'error': 'PARAM_KEY_NOT_FOUND', 'detail': f'在 snapshot 中找不到参数: {dotted}'}, ensure_ascii=False))\n"
        "    sys.exit(2)\n"
        "  return cands[0]\n"
        "merged = list(ps_data.get('changes') or [])\n"
        "for item in incoming:\n"
        "  f_rel = item['file'].strip().lstrip('/').replace('\\\\', '/')\n"
        "  k = item['key']\n"
        "  if not f_rel:\n"
        "    f_rel, real_from = _find_key(k)\n"
        "  else:\n"
        "    fp = os.path.join(snap_dir, f_rel)\n"
        "    if not os.path.isfile(fp):\n"
        "      print(json.dumps({'error': 'PARAM_FILE_NOT_FOUND', 'detail': f'snapshot 中不存在文件: {f_rel}'}, ensure_ascii=False))\n"
        "      sys.exit(2)\n"
        "    with open(fp, 'r', encoding='utf-8') as f: d = yaml.safe_load(f) or {}\n"
        "    ok, real_from = _lookup(d, k)\n"
        "    if not ok:\n"
        "      print(json.dumps({'error': 'PARAM_KEY_NOT_FOUND', 'detail': f'{f_rel} 中找不到参数: {k}'}, ensure_ascii=False))\n"
        "      sys.exit(2)\n"
        "  to_val = _coerce(item['to'], real_from)\n"
        "  updated = False\n"
        "  for cur in merged:\n"
        "    if cur.get('file', '').replace('\\\\', '/') == f_rel and cur.get('key') == k:\n"
        "      cur['from'] = real_from\n"
        "      cur['to'] = to_val\n"
        "      updated = True\n"
        "      break\n"
        "  if not updated:\n"
        "    merged.append({'file': f_rel, 'key': k, 'from': real_from, 'to': to_val})\n"
        "ps_data['changes'] = merged\n"
        "with open(ps_path, 'w', encoding='utf-8') as f:\n"
        "  yaml.safe_dump(ps_data, f, allow_unicode=True, sort_keys=False)\n"
        "sys.path.insert(0, os.path.join(ws_dir, 'sim'))\n"
        "from sim_common import params\n"
        "ps = params.load_param_set(dst)\n"
        "plan = params.resolve_changes(ps)\n"
        "print(json.dumps({'param_set_id': new_ps, 'changes': ps['changes'], 'param_plan': plan}, ensure_ascii=False))\n"
    )
    rc, out, err = _exec(f"python3 -c {shlex.quote(py_script)}", timeout=30, check=False)
    lines = [ln.strip() for ln in (out or "").splitlines() if ln.strip()]
    if rc != 0:
        if lines:
            try:
                err_obj = json.loads(lines[-1])
                if "error" in err_obj:
                    raise RunError(err_obj["error"], err_obj.get("detail", ""))
            except ValueError:
                pass
        raise RunError("CONTAINER_CMD_FAILED",
                       f"param_set 准备失败 rc={rc}: {(err or out)[-400:]}")
    if not lines:
        return {"param_set_id": new_ps, "changes": incoming, "param_plan": []}
    try:
        return json.loads(lines[-1])
    except ValueError:
        return {"param_set_id": new_ps, "changes": incoming, "param_plan": []}


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
        segment_info = None
        traj_json = req.get("traj_json") or {}
        has_custom_traj = (isinstance(traj_json, dict)
                           and isinstance(traj_json.get("points"), list)
                           and len(traj_json["points"]) >= 2)
        if req.get("segment") or has_custom_traj:
            status("queued", 0.27, "写入画布轨迹 / 裁剪 L2 分段并计算安全起步点")
            scene_idx = args.index("--scene") + 1
            dur_idx = args.index("--duration") + 1
            base_scene = args[scene_idx]
            segment_info = _prepare_segment_scene(
                base_scene, req.get("segment"), run_id,
                total_duration_s=venue.get("duration_s") or 76,
                req_duration_s=req.get("duration_s"),
                custom_traj_json=traj_json if has_custom_traj else None,
            )
            if segment_info:
                args[scene_idx] = segment_info["scene_path"]
                args[dur_idx] = str(int(segment_info["duration_s"]))
                init_pose = segment_info["init_pose"]
        overrides = req.get("param_overrides") or {}
        changes = req.get("param_changes") or []
        param_meta = {"param_set_id": candidate_ps, "changes": [], "param_plan": []}
        if changes or overrides:
            base_ps = req.get("param_set") or baseline_ps
            tmp_ps = candidate_ps
            param_meta = _prepare_param_set_with_overrides(
                base_ps, tmp_ps, overrides=overrides, changes=changes)

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

        # T4.2 补丁：b_line_run.py 的 wrapper launch 路径若漏传 initial_x/y/yaw，自动补齐
        import shlex
        patch_py = (
            "import os\n"
            "anchor = '        fh.write(\\'    <arg name=\"bag\" value=\"%s\" />\\\\n\\' % _xml(recorded_bag))'\n"
            "extra = (\n"
            "    anchor + '\\n'\n"
            "    + '        fh.write(\\'    <arg name=\"initial_x\" value=\"%s\" />\\\\n\\' % _xml(initial_pose[\"x\"]))\\n'\n"
            "    + '        fh.write(\\'    <arg name=\"initial_y\" value=\"%s\" />\\\\n\\' % _xml(initial_pose[\"y\"]))\\n'\n"
            "    + '        fh.write(\\'    <arg name=\"initial_yaw\" value=\"%s\" />\\\\n\\' % _xml(initial_pose[\"yaw\"]))'\n"
            ")\n"
            "for p in ('/root/catkin_ws/sim/scripts/b_line_run.py', '/root/catkin_ws/variant_tmp/sim/scripts/b_line_run.py'):\n"
            "    if not os.path.isfile(p): continue\n"
            "    s = open(p).read()\n"
            "    if 'name=\"initial_x\"' in s: continue\n"
            "    if anchor in s:\n"
            "        open(p, 'w').write(s.replace(anchor, extra, 1))\n"
        )
        _exec(f"python3 -c {shlex.quote(patch_py)}", timeout=20, check=False)

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
                "candidate_param_set": candidate_ps,
                "param_set_id": param_meta.get("param_set_id") or candidate_ps,
                "param_changes": param_meta.get("changes", []),
                "param_plan": param_meta.get("param_plan", []),
                "segment_info": segment_info}
    finally:
        # ps_{run_id} 参数集副本持久化保留用于追溯对比；仅清理临时 .tmp_ 前缀目录
        if tmp_ps and tmp_ps.startswith(".tmp_"):
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
