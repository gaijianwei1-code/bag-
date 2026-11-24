# T0.3 sim-bridge API 契约 v1

> 状态：v1 草案（2026-11-18）。实现：FastAPI（`sim-bridge/`），监听 `0.0.0.0:9100`（局域网可访问，现场 Windows ↔ Mac/工控机）。
> 鉴权：首版用固定 token（`X-Bridge-Token` 头，部署时配置）；不暴露公网。

## 1. REST 接口

| 方法/路径 | 入参 | 出参 | 说明 |
|---|---|---|---|
| `GET /health` | — | `{status, version, ros_ok}` | 存活检查（含容器可达性） |
| `GET /venues` | — | `[{venue_id, name, bag_count, map_pgm}]` | 档案列表（MapTool 场地下拉） |
| `POST /venues` | venue YAML（schema venue/v1） | `{venue_id}` 201 | 新建档案；服务端再跑一遍 C1–C6 校验 |
| `PUT /venues/{id}` | venue YAML | 200 | 编辑 |
| `GET /venues/{id}/bags` | — | `[{bag, traj, duration_s, notes}]` | 该场地可跑的 bag 列表 |
| `POST /runs` | 见 §2 | `{run_id}` 202 | 提交回归 run（异步执行） |
| `GET /runs/{id}` | — | manifest + 状态 + 指标 | 轮询用；WS 断线后补拉 |
| `GET /runs/{id}/timeline` | — | timeline JSON（T0.1） | 跑完整份下载（直播丢失帧的兜底） |
| `GET /runs/{id}/report` | `?download=1`（可选） | 单文件 HTML 对比报告（T5.1） | 默认 `text/html` 直出；`download=1` 带 `Content-Disposition: attachment`。判定值直读容器 `compare.json`，缺数据标"无数据" |
| `GET /branches` | `repo=roboneo_sim` | `[{branch, commit, date}]` | 分支下拉（T4.1） |

## 2. `POST /runs` 请求体

```json
{
  "venue_id": "nana-jingdu-01",
  "bag": "bags/nana_20261117_351s.bag",
  "branch": "develop",
  "commit": null,
  "traj_json": { "...": "MapTool 导出的轨迹 JSON 全文（透传 custom_path_node）" },
  "param_overrides": { "lookahead_dist": 1.2, "publish_odom_imu": true },
  "segment": null,
  "notes": "验证前视调小后能否进弯"
}
```

- `commit`：null = 分支 HEAD；指定则 worktree 检出该 commit（可复现）
- `segment`：null = 全程（L3）；`{"from_t": 120.5, "to_t": 150.0}` = 分段重跑（L2，从最近安全点起播）
- **不含任何 map/init_pose 字段**——一律从场地档案取（venue_schema §3 第 4 条）

## 3. WebSocket `/ws/runs/{run_id}/stream`

**下行消息**（JSON，每条带 `type`）：

| type | 频率 | 载荷 |
|---|---|---|
| `frame` | 5–10 Hz | T0.1 的单帧对象 |
| `front_hits` | 1–2 Hz | `{t, hits: [[x,y],…≤50]}` |
| `status` | 事件 | `{state: queued\|replaying\|analyzing\|done\|failed, progress: 0–1, msg}` |
| `metrics` | 1 次 | T0.1 的 metrics 对象（done 时） |
| `error` | 事件 | `{code, msg, detail}` |

**上行消息**（MapTool → bridge）：仅 `{"type":"ping"}`；60 s 无任何消息则 bridge 主动断开。

**重连约定**：MapTool 断线后指数退避重连（1s/2s/4s/…上限 30 s），重连后先 `GET /runs/{id}` 对齐状态，缺帧用 `/timeline` 补。

## 4. 错误码

| code | HTTP | 场景 |
|---|---|---|
| VENUE_NOT_FOUND | 404 | venue_id 不存在 |
| BAG_NOT_IN_VENUE | 422 | bag 未在场地档案登记 |
| INIT_POSE_MISSING | 422 | bag 起始段无有效 `/ndt/current_pose` |
| BRANCH_NOT_FOUND | 422 | 分支/commit 不存在 |
| RUN_CONFLICT | 409 | 容器已有 run 在跑（首版单并发） |
| REPLAY_FAILED | 500 | 回放异常（附容器日志尾部） |

## 5. mock 联调验收（T0.3 完成定义）

- MapTool 侧（QNetworkAccessManager/QWebSocket）与 bridge 侧各按本文档实现 mock：档案增删查、提交 run→收 frame 流→收 metrics、错误码路径（错场地/缺 init_pose）
- 双方对 §3 消息时序无歧义（顺序：status(queued) → frame* → status(analyzing) → metrics → status(done)）
