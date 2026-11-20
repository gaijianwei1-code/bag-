# B 线可视化回归验证 — 开发约定

## 🔴 每次改完代码必须同步到 GitHub

**远端**：<https://github.com/gaijianwei1-code/bag->（`origin`, 分支 `main`, SSH 免密）

**一句话规则**：任何代码改动（修复 / 新功能 / 测试 / 文档）完成后，**必须**执行同步，
不要等用户提醒，也不要留着本地未推的提交。

```bash
cd /Users/a11/地图工具bag回放 && bash sync.sh "类型: 做了什么"
```

脚本行为：`add -A` → `commit` → `push origin main` → 校验远端哈希一致。
工作区干净时只检查推送状态，不产生空提交。

### 提交信息格式

`<类型>: <做了什么>`

| 类型 | 用于 |
|---|---|
| `fix:` | 修 bug（写清根因，如 `修 venue_id 解析少截断 /bags`） |
| `feat:` | 新功能（带任务号，如 `T4.2 分段重跑`） |
| `test:` | 只加/改测试 |
| `docs:` | 只改文档 |
| `chore:` | 构建、依赖、清理 |

一次改动涉及多处时，在正文里分行说明根因，便于以后回溯。

## 验证门槛（推送前）

推送前至少跑过对应测试，**不允许把未验证的代码推上去**：

| 改动范围 | 最低验证 |
|---|---|
| C++ / Qt 代码 | `bash docs/smoke/run_smoke.sh`（7 项全绿） |
| sim-bridge Python | `cd sim-bridge && ./.venv/bin/python -m pytest -q`（59 用例） |
| WS / 直播链路 | 冒烟 5.6 `test_live_ws` 必须 PASS |
| 涉及真实跑 run | 另外跑 `./.venv/bin/python test_e2e_full.py`（40 项） |

## 环境速查

```bash
# Qt6 工具链
export PATH="$HOME/Documents/Codex/2026-10-12/n-h/work/toolchain/env/bin:$PATH"
export CMAKE_PREFIX_PATH="$HOME/Documents/Codex/2026-10-12/n-h/work/toolchain/env/lib/cmake/Qt6"

# 构建
cmake -S maptool-sim/mac-tool-src -B build && cmake --build build -j4
# 产物：build/dist/bin/MapTool.app

# bridge
bash sim-bridge/start_bridge.sh
# http://127.0.0.1:9100, header X-Bridge-Token: dev-token
```

## 已知坑位（踩过就记下来）

1. **bridge 单并发**：有 run 在跑时 `POST /runs` 返回 409。测试前先
   `docker exec roboneo-sim pkill -f 'bag_replay|b_line_run'` 并重启 bridge。
2. **Qt 把 4xx 统一显示为** `server replied: Not Found`，真实原因在响应体 `detail`。
   面板代码已解析并展示，排查时优先看界面红字而非 Qt 的 errorString。
3. **不可跑的场地**：`/venues` 返回 `runnable` 字段；缺 `container:` 段的 YAML
   （如 T2.1 的 CRUD 样例 nana）会把 run 打成 failed，面板已灰显+拦截。
4. **`test_e2e_full.py` 是过程式脚本**，已在 `conftest.py` 的 `collect_ignore` 中排除；
   它必须单独用 `python test_e2e_full.py` 运行，不能被 pytest 收集（否则抛 SystemExit）。
5. **进程别用 `&` 启动常驻服务**（shell 退出即死），用 `nohup` 或 `start_bridge.sh`。
