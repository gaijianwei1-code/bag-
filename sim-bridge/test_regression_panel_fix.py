#!/usr/bin/env python3
"""T4.1 回归面板 bugfix 回归：venue_id 解析必须剥离 "/bags"

历史 bug：regression_panel.cpp 里
    url.section("/venues/", 1).section("/bags", 0)
end 参数默认 -1（到末尾），导致 venue_id 变成 "nana-jingdu-01/bags"，
POST /runs 时 bridge 返回 404 "场地不存在：nana-jingdu-01/bags"，
而 Qt 把 404 统一显示为 "server replied: Not Found"，极难排查。

本测试两层验证（统一用 TestClient，不依赖局域网/额外包）：
  1) 纯逻辑：复现 QString::section 的两种写法差异
  2) 端到端：用「正确 venue_id」提交不得 404；
     用「带 /bags 后缀的 venue_id」提交必须 404（证明这就是当初的根因）
"""
import os
import sys

import pytest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

VENUE = "nana-jingdu-01"
BAG = "bags/nana_20261117_351s.bag"
TOKEN = os.environ.get("BRIDGE_TOKEN", "dev-token")
HDRS = {"X-Bridge-Token": TOKEN}


# ---------- 1. 纯逻辑：QString::section 语义回归 ----------
def _parse_vid_buggy(url: str) -> str:
    """复刻修复前的 C++ 逻辑：url.section("/venues/", 1).section("/bags", 0)"""
    tail = url.split("/venues/", 1)[1]
    # section("/bags", 0) => end 默认 -1（到末尾），即原样返回
    return tail


def _parse_vid_fixed(url: str) -> str:
    """复刻修复后的 C++ 逻辑：url.section("/venues/", 1).section("/bags", 0, 0)"""
    tail = url.split("/venues/", 1)[1]
    return tail.split("/bags", 1)[0]


def test_parse_vid_buggy_keeps_bags_suffix():
    """修复前的写法会把 /bags 留在 venue_id 里 —— 这就是 404 的根因"""
    url = f"/venues/{VENUE}/bags"
    assert _parse_vid_buggy(url) == f"{VENUE}/bags"   # 错误值，暴露 bug


def test_parse_vid_fixed_strips_bags_suffix():
    """修复后必须得到干净的 venue_id"""
    url = f"/venues/{VENUE}/bags"
    assert _parse_vid_fixed(url) == VENUE
    assert "/" not in _parse_vid_fixed(url)


@pytest.mark.parametrize("vid", ["nana-jingdu-01", "scenario-1793346971223"])
def test_parse_vid_fixed_all_real_venues(vid):
    """所有真实场地都能正确解析"""
    assert _parse_vid_fixed(f"/venues/{vid}/bags") == vid


# ---------- 2. 端到端：真实 bridge 行为 ----------
@pytest.fixture(scope="module")
def client():
    from fastapi.testclient import TestClient
    import app as A
    return TestClient(A.app)


def test_bad_venue_id_returns_404(client):
    """用带 /bags 的 venue_id 提交 → 必须 404「场地不存在」，复现历史报错"""
    body = {"venue_id": f"{VENUE}/bags", "bag": BAG, "branch": "main",
            "traj_json": {}, "duration_s": 5}
    r = client.post("/runs", json=body, headers=HDRS)
    assert r.status_code == 404, f"期望 404，实际 {r.status_code}: {r.text}"
    assert "场地不存在" in r.json().get("detail", ""), r.text


def test_good_venue_id_not_404(client):
    """修复后的干净 venue_id 不得再 404（202 正常受理，409 是单并发冲突）"""
    body = {"venue_id": VENUE, "bag": BAG, "branch": "main",
            "traj_json": {}, "duration_s": 5}
    r = client.post("/runs", json=body, headers=HDRS)
    assert r.status_code in (202, 409), f"期望 202/409，实际 {r.status_code}: {r.text}"
    if r.status_code == 202:
        assert r.json().get("run_id", "").startswith(VENUE), r.text
