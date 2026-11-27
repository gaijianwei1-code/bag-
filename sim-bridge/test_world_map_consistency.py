"""world 点云与 PGM 一致性裁剪（2026-11-26 najd1 卡点根因）。

跑法：./.venv/bin/python -m pytest test_world_map_consistency.py -v
容器内实跑裁剪需要 numpy；宿主机 venv 无 numpy 时自动 skip。
"""
import os

import pytest

import runner
import venues as V

VENUE_90S = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "data", "venues", "najd1_90s.yaml")


def _venue():
    v = V.load_venue(VENUE_90S)
    return v


def test_najd1_90s_enables_world_consistency():
    """场地档案必须开启一致性裁剪，否则 10.5 m 卡点会复现。"""
    cfg = _venue().get("world_map_consistency") or {}
    assert cfg.get("enabled") is True
    assert 0.0 < float(cfg.get("max_dist_m", 0)) <= 1.0


def test_disabled_when_flag_absent():
    v = _venue()
    v.pop("world_map_consistency", None)
    scene, stats = runner.prepare_world_consistent_scene(v, "ut", "/tmp/x.yaml")
    assert scene == "/tmp/x.yaml" and stats is None


def test_disabled_when_false():
    v = _venue()
    v["world_map_consistency"] = {"enabled": False}
    scene, stats = runner.prepare_world_consistent_scene(v, "ut", "/tmp/x.yaml")
    assert scene == "/tmp/x.yaml" and stats is None


def test_crop_keeps_only_points_near_map_occupied():
    """裁剪语义：只有贴近 PGM 占据格的点保留。

    宿主机无 numpy 时跳过（真正的裁剪在容器内跑）。
    """
    pytest.importorskip("numpy")
    import struct
    import tempfile

    # PGM：10x10，全自由，仅 (col=5,row=5) 一个占据格；origin=(0,0) res=1.0
    pgm = b"P5\n10 10\n255\n" + bytes(
        [0 if (r == 5 and c == 5) else 255 for r in range(10) for c in range(10)])
    # 世界坐标对应 col=x, row=9-y
    pts = [(5.5, 4.5, 0.0),   # 正落在占据格上
           (1.5, 1.5, 0.0),   # 远离障碍 → 应删除
           (5.5, 2.5, 0.0)]   # 2 m 外 → 应删除
    n = len(pts)
    hdr = ("# .PCD v0.7\nVERSION 0.7\nFIELDS x y z\nSIZE 4 4 4\nTYPE F F F\n"
           f"COUNT 1 1 1\nWIDTH {n}\nHEIGHT 1\nVIEWPOINT 0 0 0 1 0 0 0\n"
           f"POINTS {n}\nDATA binary\n")
    with tempfile.TemporaryDirectory() as d:
        pcd = os.path.join(d, "w.pcd")
        with open(pcd, "wb") as f:
            f.write(hdr.encode())
            for p in pts:
                f.write(struct.pack("<3f", *p))
        m = os.path.join(d, "m.pgm")
        with open(m, "wb") as f:
            f.write(pgm)
        stats = runner.crop_pcd_to_map_occupied(pcd, m, 0.0, 0.0, 1.0, max_dist_m=1.0)
        assert stats["points_before"] == 3
        assert stats["points"] == 1
        blob = open(pcd, "rb").read()
        i = blob.find(b"DATA binary")
        nl = blob.find(b"\n", i)
        rows = struct.unpack_from("<3f", blob, nl + 1)
        assert rows == (5.5, 4.5, 0.0)
