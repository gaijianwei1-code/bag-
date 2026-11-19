#!/usr/bin/env python3
"""W16 侦查一：历史 BLINE run 的运动数据普查。
回答一个关键问题：BLINE 这条链路上，车有没有动过？
"""
import glob
import json
import os

print(f"{'run_id':<50} {'travel_m':>10} {'nonzero':>8} {'peak':>6} {'end_err':>9}")
print("-" * 90)
rows = []
for d in sorted(glob.glob("/root/catkin_ws/runs/BLINE/*/")):
    rid = os.path.basename(d.rstrip("/"))
    sp = os.path.join(d, "summary.json")
    if not os.path.isfile(sp):
        print(f"{rid:<50} {'(无 summary)':>10}")
        continue
    try:
        s = json.load(open(sp))
    except Exception as e:
        print(f"{rid:<50} (解析失败: {e})")
        continue
    m = s.get("motion") or {}
    c = s.get("commands") or {}
    travel = m.get("travel_m")
    rows.append((rid, travel))
    print(f"{rid:<50} {str(travel):>10} {str(c.get('nonzero')):>8} "
          f"{str(m.get('progress_peak')):>6} {str(m.get('end_error_m')):>9}")

moved = [r for r in rows if r[1] and float(r[1]) > 0.1]
print(f"\n总计 {len(rows)} 个 run，其中车移动过(travel>0.1m)的: {len(moved)} 个")
if moved:
    for r in moved:
        print("  ✓", r[0], r[1])
else:
    print("  ✗ 没有任何 run 的车动过 —— 说明 BLINE 链路下 planner 从未成功驱动过车")
