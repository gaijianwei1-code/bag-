#!/usr/bin/env python3
"""W16 侦查二：对比成功 run 与失败 run 的 scene/param 配置差异。"""
import glob
import json
import os

def load_record(run_dir):
    sp = os.path.join(run_dir, "summary.json")
    rp = os.path.join(run_dir, "run_record.json")
    summary = json.load(open(sp)) if os.path.isfile(sp) else {}
    record = json.load(open(rp)) if os.path.isfile(rp) else {}
    return summary, record

print("=" * 80)
print("成功 run（车动过）的关键配置：")
for rid in ["20261104_bline_cone_offset_v1", "20261105_w0i4_a", "web_closed_20261119_083248_base"]:
    d = f"/root/catkin_ws/runs/BLINE/{rid}"
    if not os.path.isdir(d):
        print(f"  {rid}: 目录不存在"); continue
    s, r = load_record(d)
    scene = (r.get("scene") or {})
    ps = (r.get("param_set") or {})
    adapter = (r.get("adapter") or {})
    print(f"\n  [{rid}]")
    print(f"    scene_id:   {scene.get('scene_id')}")
    print(f"    world:      {scene.get('world', '')[:80]}")
    print(f"    map_dir:    {scene.get('map_dir', '')[:60]}")
    print(f"    duration:   {scene.get('duration_s')}")
    print(f"    param_set:  {ps.get('param_set_id')} changes={len(ps.get('changes', []))}")
    if ps.get("changes"):
        for c in ps["changes"][:3]:
            print(f"      {c.get('file')} {c.get('key')}: {c.get('from')} -> {c.get('to')}")
    print(f"    adapter:    {adapter.get('adapter_id')} range={adapter.get('testable_range')}")

print("\n" + "=" * 80)
print("我的 run（车不动）的关键配置：")
my_runs = sorted(glob.glob("/root/catkin_ws/runs/BLINE/scenario-1793346971223_*"))[-3:]
for d in my_runs:
    rid = os.path.basename(d)
    s, r = load_record(d)
    scene = (r.get("scene") or {})
    ps = (r.get("param_set") or {})
    adapter = (r.get("adapter") or {})
    print(f"\n  [{rid}]")
    print(f"    scene_id:   {scene.get('scene_id')}")
    print(f"    world:      {scene.get('world', '')[:80]}")
    print(f"    map_dir:    {scene.get('map_dir', '')[:60]}")
    print(f"    duration:   {scene.get('duration_s')}")
    print(f"    param_set:  {ps.get('param_set_id')} changes={len(ps.get('changes', []))}")
    if ps.get("changes"):
        for c in ps["changes"][:3]:
            print(f"      {c.get('file')} {c.get('key')}: {c.get('from')} -> {c.get('to')}")
    print(f"    adapter:    {adapter.get('adapter_id')} range={adapter.get('testable_range')}")

print("\n" + "=" * 80)
print("对照结论将由读图者判断。")
