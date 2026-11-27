"""T2.3 live_bridge.py (JSON Lines) → timeline schema（T0.1）适配器。

live_bridge 每行一条 JSON：
  {"type":"hello","duration":..,"line":"B"}
  {"type":"state","t":..,"x":..,"y":..,"yaw":..,"speed":..,"progress":..}
  {"type":"points","t":..,"pts":[[x,y,z],..]}      ← 抽稀 220 点，map 系
  {"type":"path","path":[[x,y],..]}
  {"type":"obstacles","obstacles":[{id,x,y,yaw,speed,type}]}

我们转成：
  frame      ← state（5 Hz）
  front_hits ← points 中位于车位姿前视锥内的点（≤50，1-2 Hz 降频）
  path/obstacles 单独透传（MapTool 画参考线/障碍真值）

关键设计：**前视命中在宿主机侧用 L0 同一套几何判定**，这样 L1 直播与 L0 预判
用完全相同的 front_hits 语义，T3.2 一致率校准才有可比性。
"""
import math

from planner_limits import (PlannerLimits, TrajectorySceneMatcher,
                            compute_lookahead_dist, pick_scene,
                            scene_front_dist)

MAX_HITS = 50
HIT_EMIT_EVERY = 3   # points 约 2.5 Hz → 命中约 0.8 Hz（区间 1-2 Hz 内）


def yaw_diff(a, b):
    d = a - b
    while d > math.pi:
        d -= 2 * math.pi
    while d <= -math.pi:
        d += 2 * math.pi
    return d


def front_hits_from_points(pts, x, y, yaw, front_dist=3.0, half_angle=0.5,
                           max_hits=MAX_HITS):
    """从一帧点云里筛出前视锥内的命中点（世界坐标，按距离排序取前 N）。

    与 L0 cast_fov 的语义对齐：锥内、距离 ≤ front_dist、按最近优先限流。
    """
    out = []
    cos_lim = math.cos(half_angle)
    for p in pts:
        if len(p) < 2:
            continue
        dx = p[0] - x
        dy = p[1] - y
        d = math.hypot(dx, dy)
        if d < 1e-6 or d > front_dist:
            continue
        if (dx * math.cos(yaw) + dy * math.sin(yaw)) / d < cos_lim:
            continue    # 锥外
        out.append((d, p[0], p[1]))
    out.sort(key=lambda t: t[0])
    return [[round(hx, 3), round(hy, 3)] for _, hx, hy in out[:max_hits]]


def first_hit_dist(hits, x, y):
    if not hits:
        return None
    return round(min(math.hypot(h[0] - x, h[1] - y) for h in hits), 3)


class TimelineAdapter:
    """把 live_bridge 的行流转换成 timeline 消息。有状态：记住最近位姿与降频计数。"""

    def __init__(self, front_dist=3.0, half_angle=0.5, hit_every=HIT_EMIT_EVERY,
                 traj_points=None, planner_limits=None,
                 use_planner_front_dist=None):
        self.front_dist = front_dist
        self.half_angle = half_angle
        self.hit_every = hit_every
        self.pose = None          # 最近 (x, y, yaw)
        self._pt_seq = 0
        self._last_path = None
        self._last_obstacles = None
        # T3.1: 最近一次观测到的 lookahead/carrot（由 state 消息捎带，见下）
        self.lookahead = None
        self.carrot = None
        self._carrot_local = None
        # T3.4：按轨迹点属性分档的前视距离（未传 traj_points 时完全沿用固定值）
        self._planner_limits = planner_limits or PlannerLimits()
        self._scene_matcher = None
        if traj_points and use_planner_front_dist is not False:
            m = TrajectorySceneMatcher(traj_points, self._planner_limits)
            if len(m):
                self._scene_matcher = m
        self._current_scene = None
        self._current_front_dist = None
        self._prev_pose_t = None   # (t, yaw)

    def _update_scene(self, obj, x, y):
        """优先用 live_bridge 直推的属性/场景，否则按位姿匹配轨迹最近点。"""
        flags = obj.get("point_flags")
        if isinstance(flags, dict):
            scene = pick_scene(flags.get("curve_paths", 0),
                               flags.get("turnAround", flags.get("turn_around", 0)),
                               flags.get("bypass", 0))
            self._current_scene = scene
            self._current_front_dist = scene_front_dist(self._planner_limits, scene)
            return
        if isinstance(obj.get("scene"), str):
            scene = obj["scene"]
            self._current_scene = scene
            self._current_front_dist = scene_front_dist(self._planner_limits, scene)
            return
        if self._scene_matcher is None:
            return
        r = self._scene_matcher.match(x, y)
        if r is None:
            self._current_scene = None
            self._current_front_dist = None
        else:
            _, self._current_scene, self._current_front_dist = r

    def feed(self, obj):
        """输入一条 live_bridge JSON 对象，返回 timeline 消息列表（可能为空）。"""
        t = obj.get("type")
        if t == "hello":
            return [{"type": "status", "state": "replaying", "progress": 0.0,
                     "msg": f"live_bridge 已连接（{obj.get('line')} 线，"
                            f"{obj.get('duration')}s）"}]
        if t == "state":
            return self._on_state(obj)
        if t == "points":
            return self._on_points(obj)
        if t == "path":
            self._last_path = obj.get("path")
            return [{"type": "path", "path": self._last_path}]
        if t == "obstacles":
            self._last_obstacles = obj.get("obstacles")
            return [{"type": "obstacles", "obstacles": self._last_obstacles}]
        # T3.1 扩展：独立观测节点产出的 lookahead / carrot
        if t == "lookahead":
            self.lookahead = obj.get("dist")
            if "x" in obj and "y" in obj:
                self._carrot_local = (float(obj["x"]), float(obj["y"]))
            return []
        if t == "carrot":
            self.carrot = obj.get("xy")
            return []
        return []

    def _on_state(self, obj):
        if "x" not in obj:
            return []
        self.pose = (obj["x"], obj["y"], obj.get("yaw", 0.0))
        self._update_scene(obj, obj["x"], obj["y"])
        # 角速度：优先上游给的，否则由相邻帧 yaw 差估算（首帧 0）
        t_now = obj.get("t")
        yaw_now = float(obj.get("yaw", 0.0))
        omega = obj.get("angular_vel")
        if omega is None:
            omega = 0.0
            if self._prev_pose_t is not None and t_now is not None:
                dt = float(t_now) - self._prev_pose_t[0]
                if dt > 1e-3:
                    omega = yaw_diff(yaw_now, self._prev_pose_t[1]) / dt
        if t_now is not None:
            self._prev_pose_t = (float(t_now), yaw_now)
        carrot = obj.get("carrot", self.carrot)
        if carrot is None and self._carrot_local is not None:
            cx, cy = self._carrot_local
            yaw = float(obj.get("yaw", 0.0))
            c, s = math.cos(yaw), math.sin(yaw)
            carrot = [round(obj["x"] + c * cx - s * cy, 4),
                      round(obj["y"] + s * cx + c * cy, 4)]
        frame = {
            "t": obj.get("t", 0.0),
            "x": obj["x"], "y": obj["y"], "yaw": obj.get("yaw", 0.0),
            "speed": obj.get("speed", 0.0),
            "lookahead": obj.get("lookahead", self.lookahead),
            "loc_score": obj.get("loc_score"),
            "carrot": carrot,
            "front_hits": obj.get("front_hits"),
        }
        # 理论 lookahead（真实公式）；绝不覆盖实测的 frame["lookahead"]
        frame["lookahead_expected"] = round(compute_lookahead_dist(
            self._planner_limits, obj.get("speed", 0.0), omega), 4)
        if self._current_scene:
            frame["scene"] = self._current_scene
            frame["front_dist"] = round(self._current_front_dist or 0.0, 3)
        # progress 属 status 语义，单独发一条便于 UI 进度条
        msgs = [{"type": "frame", **frame}]
        if "progress" in obj:
            msgs.append({"type": "status", "state": "replaying",
                         "progress": round(float(obj["progress"]) / 100.0, 4),
                         "msg": ""})
        return msgs

    def _on_points(self, obj):
        self._pt_seq += 1
        if not self.pose or self._pt_seq % self.hit_every:
            return []
        x, y, yaw = self.pose
        pts = obj.get("pts") or []
        eff = (self._current_front_dist
               if self._current_front_dist and self._current_front_dist > 0
               else self.front_dist)
        hits = front_hits_from_points(pts, x, y, yaw, eff, self.half_angle)
        msg = {"type": "front_hits", "t": obj.get("t", 0.0), "hits": hits}
        if self._current_scene:
            msg["front_dist"] = round(eff, 3)
            msg["scene"] = self._current_scene
        d = first_hit_dist(hits, x, y)
        if d is not None:
            msg["first_hit_dist"] = d
        return [msg]
