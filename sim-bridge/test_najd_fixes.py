"""najd1 修复：临时轨迹落 map_dir、3 字段 PCD 自动补 intensity。（零 numpy 依赖）"""
import os
import struct

import pytest

import runner


def _write_pcd(path, fields, pts):
    n = len(pts)
    k = len(fields)
    hdr = ("# .PCD v0.7\nVERSION 0.7\nFIELDS " + " ".join(fields) + "\n"
           "SIZE " + " ".join(["4"] * k) + "\nTYPE " + " ".join(["F"] * k) + "\n"
           "COUNT " + " ".join(["1"] * k) + f"\nWIDTH {n}\nHEIGHT 1\n"
           f"VIEWPOINT 0 0 0 1 0 0 0\nPOINTS {n}\nDATA binary\n")
    with open(path, "wb") as f:
        f.write(hdr.encode())
        for p in pts:
            f.write(struct.pack("<%df" % k, *p))


def _read_pcd(path):
    blob = open(path, "rb").read()
    idx = blob.find(b"DATA binary")
    nl = blob.find(b"\n", idx)
    meta = {}
    for line in blob[:nl].decode().splitlines():
        p = line.split()
        if p:
            meta[p[0]] = p[1:]
    k = len(meta["FIELDS"])
    n = int(meta["POINTS"][0])
    body = blob[nl + 1:nl + 1 + n * 4 * k]
    rows = [struct.unpack_from("<%df" % k, body, i * 4 * k) for i in range(n)]
    return meta, rows


def test_pcd_xyz_to_xyzi(tmp_path):
    p = str(tmp_path / "w.pcd")
    pts = [(1.0, 2.0, 3.0), (-4.5, 0.25, 0.5), (0.0, 0.0, 0.0)]
    _write_pcd(p, ["x", "y", "z"], pts)
    r = runner.convert_pcd_xyz_to_xyzi_file(p)
    assert r["converted"] is True and r["downsampled"] is False and r["points"] == 3
    meta, rows = _read_pcd(p)
    assert meta["FIELDS"] == ["x", "y", "z", "intensity"]
    assert meta["SIZE"] == ["4"] * 4 and meta["POINTS"] == ["3"]
    for row, o in zip(rows, pts):
        assert row[:3] == o and row[3] == 0.0
    # 幂等
    r2 = runner.convert_pcd_xyz_to_xyzi_file(p)
    assert r2["converted"] is False and r2["downsampled"] is False and r2["points"] == 3


def test_pcd_downsample_when_over_limit(tmp_path):
    p = str(tmp_path / "big.pcd")
    pts = [(float(i) * 0.01, 0.0, 0.0, 1.0) for i in range(40)]
    _write_pcd(p, ["x", "y", "z", "intensity"], pts)
    r = runner.convert_pcd_xyz_to_xyzi_file(p, max_points=10, voxel_size=0.10)
    assert r["downsampled"] is True and r["points"] <= 10
    meta, rows = _read_pcd(p)
    assert int(meta["POINTS"][0]) == r["points"] == len(rows)


def test_najd1_uses_small_tolerance_param_set():
    """najd1 是 4134 点长轨迹，但分段回归实际只跑到前 95 点，
    必须用 1.0 m 容差；baseline_branchB 的 6 m 会让 goal 跳过整段入口。"""
    import venues as V
    v = V.load_venue(os.path.join(os.path.dirname(__file__), "data", "venues", "najd1.yaml"))
    assert v.get("default_param_set") == "t31_tol_1p0"


def test_najd1_90s_venue_points_at_short_bag():
    import venues as V
    v = V.load_venue(os.path.join(os.path.dirname(__file__), "data", "venues", "najd1_90s.yaml"))
    assert v["container"]["bag"] == "/root/bags/najd1/planner_90s.bag"
    assert v["bags"][0]["duration_s"] == 89


def test_pcd_already_xyzi_untouched(tmp_path):
    p = str(tmp_path / "w4.pcd")
    _write_pcd(p, ["x", "y", "z", "intensity"], [(1, 2, 3, 9.0)])
    before = open(p, "rb").read()
    assert runner.convert_pcd_xyz_to_xyzi_file(p)["converted"] is False
    assert open(p, "rb").read() == before


def test_pcd_unsupported_fields_raises(tmp_path):
    p = str(tmp_path / "w5.pcd")
    _write_pcd(p, ["x", "y", "z", "rgb", "foo"], [(1, 2, 3, 4, 5)])
    with pytest.raises(runner.RunError) as ei:
        runner.convert_pcd_xyz_to_xyzi_file(p)
    assert ei.value.code == "PCD_FIELDS_UNSUPPORTED"


def test_segment_traj_prefers_map_dir_and_cleans_up():
    src = open(runner.__file__, encoding="utf-8").read()
    assert "os.path.join(map_dir, 'blz_seg_traj_' + run_id + '.json')" in src
    assert "rm -f" in src and "blz_seg_scene_" in src   # finally 清理
