"""T5.1 一键导出对比报告（HTML，单文件自包含）。

设计约束
--------
1. **数据全部来自 run 档案**（`data/runs/{run_id}/run.json` + 场地档案 + 容器
   `compare.json`），不手工整理任何数字。
2. **零第三方依赖**：容器/venv 里没有 matplotlib、PIL。地图底图用标准库
   `zlib` 自己编码灰度 PNG；所有矢量叠加（轨迹/carrot/命中点/车身框/曲线）
   用内联 SVG，输出单个 `.html` 文件，浏览器直接打开，也可直接"打印成 PDF"
   挂进验收单体系。
3. 报告章节与 WBS T5.1 对齐：
   摘要 → 门禁指标对照（8 项 F 门）→ 参数 diff → 轨迹对比图 →
   问题点位帧截图 → 时间序列 → 数据溯源附录。

对外只有两个函数：
    build_report(run_id, run, venue=None, ...) -> str      # 完整 HTML
    render_map_png(pgm_path, crop, step) -> (b64, w, h)    # 供其他模块复用
"""

from __future__ import annotations

import base64
import html
import io
import json
import math
import os
import struct
import zlib
from datetime import datetime
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple

# --------------------------------------------------------------------------
# 常量：与 MapTool / B 线工程对齐
# --------------------------------------------------------------------------

BODY_LEN_M = 1.85          # 实车车长（与 live_layer 一致）
BODY_WID_M = 1.50          # 实车车宽
FOV_HALF_ANGLE_DEG = 28.6  # 前视半锥角（与 T3.2/L0 一致）

# 8 项 F 门：(key, 中文名, 单位, 门限说明, 比较方式)
GATE_DEFS: List[Tuple[str, str, str, str]] = [
    ("tracking_rms", "轨迹跟踪 RMS", "m", "≤ 0.030"),
    ("tracking_signed_mean", "跟踪有符号均值（贴边偏向）", "m", "|x| ≤ 0.030"),
    ("clearance_min_diff", "最小净空差（毛刷）", "m", "|Δ| ≤ 0.030"),
    ("travel_diff", "行程差", "%", "|Δ| ≤ 2.0"),
    ("speed_diff", "均速差", "%", "|Δ| ≤ 5.0"),
    ("planner_mean_dev", "规划平均偏差", "m", "≤ 0.050"),
    ("theta_rms", "转角 RMS", "rad", "≤ 0.020"),
    ("collision", "碰撞", "次", "= 0"),
]

CLOSED_LOOP_DEFS: List[Tuple[str, str, str]] = [
    ("progress_100", "跑满全程", "progress_peak = 100%"),
    ("terminal", "到终点", "terminal = true"),
    ("collision", "无碰撞", "collision = false"),
    ("edge_rms", "贴边 RMS", "≤ 0.15 m（闭环口径）"),
    ("brush_min", "毛刷最小净空", "> 0.10 m"),
    ("no_failure_codes", "无失败码", "failure_codes = []"),
]


# --------------------------------------------------------------------------
# 极简 PNG 编码（8-bit 灰度，filter=0）——不需要任何图像库
# --------------------------------------------------------------------------

def png_gray(width: int, height: int, rows: Sequence[bytes]) -> bytes:
    """把若干行灰度像素（每行 width 字节）编码成 PNG 字节串。"""
    raw = bytearray()
    for r in rows:
        raw.append(0)          # filter type 0 (None)
        raw += r
    comp = zlib.compress(bytes(raw), 6)

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", comp)
            + chunk(b"IEND", b""))


def png_data_uri(width: int, height: int, rows: Sequence[bytes]) -> str:
    return "data:image/png;base64," + base64.b64encode(
        png_gray(width, height, rows)).decode("ascii")


# --------------------------------------------------------------------------
# PGM 读取与裁剪
# --------------------------------------------------------------------------

class PgmMap:
    """ROS 地图（P5 二进制 PGM）+ 世界坐标 ↔ 像素换算。"""

    def __init__(self, path: str, origin: Tuple[float, float], resolution: float):
        self.path = path
        self.ox, self.oy = float(origin[0]), float(origin[1])
        self.res = float(resolution) or 0.05
        with open(path, "rb") as f:
            magic = f.readline().strip()
            if magic != b"P5":
                raise ValueError(f"仅支持 P5 二进制 PGM：{path}（读到 {magic!r}）")
            line = f.readline()
            while line.startswith(b"#"):
                line = f.readline()
            while len(line.split()) < 2:
                line += b" " + f.readline()
            w, h = (int(v) for v in line.split()[:2])
            maxval = int(f.readline())
            data = f.read()
        if len(data) < w * h:
            raise ValueError(f"PGM 数据长度不足：{len(data)} < {w * h}")
        self.w, self.h, self.maxval, self.data = w, h, maxval, data

    # 世界 → 像素（float，x 列向右递增，y 行向下递增）
    def world_to_px(self, x: float, y: float) -> Tuple[float, float]:
        col = (x - self.ox) / self.res
        row = self.h - 1 - (y - self.oy) / self.res
        return col, row

    def crop(self, xmin: float, ymin: float, xmax: float, ymax: float,
             step: int = 4) -> "MapCrop":
        c0, r1 = self.world_to_px(xmin, ymin)   # ymin → 大 row
        c1, r0 = self.world_to_px(xmax, ymax)   # ymax → 小 row
        c0 = max(0, min(self.w - 1, int(math.floor(c0))))
        c1 = max(0, min(self.w - 1, int(math.ceil(c1))))
        r0 = max(0, min(self.h - 1, int(math.floor(r0))))
        r1 = max(0, min(self.h - 1, int(math.ceil(r1))))
        step = max(1, int(step))
        rows = []
        for r in range(r0, r1 + 1, step):
            base = r * self.w
            rows.append(self.data[base + c0: base + c1 + 1: step])
        if not rows:
            raise ValueError("裁剪区域为空")
        return MapCrop(self, c0, r0, step, rows, c1 - c0 + 1)


class MapCrop:
    """已裁剪并降采样的地图瓦片，负责世界坐标 → 瓦片像素。"""

    def __init__(self, m: PgmMap, c0: int, r0: int, step: int,
                 rows: List[bytes], full_w: int):
        self.map = m
        self.c0, self.r0, self.step = c0, r0, step
        self.rows = rows
        self.h = len(rows)
        self.w = len(rows[0]) if rows else 0
        self.full_w = full_w
        # 瓦片像素 → 世界坐标的逆变换（用于画比例尺/坐标轴）
        self.m_per_px = m.res * step

    def to_px(self, x: float, y: float) -> Tuple[float, float]:
        col, row = self.map.world_to_px(x, y)
        return (col - self.c0) / self.step, (row - self.r0) / self.step

    def px_to_world(self, px: float, py: float) -> Tuple[float, float]:
        col = px * self.step + self.c0
        row = py * self.step + self.r0
        return (self.map.ox + col * self.map.res,
                self.map.oy + (self.map.h - 1 - row) * self.map.res)

    # 显示用：把 0..255 原始栅格转成"浅底深障"的 RGB 行（PNG 只支持灰度，
    # 所以这里仍输出灰度，只是把中间值映射成更易读的对比度）
    def gray_rows(self) -> List[bytes]:
        out = []
        for r in self.rows:
            out.append(bytes(250 if v > 240 else (10 if v < 50 else
                                                  int(120 + (v - 50) * 0.5))
                             for v in r))
        return out

    def data_uri(self) -> str:
        return png_data_uri(self.w, self.h, self.gray_rows())


# --------------------------------------------------------------------------
# 小工具
# --------------------------------------------------------------------------

def _esc(v: Any) -> str:
    return html.escape("" if v is None else str(v))


def _fmt(v: Any, nd: int = 3) -> str:
    if v is None:
        return "—"
    if isinstance(v, bool):
        return "是" if v else "否"
    try:
        f = float(v)
    except (TypeError, ValueError):
        return _esc(v)
    if math.isnan(f) or math.isinf(f):
        return "—"
    if f == int(f) and abs(f) < 1e6:
        return str(int(f))
    return f"{f:.{nd}f}"


def _pct(v: Any, nd: int = 2) -> str:
    if v is None:
        return "—"
    try:
        return f"{float(v):+.{nd}f}%"
    except (TypeError, ValueError):
        return "—"


def _badge(ok: Optional[bool], text_ok: str = "PASS",
           text_bad: str = "FAIL", text_na: str = "无数据") -> str:
    if ok is None:
        return f'<span class="badge na">{text_na}</span>'
    cls = "pass" if ok else "fail"
    return f'<span class="badge {cls}">{text_ok if ok else text_bad}</span>'


def _verdict_class(v: Optional[str]) -> str:
    if not v:
        return "na"
    v = str(v).upper()
    if "OK" in v and "BOUNDARY" not in v:
        return "pass"
    if "BOUNDARY" in v or "PARTIAL" in v or "TRADEOFF" in v or "EFFECT" in v:
        return "warn"
    if "DRIFT" in v or "FAIL" in v:
        return "fail"
    return "na"


def _downsample(points: Sequence[Tuple[float, float]], max_n: int = 1500
                ) -> List[Tuple[float, float]]:
    if len(points) <= max_n:
        return list(points)
    k = max(1, len(points) // max_n)
    out = [points[i] for i in range(0, len(points), k)]
    if out[-1] != points[-1]:
        out.append(points[-1])
    return out


def _polyline_d(points: Iterable[Tuple[float, float]], nd: int = 1) -> str:
    pts = list(points)
    if not pts:
        return ""
    parts = [f"M{pts[0][0]:.{nd}f},{pts[0][1]:.{nd}f}"]
    for x, y in pts[1:]:
        parts.append(f"L{x:.{nd}f},{y:.{nd}f}")
    return "".join(parts)


# --------------------------------------------------------------------------
# 数据抽取：把 run 档案 / compare.json 归一化
# --------------------------------------------------------------------------

def _get(d: Optional[dict], *path, default=None):
    cur: Any = d
    for p in path:
        if not isinstance(cur, dict):
            return default
        cur = cur.get(p)
        if cur is None:
            return default
    return cur


def extract_gates(record: Optional[dict]) -> Dict[str, Any]:
    """从 compare.json（runner 已合并进 record['compare']）抽出 8 项门数据。

    口径严格照抄容器侧 `bag_replay_diff.py` 的 `checks`：仿真侧一律叫
    `baseline`，真值侧叫 `truth`。
    """
    cmp = (record or {}).get("compare") or {}
    fid = cmp.get("fidelity") or {}
    checks = fid.get("checks") or {}
    not_comp = fid.get("not_computable") or []
    boundary = fid.get("known_boundary") or {}

    trk = _get(fid, "tracking", "baseline", default={}) or {}
    trk_truth = _get(fid, "tracking", "truth", default={}) or {}
    brush_t = _get(fid, "clearance", "truth", "brush_clearance", "min")
    brush_b = _get(fid, "clearance", "baseline", "brush_clearance", "min")
    brush_diff = (None if brush_t is None or brush_b is None
                  else float(brush_b) - float(brush_t))

    values: Dict[str, Any] = {
        "tracking_rms": trk.get("rms"),
        "tracking_signed_mean": trk.get("mean_signed"),
        "clearance_min_diff": brush_diff,
        "travel_diff": _get(fid, "travel", "delta_pct"),
        "speed_diff": _get(fid, "speed", "mean_delta_pct"),
        "planner_mean_dev": _get(fid, "planner_vs_truth",
                                "mean_deviation_m", "mean"),
        "theta_rms": _get(fid, "theta_vs_truth", "rms"),
        "collision": _get(fid, "collisions", "nonzero"),
    }
    return {
        "checks": checks,
        "values": values,
        "not_computable": list(not_comp),
        "known_boundary": boundary,
        "verdict": fid.get("verdict"),
        "overall": _get(cmp, "verdict", "overall"),
        "screening": (cmp.get("screening") or {}).get("verdict"),
        "closed_loop": cmp.get("closed_loop") or {},
        "screening_block": cmp.get("screening") or {},
        "applied": cmp.get("applied") or {},
        "extra": {
            "truth_brush_min": brush_t,
            "sim_brush_min": brush_b,
            "truth_rms": trk_truth.get("rms"),
            "pose_delta_median": _get(fid, "pose_delta_m", "median"),
            "pose_delta_mean": _get(fid, "pose_delta_m", "mean"),
            "pose_delta_p95": _get(fid, "pose_delta_m", "p95"),
            "truth_travel_m": _get(fid, "travel", "truth_m"),
            "sim_travel_m": _get(fid, "travel", "baseline_m"),
            "truth_speed": _get(fid, "speed", "truth", "speed_mean"),
            "sim_speed": _get(fid, "speed", "baseline", "speed_mean"),
            "planner_matched": _get(fid, "planner_vs_truth", "matched"),
        },
    }


def extract_frames(run: Optional[dict]) -> List[dict]:
    return [f for f in ((run or {}).get("frames") or []) if isinstance(f, dict)]


def _frame_xy(f: dict) -> Optional[Tuple[float, float]]:
    x, y = f.get("x"), f.get("y")
    if x is None or y is None:
        return None
    return float(x), float(y)


def find_issues(frames: Sequence[dict]) -> List[dict]:
    """自动挑"问题点位"：报告不靠人手挑帧，规则写死可复现。

    规则（按严重度排序，每类最多 3 个代表帧，且彼此至少隔 1 s）：
      A. 碰撞/前视命中（front_hits 非空，且首个命中 < 0.5 m）—— 最硬的证据
      B. 静默停车（speed=0 但进度未满 100%）
      C. 数据缺口（有车有速却缺 carrot / lookahead）
      D. 停车时长最长的连续段起点
    """
    issues: List[dict] = []

    def add(kind: str, level: str, f: dict, why: str) -> None:
        issues.append({"kind": kind, "level": level, "frame": f,
                       "t": f.get("t"), "why": why})

    # A. 前视命中
    hit_count = 0
    last_t = -99.0
    for f in frames:
        hits = f.get("front_hits")
        if not hits:
            continue
        d = f.get("first_hit_dist")
        if d is None:
            xy = _frame_xy(f)
            if xy:
                d = min(math.hypot(h[0] - xy[0], h[1] - xy[1]) for h in hits
                        if len(h) >= 2)
        if d is None or d > 0.5:
            continue
        if float(f.get("t") or 0) - last_t < 1.0 or hit_count >= 3:
            continue
        add("front_hit", "high", f, f"前视命中 {float(d):.2f} m（{len(hits)} 点）")
        last_t = float(f.get("t") or 0)
        hit_count += 1

    # B. 静默停车（speed≈0 且 lookahead 有值 = planner 在算但车不动）
    zero_seen = 0
    last_t = -99.0
    for f in frames:
        sp = f.get("speed")
        if sp is None or float(sp) > 1e-3:
            continue
        if float(f.get("t") or 0) < 2.0:    # 起步前静止不算问题
            continue
        if float(f.get("t") or 0) - last_t < 5.0 or zero_seen >= 2:
            continue
        add("stopped", "high", f,
            "speed=0（planner 仍在出 carrot/lookahead，车未动）"
            if f.get("carrot") else "speed=0")
        last_t = float(f.get("t") or 0)
        zero_seen += 1

    # C. 数据缺口
    gap = None
    for f in frames:
        if f.get("speed") is None:
            continue
        if f.get("carrot") is None and f.get("lookahead") is None:
            if gap is None:
                gap = f
    if gap is not None:
        add("gap", "mid", gap, "缺 carrot 与 lookahead（planner 未参与控制）")

    # D. 全程兜底：末帧
    if not issues and frames:
        add("tail", "low", frames[-1], "无异常规则命中，附末帧供人工复核")
    return issues


# --------------------------------------------------------------------------
# SVG 渲染
# --------------------------------------------------------------------------

def _svg_vehicle(px: float, py: float, yaw: float, m_per_px: float,
                 color: str = "#dc2626", opacity: float = 1.0) -> str:
    """真实比例车身矩形 + 车头方向指示（世界 yaw → 像素坐标需翻转 y）。"""
    L = BODY_LEN_M / m_per_px
    W = BODY_WID_M / m_per_px
    # 像素坐标系 y 向下 = 世界 y 向上取反，故 yaw 取负
    deg = -math.degrees(yaw)
    l, w = max(L, 3.0), max(W, 3.0)
    return (f'<g transform="translate({px:.1f},{py:.1f}) rotate({deg:.2f})" '
            f'opacity="{opacity}">'
            f'<rect x="{-l / 2:.1f}" y="{-w / 2:.1f}" width="{l:.1f}" '
            f'height="{w:.1f}" fill="none" stroke="{color}" '
            f'stroke-width="1.2"/>'
            f'<line x1="{l / 2:.1f}" y1="0" x2="{l * 0.9:.1f}" y2="0" '
            f'stroke="{color}" stroke-width="2"/>'
            f'</g>')


def _svg_fov(px: float, py: float, yaw: float, front_m: float,
             m_per_px: float, color: str = "#f59e0b") -> str:
    r = front_m / m_per_px
    a = math.radians(FOV_HALF_ANGLE_DEG)
    # 世界方向角 → 像素：角度取负，且 y 轴向下
    pts = []
    for ang in (yaw - a, yaw + a):
        pts.append((px + r * math.cos(ang), py - r * math.sin(ang)))
    return (f'<path d="M{px:.1f},{py:.1f} L{pts[0][0]:.1f},{pts[0][1]:.1f} '
            f'A{r:.1f},{r:.1f} 0 0 0 {pts[1][0]:.1f},{pts[1][1]:.1f} Z" '
            f'fill="{color}" fill-opacity="0.10" stroke="{color}" '
            f'stroke-opacity="0.55" stroke-width="0.8"/>')


def render_map_svg(crop: MapCrop, run: dict, frames: Sequence[dict],
                   issues: Sequence[dict], max_issues: int = 6) -> str:
    """轨迹对比图：地图底图（PNG，见 HTML）+ 本 SVG 矢量叠加。"""
    W, Hh = crop.w, crop.h
    parts: List[str] = [f'<svg class="overlay" viewBox="0 0 {W} {Hh}" '
                        f'preserveAspectRatio="xMidYMid meet" '
                        f'xmlns="http://www.w3.org/2000/svg">']

    def to_px(xy: Tuple[float, float]) -> Tuple[float, float]:
        return crop.to_px(xy[0], xy[1])

    # 规划轨迹（run.path 优先，其次场地 default_traj 已读入 run["req"]["traj_json"]）
    path_pts: List[Tuple[float, float]] = []
    for p in (run.get("path") or []):
        if isinstance(p, (list, tuple)) and len(p) >= 2:
            path_pts.append((float(p[0]), float(p[1])))
    if not path_pts:
        for p in _get(run, "req", "traj_json", "points", default=[]) or []:
            if isinstance(p, dict) and "x" in p and "y" in p:
                path_pts.append((float(p["x"]), float(p["y"])))
    if path_pts:
        d = _polyline_d(_downsample([to_px(p) for p in path_pts], 3000))
        parts.append(f'<path d="{d}" fill="none" stroke="#2563eb" '
                     f'stroke-width="1.6" stroke-opacity="0.85"/>')

    # 实际行驶轨迹
    act: List[Tuple[float, float]] = []
    for f in frames:
        xy = _frame_xy(f)
        if xy:
            act.append(to_px(xy))
    if len(act) >= 2:
        d = _polyline_d(_downsample(act, 3000))
        parts.append(f'<path d="{d}" fill="none" stroke="#16a34a" '
                     f'stroke-width="1.8" stroke-dasharray="5 3"/>')

    # carrot（绿十字，抽样避免糊图）
    carrots = [(to_px((float(c[0]), float(c[1]))), f)
               for f in frames if isinstance(f.get("carrot"), (list, tuple))
               and len(f["carrot"]) >= 2
               for c in [f["carrot"]]]
    if carrots:
        step = max(1, len(carrots) // 120)
        for (cx, cy), _f in carrots[::step]:
            parts.append(f'<path d="M{cx - 3:.1f},{cy:.1f}H{cx + 3:.1f}'
                         f'M{cx:.1f},{cy - 3:.1f}V{cy + 3:.1f}" '
                         f'stroke="#16a34a" stroke-width="1" '
                         f'stroke-opacity="0.75"/>')

    # 前视命中点（红点）
    hits_all = []
    for f in frames:
        for h in (f.get("front_hits") or [])[:50]:
            if len(h) >= 2:
                hits_all.append(to_px((float(h[0]), float(h[1]))))
    if hits_all:
        step = max(1, len(hits_all) // 800)
        for hx, hy in hits_all[::step]:
            parts.append(f'<circle cx="{hx:.1f}" cy="{hy:.1f}" r="1.6" '
                         f'fill="#dc2626" fill-opacity="0.7"/>')

    # 起终点
    if act:
        sx, sy = act[0]
        ex, ey = act[-1]
        parts.append(f'<circle cx="{sx:.1f}" cy="{sy:.1f}" r="4.5" '
                     f'fill="#0ea5e9" stroke="#fff" stroke-width="1"/>')
        parts.append(f'<circle cx="{ex:.1f}" cy="{ey:.1f}" r="4.5" '
                     f'fill="#9333ea" stroke="#fff" stroke-width="1"/>')

    # 问题点位（编号与下方截图一一对应）
    for i, it in enumerate(issues[:max_issues], 1):
        xy = _frame_xy(it["frame"])
        if not xy:
            continue
        px, py = to_px(xy)
        color = "#dc2626" if it["level"] == "high" else "#f59e0b"
        parts.append(f'<circle cx="{px:.1f}" cy="{py:.1f}" r="7" '
                     f'fill="none" stroke="{color}" stroke-width="1.8"/>')
        parts.append(f'<text x="{px + 10:.1f}" y="{py - 8:.1f}" '
                     f'font-size="11" font-weight="700" fill="{color}" '
                     f'font-family="system-ui,sans-serif">{i}</text>')

    # 末帧车身姿态
    if frames:
        last = frames[-1]
        xy = _frame_xy(last)
        if xy:
            px, py = to_px(xy)
            if last.get("carrot"):
                fl = float(last.get("lookahead") or 0) or 3.0
                parts.append(_svg_fov(px, py, float(last.get("yaw") or 0),
                                      max(fl, 1.0), crop.m_per_px))
            parts.append(_svg_vehicle(px, py, float(last.get("yaw") or 0),
                                      crop.m_per_px))

    # 比例尺（左下角 5 m）
    bar = 5.0 / crop.m_per_px
    pad = 8
    y0 = Hh - pad
    parts.append(f'<g stroke="#111" stroke-width="2">'
                 f'<line x1="{pad}" y1="{y0}" x2="{pad + bar:.1f}" y2="{y0}"/>'
                 f'</g><text x="{pad}" y="{y0 - 5:.1f}" font-size="10" '
                 f'font-family="system-ui,sans-serif" fill="#111">5 m</text>')
    parts.append("</svg>")
    return "".join(parts)


def render_issue_thumb(crop: MapCrop, issue: dict,
                       pad_m: float = 4.0) -> Tuple[str, str]:
    """问题点位帧截图：从地图瓦片里裁一小块 + 车身 + 前视锥 + 命中点。

    返回 (data_uri_png, svg_overlay)。
    """
    f = issue["frame"]
    xy = _frame_xy(f)
    if not xy:
        return "", ""
    m = crop.map
    xmin, ymin = xy[0] - pad_m, xy[1] - pad_m
    xmax, ymax = xy[0] + pad_m, xy[1] + pad_m
    try:
        sub = m.crop(xmin, ymin, xmax, ymax, step=max(1, crop.step // 2))
    except ValueError:
        return "", ""
    uri = sub.data_uri()

    px, py = sub.to_px(xy[0], xy[1])
    yaw = float(f.get("yaw") or 0.0)
    parts = [f'<svg viewBox="0 0 {sub.w} {sub.h}" '
             f'preserveAspectRatio="xMidYMid meet" class="overlay" '
             f'xmlns="http://www.w3.org/2000/svg">']
    la = f.get("lookahead")
    if la:
        parts.append(_svg_fov(px, py, yaw, max(float(la), 1.0), sub.m_per_px))
    for h in (f.get("front_hits") or [])[:50]:
        if len(h) >= 2:
            hx, hy = sub.to_px(float(h[0]), float(h[1]))
            parts.append(f'<circle cx="{hx:.1f}" cy="{hy:.1f}" r="2" '
                         f'fill="#dc2626"/>')
    if f.get("carrot"):
        cx, cy = sub.to_px(float(f["carrot"][0]), float(f["carrot"][1]))
        parts.append(f'<path d="M{cx - 4:.1f},{cy:.1f}H{cx + 4:.1f}'
                     f'M{cx:.1f},{cy - 4:.1f}V{cy + 4:.1f}" '
                     f'stroke="#16a34a" stroke-width="1.6"/>')
    parts.append(_svg_vehicle(px, py, yaw, sub.m_per_px))
    parts.append("</svg>")
    return uri, "".join(parts)


def render_timeseries(frames: Sequence[dict], key: str, color: str,
                      label: str, unit: str) -> str:
    """单指标时间序列（SVG），自动跳过 None。"""
    pts: List[Tuple[float, float]] = []
    for f in frames:
        v = f.get(key)
        if v is None:
            continue
        try:
            pts.append((float(f.get("t") or 0), float(v)))
        except (TypeError, ValueError):
            continue
    if len(pts) < 2:
        return f'<p class="muted">{_esc(label)}：该 run 无可用数据。</p>'

    W, Hh = 880, 150
    pl, pr, pt, pb = 52, 12, 12, 26
    t0, t1 = pts[0][0], pts[-1][0]
    vs = [p[1] for p in pts]
    v0, v1 = min(vs), max(vs)
    if v1 - v0 < 1e-9:
        v0, v1 = v0 - 0.5, v1 + 0.5
    span = (v1 - v0) * 1.1
    v0, v1 = v0 - span * 0.05, v1 + span * 0.05
    ts = max(t1 - t0, 1e-6)

    def X(t: float) -> float:
        return pl + (t - t0) / ts * (W - pl - pr)

    def Y(v: float) -> float:
        return pt + (1 - (v - v0) / (v1 - v0)) * (Hh - pt - pb)

    step = max(1, len(pts) // 1200)
    d = _polyline_d([(X(t), Y(v)) for t, v in pts[::step]], 1)
    grid = []
    for i in range(5):
        v = v0 + (v1 - v0) * i / 4
        y = Y(v)
        grid.append(f'<line x1="{pl}" y1="{y:.1f}" x2="{W - pr}" '
                    f'y2="{y:.1f}" stroke="#e5e7eb" stroke-width="1"/>'
                    f'<text x="{pl - 6}" y="{y + 3.5:.1f}" font-size="9" '
                    f'text-anchor="end" fill="#6b7280" '
                    f'font-family="system-ui,sans-serif">{v:.2f}</text>')
    ticks = []
    for i in range(5):
        t = t0 + ts * i / 4
        x = X(t)
        ticks.append(f'<text x="{x:.1f}" y="{Hh - 8}" font-size="9" '
                     f'text-anchor="middle" fill="#6b7280" '
                     f'font-family="system-ui,sans-serif">{t:.0f}s</text>')
    return (f'<svg viewBox="0 0 {W} {Hh}" class="ts" xmlns="http://www.w3.org/2000/svg">'
            + "".join(grid) + "".join(ticks)
            + f'<path d="{d}" fill="none" stroke="{color}" stroke-width="1.4"/>'
            + f'<text x="{pl}" y="{pt + 10}" font-size="10" fill="{color}" '
              f'font-weight="600" font-family="system-ui,sans-serif">'
              f'{_esc(label)}（{_esc(unit)}）</text>'
            + "</svg>")


# --------------------------------------------------------------------------
# HTML 组装
# --------------------------------------------------------------------------

CSS = """
:root{--line:#e5e7eb;--ink:#111827;--muted:#6b7280;--pass:#16a34a;
      --fail:#dc2626;--warn:#f59e0b;--bg:#f9fafb}
*{box-sizing:border-box}
body{margin:0;padding:24px 32px 64px;font:14px/1.6 -apple-system,
     "PingFang SC","Helvetica Neue",system-ui,sans-serif;color:var(--ink);
     background:#fff;max-width:1180px}
h1{font-size:22px;margin:0 0 4px}
h2{font-size:16px;margin:32px 0 10px;padding-bottom:6px;
   border-bottom:2px solid var(--line)}
h3{font-size:13px;margin:16px 0 6px;color:var(--muted);
   text-transform:uppercase;letter-spacing:.04em}
.sub{color:var(--muted);font-size:12px;margin:0 0 16px}
table{border-collapse:collapse;width:100%;font-size:13px;margin:6px 0 4px}
th,td{border:1px solid var(--line);padding:6px 9px;text-align:left;
      vertical-align:top}
th{background:var(--bg);font-weight:600;font-size:12px;color:#374151}
td.num{text-align:right;font-variant-numeric:tabular-nums;
       font-family:ui-monospace,SFMono-Regular,Menlo,monospace}
tr.fail td{background:#fef2f2}
tr.pass td{background:#f0fdf4}
.badge{display:inline-block;padding:1px 8px;border-radius:10px;
       font-size:11px;font-weight:700}
.badge.pass{background:#dcfce7;color:#15803d}
.badge.fail{background:#fee2e2;color:#b91c1c}
.badge.warn{background:#fef3c7;color:#b45309}
.badge.na{background:#f3f4f6;color:#6b7280}
.kpis{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));
      gap:10px;margin:12px 0}
.kpi{border:1px solid var(--line);border-radius:8px;padding:9px 12px;
     background:var(--bg)}
.kpi .k{font-size:11px;color:var(--muted)}
.kpi .v{font-size:17px;font-weight:700;
        font-variant-numeric:tabular-nums}
.mapwrap{position:relative;display:inline-block;max-width:100%;
         border:1px solid var(--line);border-radius:6px;overflow:hidden;
         background:#fff}
.mapwrap img{display:block;width:100%;height:auto}
.overlay{position:absolute;inset:0;width:100%;height:100%}
.legend{font-size:11px;color:var(--muted);margin:6px 0 14px}
.legend i{display:inline-block;width:14px;height:3px;margin:0 4px 2px 12px;
          vertical-align:middle}
.thumbs{display:grid;grid-template-columns:repeat(auto-fill,minmax(260px,1fr));
        gap:14px;margin-top:10px}
.thumb{border:1px solid var(--line);border-radius:8px;padding:8px;
       background:var(--bg)}
.thumb .imgwrap{position:relative;aspect-ratio:1/1;overflow:hidden;
                border-radius:4px;background:#fff}
.thumb .meta{font-size:11px;color:#374151;margin-top:6px}
.thumb .t{font-weight:700}
.muted{color:var(--muted);font-size:12px}
.ts{width:100%;height:auto;margin:4px 0 10px}
code{background:#f3f4f6;padding:1px 4px;border-radius:3px;font-size:12px}
pre{background:var(--bg);border:1px solid var(--line);border-radius:6px;
    padding:10px;overflow:auto;font-size:12px}
.footer{margin-top:36px;padding-top:12px;border-top:1px solid var(--line);
        color:var(--muted);font-size:11px}
@media print{body{max-width:none;padding:0}
  h2{page-break-after:avoid}.thumb,.mapwrap{page-break-inside:avoid}}
"""


def _gates_table(g: Dict[str, Any]) -> str:
    checks = g.get("checks") or {}
    vals = g.get("values") or {}
    nc = set(g.get("not_computable") or [])
    rows = []
    passed = failed = na = 0
    for key, name, unit, limit in GATE_DEFS:
        v = vals.get(key)
        ok = checks.get(key)
        if key in nc:
            ok = None
        # 老版本 compare.json 没有 not_computable，缺数据时 checks 直接给 False。
        # 值本身为 None 就说明"测不出来"，不能算失败，否则报告会冤枉仿真。
        if ok is False and v is None:
            ok = None
        if ok is None:
            na += 1
            cls, badge = "", '<span class="badge na">无数据</span>'
        elif ok:
            passed += 1
            cls, badge = "pass", _badge(True)
        else:
            failed += 1
            cls, badge = "fail", _badge(False)
        shown = _pct(v) if key in ("travel_diff", "speed_diff") else _fmt(v, 4)
        rows.append(f'<tr class="{cls}"><td>{_esc(name)}</td>'
                    f'<td class="num">{shown}</td>'
                    f'<td>{_esc(unit)}</td><td>{_esc(limit)}</td>'
                    f'<td>{badge}</td></tr>')
    head = ('<table><thead><tr><th>门禁项</th><th>实测值</th><th>单位</th>'
            '<th>门限</th><th>结果</th></tr></thead><tbody>')
    tail = ('</tbody></table>'
            f'<p class="muted">通过 <b>{passed}</b> / 失败 <b>{failed}</b> / '
            f'无数据 <b>{na}</b>（共 8 项）。门限口径照抄容器侧 '
            f'<code>sim/scripts/bag_replay_diff.py</code>，未做任何放宽。</p>')
    return head + "".join(rows) + tail


def _closed_loop_table(cl: Dict[str, Any]) -> str:
    if not cl:
        return '<p class="muted">该 run 未产出闭环检查（旧 run 或非 bag_replay 模式）。</p>'
    checks = cl.get("checks") or {}
    summary = cl.get("summary") or {}
    rows = []
    for key, name, hint in CLOSED_LOOP_DEFS:
        ok = checks.get(key)
        if key == "edge_rms":
            val = summary.get("edge_rms")
        elif key == "brush_min":
            val = _get(cl, "clearance", "brush_clearance", "min")
        elif key == "collision":
            val = summary.get("collision")
        else:
            val = checks.get(key)
        shown = ("—" if val is None
                 else ("true" if val is True else "false" if val is False
                       else _fmt(val, 3)))
        rows.append(f'<tr><td>{_esc(name)}</td><td class="num">{shown}</td>'
                    f'<td>{_esc(hint)}</td><td>{_badge(ok)}</td></tr>')
    return ('<table><thead><tr><th>闭环检查</th><th>值</th><th>门限</th>'
            '<th>结果</th></tr></thead><tbody>'
            + "".join(rows) + '</tbody></table>')


def _param_table(run: dict, g: Dict[str, Any]) -> str:
    changes = run.get("param_changes") or _get(run, "req", "param_changes",
                                               default=[]) or []
    plan = run.get("param_plan") or []
    applied = (g.get("applied") or {}).get("rows") or []
    overrides = _get(run, "req", "param_overrides", default={}) or {}

    if not (changes or plan or applied or overrides):
        return ('<p class="muted">本次 run 未修改任何参数（全过程使用基线参数集 '
                f'<code>{_esc(run.get("baseline_param_set") or "默认")}</code>）。</p>')

    blocks = []
    if changes:
        rows = []
        for c in changes:
            if not isinstance(c, dict):
                continue
            rows.append(
                f'<tr><td>{_esc(c.get("file"))}</td>'
                f'<td><code>{_esc(c.get("key"))}</code></td>'
                f'<td class="num">{_esc(c.get("from"))}</td>'
                f'<td class="num">{_esc(c.get("to"))}</td></tr>')
        if rows:
            blocks.append('<h3>参数 diff（MapTool 提交）</h3>'
                          '<table><thead><tr><th>文件</th><th>参数</th>'
                          '<th>原值</th><th>新值</th></tr></thead><tbody>'
                          + "".join(rows) + "</tbody></table>")
    if overrides and not changes:
        rows = "".join(f'<tr><td><code>{_esc(k)}</code></td>'
                       f'<td class="num">{_esc(v)}</td></tr>'
                       for k, v in overrides.items())
        blocks.append('<h3>参数覆盖（param_overrides）</h3>'
                      '<table><thead><tr><th>参数</th><th>值</th></tr>'
                      f'</thead><tbody>{rows}</tbody></table>')
    if applied:
        rows = "".join(
            f'<tr><td><code>{_esc(r.get("name"))}</code></td>'
            f'<td class="num">{_esc(r.get("actual"))}</td>'
            f'<td class="num">{_esc(r.get("expected"))}</td>'
            f'<td>{_badge(str(r.get("actual")) == str(r.get("expected")))}</td></tr>'
            for r in applied if isinstance(r, dict))
        blocks.append('<h3>容器侧生效校验（compare.json → applied）</h3>'
                      '<table><thead><tr><th>ROS 参数</th><th>实际</th>'
                      '<th>期望</th><th>一致</th></tr></thead><tbody>'
                      f'{rows}</tbody></table>')

    meta = []
    if run.get("param_set_id"):
        meta.append(f'参数集副本 <code>{_esc(run["param_set_id"])}</code>')
    if run.get("baseline_param_set"):
        meta.append(f'基线 <code>{_esc(run["baseline_param_set"])}</code>')
    if plan:
        meta.append(f"计划 {len(plan)} 项")
    intro = f'<p class="muted">{"；".join(meta)}</p>' if meta else ""
    return intro + "".join(blocks)


def _issues_section(crop: Optional[MapCrop], issues: Sequence[dict]) -> str:
    if not issues:
        return '<p class="muted">未检出问题点位。</p>'
    cards = []
    for i, it in enumerate(issues, 1):
        f = it["frame"]
        color = "var(--fail)" if it["level"] == "high" else "var(--warn)"
        uri, overlay = ("", "")
        if crop is not None:
            uri, overlay = render_issue_thumb(crop, it)
        img = (f'<div class="imgwrap"><img src="{uri}" alt="frame {i}">'
               f'{overlay}</div>') if uri else \
              '<p class="muted">（该帧无法定位到地图范围）</p>'
        kv = [f"t = {_fmt(f.get('t'), 2)} s",
              f"speed = {_fmt(f.get('speed'), 2)} m/s",
              f"lookahead = {_fmt(f.get('lookahead'), 3)} m"]
        if f.get("carrot"):
            kv.append(f"carrot = ({_fmt(f['carrot'][0], 2)}, "
                      f"{_fmt(f['carrot'][1], 2)})")
        hits = f.get("front_hits") or []
        if hits:
            kv.append(f"命中点数 = {len(hits)}")
        cards.append(
            f'<div class="thumb"><div class="imgwrap" '
            f'style="position:relative"><img src="{uri}" alt="">{overlay}</div>'
            f'<div class="meta"><span class="t" style="color:{color}">'
            f'#{i} {_esc(it["kind"])}</span> · {_esc(it["why"])}<br>'
            f'{_esc(" · ".join(kv))}</div></div>')
    return '<div class="thumbs">' + "".join(cards) + "</div>"


def _summary_kpis(run: dict, frames: Sequence[dict], g: Dict[str, Any]) -> str:
    met = run.get("metrics") or {}
    travel = met.get("travel_m")
    if travel is None:
        travel = _get(g, "extra", "sim_travel_m")
    dur = frames[-1].get("t") if frames else None
    nonzero = met.get("cmd_nonzero")
    total = met.get("cmd_count")
    kpis = [
        ("行程", f"{_fmt(travel, 2)} m"),
        ("时长", f"{_fmt(dur, 1)} s"),
        ("均速", f"{_fmt(met.get('mean_speed'), 3)} m/s"),
        ("非零命令", f"{_fmt(nonzero)} / {_fmt(total)}"),
        ("末端误差", f"{_fmt(met.get('end_error_m'), 3)} m"),
        ("贴边 RMS", f"{_fmt(met.get('edge_lateral_rms'), 4)} m"),
        ("帧数", f"{len(frames)}"),
    ]
    return '<div class="kpis">' + "".join(
        f'<div class="kpi"><div class="k">{_esc(k)}</div>'
        f'<div class="v">{_esc(v)}</div></div>' for k, v in kpis) + "</div>"


def _load_pgm_for_run(run: dict, venue: Optional[dict],
                      base_dir: str) -> Optional[PgmMap]:
    """找地图：场地档案 map_pgm（相对路径按仓库根解析）。"""
    cands: List[str] = []
    for src in (venue or {}, run.get("req") or {}):
        p = src.get("map_pgm") if isinstance(src, dict) else None
        if p:
            cands.append(str(p))
    for p in cands:
        for cand in (p, os.path.join(base_dir, p),
                     os.path.join(os.path.dirname(base_dir), p)):
            if os.path.isfile(cand):
                ref = (venue or {}).get("map_pose_ref") or {}
                origin = ref.get("origin") or {}
                ox = origin.get("x") if isinstance(origin, dict) else None
                oy = origin.get("y") if isinstance(origin, dict) else None
                res = ref.get("resolution")
                if ox is None or oy is None or not res:
                    yaml_path = os.path.splitext(cand)[0] + ".yaml"
                    try:
                        import yaml  # 已有依赖
                        with open(yaml_path, "r", encoding="utf-8") as fh:
                            y = yaml.safe_load(fh) or {}
                        o = y.get("origin") or [0, 0, 0]
                        ox, oy = float(o[0]), float(o[1])
                        res = float(y.get("resolution") or 0.05)
                    except Exception:  # noqa: BLE001
                        ox, oy, res = 0.0, 0.0, 0.05
                try:
                    return PgmMap(cand, (ox, oy), float(res))
                except (OSError, ValueError):
                    continue
    return None


def _choose_crop(m: PgmMap, run: dict, frames: Sequence[dict],
                 max_px: int = 1000) -> MapCrop:
    xs: List[float] = []
    ys: List[float] = []
    for p in (run.get("path") or []):
        if isinstance(p, (list, tuple)) and len(p) >= 2:
            xs.append(float(p[0]))
            ys.append(float(p[1]))
    for f in frames:
        xy = _frame_xy(f)
        if xy:
            xs.append(xy[0])
            ys.append(xy[1])
    for p in _get(run, "req", "traj_json", "points", default=[]) or []:
        if isinstance(p, dict) and "x" in p and "y" in p:
            xs.append(float(p["x"]))
            ys.append(float(p["y"]))
    if not xs:
        raise ValueError("无坐标可裁剪")

    pad = 6.0
    xmin, xmax = min(xs) - pad, max(xs) + pad
    ymin, ymax = min(ys) - pad, max(ys) + pad
    span_m = max(xmax - xmin, ymax - ymin, 5.0)
    step = max(1, int(math.ceil(span_m / m.res / max_px)))
    return m.crop(xmin, ymin, xmax, ymax, step=step)


def build_report(run_id: str, run: dict, venue: Optional[dict] = None,
                 base_dir: Optional[str] = None,
                 generated_at: Optional[str] = None) -> str:
    """生成单文件自包含 HTML 对比报告。

    :param run_id: run 标识（写入标题与页脚）
    :param run: run.json 反序列化后的 dict（含 frames/metrics/req/...）
    :param venue: 场地档案 dict（提供 map_pgm 与 map_pose_ref）
    :param base_dir: 解析相对路径的基准目录（默认仓库根）
    """
    base_dir = base_dir or os.path.dirname(os.path.abspath(__file__))
    base_dir = os.path.dirname(base_dir) if os.path.basename(base_dir) == "sim-bridge" else base_dir
    frames = extract_frames(run)
    g = extract_gates(run.get("record") or {})
    req = run.get("req") or {}
    issues = find_issues(frames)

    # 地图与轨迹图
    map_block = '<p class="muted">未找到场地地图（.pgm），无法渲染轨迹对比图。</p>'
    crop: Optional[MapCrop] = None
    try:
        m = _load_pgm_for_run(run, venue, base_dir)
        if m is not None and frames:
            crop = _choose_crop(m, run, frames)
            uri = crop.data_uri()
            svg = render_map_svg(crop, run, frames, issues)
            map_block = (
                f'<div class="mapwrap"><img src="{uri}" alt="map">'
                f'{svg}</div>'
                '<div class="legend">'
                '<i style="background:#2563eb"></i>规划轨迹'
                '<i style="background:#16a34a"></i>实际行驶（虚线）'
                '<i style="background:#16a34a"></i>carrot（绿十字）'
                '<i style="background:#dc2626"></i>前视命中点'
                '<i style="background:#0ea5e9;border-radius:50%;height:9px;width:9px"></i>起点'
                '<i style="background:#9333ea;border-radius:50%;height:9px;width:9px"></i>终点'
                '<br>圆圈编号 = 下方问题点位截图；地图范围按轨迹自动裁剪，'
                f'比例 {crop.m_per_px:.2f} m/px。</div>')
    except (ValueError, OSError) as e:  # noqa: BLE001
        map_block = f'<p class="muted">轨迹图渲染失败：{_esc(e)}</p>'

    # 结论
    overall = g.get("overall") or g.get("verdict")
    verdict_rows = [
        ("screening（参数是否真生效）", g.get("screening")),
        ("fidelity（8 项 F 门）", g.get("verdict")),
        ("closed_loop（闭环表现）", _get(g, "closed_loop", "verdict")),
        ("overall", overall),
    ]
    verdict_html = "".join(
        f'<tr><td>{_esc(k)}</td><td><span class="badge '
        f'{_verdict_class(v)}">{_esc(v or "未产出")}</span></td></tr>'
        for k, v in verdict_rows)

    boundary = g.get("known_boundary") or {}
    boundary_html = ""
    if boundary:
        boundary_html = (
            '<p class="muted"><b>已知边界豁免</b>：'
            + _esc(json.dumps(boundary, ensure_ascii=False)[:600]) + "</p>")

    seg = run.get("segment_info") or {}
    seg_txt = ""
    if seg:
        seg_txt = (f" · L2 分段：索引 {_esc(seg.get('from_idx'))}–"
                   f"{_esc(seg.get('to_idx'))}，"
                   f"{_esc(_fmt(seg.get('duration_s'), 1))} s")

    gen = generated_at or datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    err = run.get("error") or {}
    err_html = ""
    if err:
        err_html = (f'<div class="kpi" style="border-color:var(--fail);'
                    f'background:#fef2f2"><div class="k">错误</div>'
                    f'<div class="v" style="font-size:13px">'
                    f'{_esc(err.get("code"))}</div>'
                    f'<div class="muted">{_esc(err.get("msg"))}</div></div>')

    metrics_missing = not (g.get("checks") or g.get("values", {}).get("tracking_rms") is not None)
    warn_html = ""
    if metrics_missing:
        warn_html = ('<div class="kpi" style="border-color:var(--warn);'
                     'background:#fffbeb"><div class="k">门禁数据</div>'
                     '<div class="v" style="font-size:13px">缺失</div>'
                     '<div class="muted">该 run 未产出 compare.json'
                     '（失败/未跑完/旧档案），8 项门保持"无数据"，'
                     '不得据此判通过。</div></div>')

    train_ts = (render_timeseries(frames, "speed", "#2563eb", "速度", "m/s")
                + render_timeseries(frames, "lookahead", "#f59e0b",
                                    "lookahead（前视距离）", "m"))

    return f"""<!DOCTYPE html>
<html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>B 线回归报告 · {_esc(run_id)}</title>
<style>{CSS}</style></head><body>

<h1>B 线可视化回归验证 · 对比报告</h1>
<p class="sub">run <code>{_esc(run_id)}</code> · 生成于 {_esc(gen)} ·
数据源：run 档案 + 容器 compare.json（无手工整理）</p>

<div class="kpis">
  <div class="kpi"><div class="k">场地</div><div class="v"
    style="font-size:14px">{_esc(req.get("venue_id") or "—")}</div></div>
  <div class="kpi"><div class="k">bag</div><div class="v"
    style="font-size:13px">{_esc((req.get("bag") or "—").split("/")[-1])}</div></div>
  <div class="kpi"><div class="k">分支 / commit</div><div class="v"
    style="font-size:13px">{_esc(req.get("branch") or "—")} /
    {_esc((run.get("commit") or "")[:8] or "HEAD")}</div></div>
  <div class="kpi"><div class="k">状态</div><div class="v"
    style="font-size:14px">{_esc(run.get("state"))}</div></div>
  {err_html}{warn_html}
</div>
{_summary_kpis(run, frames, g)}

<h2>一、结论</h2>
<table style="max-width:720px"><thead><tr><th>维度</th><th>判定</th></tr>
</thead><tbody>{verdict_html}</tbody></table>
<p class="muted">判定全部来自容器侧 <code>bag_replay_diff.py</code> 的
<code>checks</code>／<code>verdict</code> 字段，报告只做呈现；标红的行
必须先在仿真侧定位，不得改口径让它变绿。</p>
{boundary_html}

<h2>二、轨迹对比</h2>
{map_block}

<h2>三、8 项 F 门指标对照</h2>
{_gates_table(g)}

<h3>闭环检查</h3>
{_closed_loop_table(g.get("closed_loop") or {})}

<h2>四、参数 diff</h2>
{_param_table(run, g)}

<h2>五、问题点位帧截图</h2>
<p class="muted">规则自动挑选（前视命中 &lt; 0.5 m / 静默停车 / 数据缺口），
每类最多 3 个代表帧且间隔 ≥ 1 s；编号与上方轨迹图圆圈对应。</p>
{_issues_section(crop, issues)}

<h2>六、时间序列</h2>
{train_ts}

<h2>七、数据溯源</h2>
<table style="max-width:820px"><tbody>
<tr><th style="width:200px">run 档案</th>
    <td><code>sim-bridge/data/runs/{_esc(run_id)}/run.json</code></td></tr>
<tr><th>对比数据</th>
    <td><code>{_esc(_get(g, "overall") and "runs/BAG_REPLAY/" + run_id + "/compare.json" or "—")}</code></td></tr>
<tr><th>参数清单</th>
    <td><code>sim-bridge/data/runs/{_esc(run_id)}/param_manifest.json</code></td></tr>
<tr><th>帧数 / 时间范围</th>
    <td>{len(frames)} 帧 · {_esc(_fmt(frames[0].get("t") if frames else None, 2))}
        – {_esc(_fmt(frames[-1].get("t") if frames else None, 2))} s{_esc(seg_txt)}</td></tr>
<tr><th>报告生成</th><td>{_esc(gen)} · 全部数值由 run 档案直读</td></tr>
<tr><th>run 消息</th><td>{_esc(run.get("msg") or "—")}</td></tr>
</tbody></table>

<h3>验收签字</h3>
<table style="max-width:820px"><tbody>
<tr><th style="width:200px">结论（现场人员手写）</th><td style="height:46px"></td></tr>
<tr><th>问题点位复核</th><td style="height:46px"></td></tr>
<tr><th>签字 / 日期</th><td style="height:46px"></td></tr>
</tbody></table>

<div class="footer">
B 线可视化回归验证 T5.1 · 单文件自包含报告（内联 SVG + base64 PNG，
无外部依赖）· 可直接「打印 → 存为 PDF」挂入验收单体系。
</div>
</body></html>
"""


def report_filename(run_id: str) -> str:
    safe = "".join(c if c.isalnum() or c in "-_." else "_" for c in run_id)
    return f"report_{safe}.html"
