"""场地档案（venue registry）加载/校验/存储。
Schema 见 docs/venue_schema.md；校验规则 C1-C6 在 validate_venue()。
"""
import os
import re
import yaml

VENUE_SCHEMA = "venue/v1"
_ID_RE = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")


class VenueError(Exception):
    """校验失败；str(e) 为用户可读文案。"""


def validate_venue(data: dict, venues_dir: str, check_files: bool = True) -> None:
    """按 C1-C6 校验。文件存在性校验可关（单测用）。"""
    if data.get("schema") != VENUE_SCHEMA:
        raise VenueError(f"schema 必须为 {VENUE_SCHEMA}")
    vid = data.get("venue_id", "")
    if not _ID_RE.match(vid or ""):
        raise VenueError(f"场地 ID 不合法：{vid!r}（小写字母数字与连字符）")

    def rel(key):
        v = data.get(key)
        if not v or not isinstance(v, str):
            raise VenueError(f"缺少 {key}")
        if v.startswith("/") or ".." in v:
            raise VenueError(f"{key} 必须是相对路径：{v}")
        return os.path.join(venues_dir, v)

    if check_files:
        for key in ("map_pgm", "map_pcd"):
            if not os.path.isfile(rel(key)):
                raise VenueError(f"文件不存在：{data[key]}")
        for b in data.get("bags", []):
            for key in ("bag", "traj"):
                p = os.path.join(venues_dir, b.get(key, ""))
                if not os.path.isfile(p):
                    raise VenueError(f"文件不存在：{b.get(key)}")

    # C3: pcd 与 pgm 坐标范围匹配——用 map_pose_ref 换算 pgm 尺寸后比包围盒
    ref = data.get("map_pose_ref") or {}
    res = ref.get("resolution")
    if not res or not (0 < res <= 0.2):
        raise VenueError("map_pose_ref.resolution 超出合理范围 (0, 0.2]")
    if "origin" not in ref:
        raise VenueError("缺少 map_pose_ref.origin")

    hf = data.get("pcd_height_filter") or {}
    if hf and (hf.get("min", 0) < 0 or hf.get("max", 99) <= hf.get("min", 0)):
        raise VenueError("pcd_height_filter 非法（min>=0 且 max>min）")

    for i, b in enumerate(data.get("bags", [])):
        if not b.get("bag") or not b.get("traj"):
            raise VenueError(f"bags[{i}] 缺 bag 或 traj")


def load_venue(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def save_venue(path: str, data: dict) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        yaml.safe_dump(data, f, allow_unicode=True, sort_keys=False)


def list_venues(venues_dir: str) -> list[dict]:
    out = []
    if os.path.isdir(venues_dir):
        for fn in sorted(os.listdir(venues_dir)):
            if fn.endswith(".yaml"):
                try:
                    out.append(load_venue(os.path.join(venues_dir, fn)))
                except yaml.YAMLError:
                    pass  # 损坏档案跳过，不拖垮列表
    return out


def bag_in_venue(data: dict, bag_rel: str) -> bool:
    return any(b.get("bag") == bag_rel for b in data.get("bags", []))
