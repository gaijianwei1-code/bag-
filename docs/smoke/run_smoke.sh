#!/bin/bash
# 标准冒烟脚本：一条命令跑完全部测试（标准样本：scenario_1793346971223）
# 用法：bash docs/smoke/run_smoke.sh
set -o pipefail

ROOT="/Users/a11/地图工具bag回放"
SAMPLE="$ROOT/testdata/scenario_1793346971223"
PCD="/Users/a11/Downloads/scenario_1793346971223/1793346971223_frames_no_ground.pcd"
TOOLCHAIN="$HOME/Documents/Codex/2026-10-12/n-h/work/toolchain/env"
TC="$TOOLCHAIN/lib/cmake/Qt6"

export PATH="$TOOLCHAIN/bin:$PATH"
export QT_QPA_PLATFORM=offscreen

FAIL=0
step() { printf "\n\033[1m== %s ==\033[0m\n" "$1"; }
ok()   { printf "  \033[32m✓ %s\033[0m\n" "$1"; }
bad()  { printf "  \033[31m✗ %s\033[0m\n" "$1"; FAIL=$((FAIL+1)); }

# ---------- 1. 主构建 ----------
step "1/8 主构建（MapTool.app）"
cd "$ROOT" || exit 1
if cmake -S maptool-sim/mac-tool-src -B build -DCMAKE_PREFIX_PATH="$TC" > /dev/null 2>&1 \
   && cmake --build build -j4 > /tmp/blz_build.log 2>&1; then
    ok "MapTool.app 构建通过"
else
    bad "构建失败，见 /tmp/blz_build.log"; tail -5 /tmp/blz_build.log
fi

# ---------- 2. 冒烟工程构建 ----------
step "2/8 冒烟工程构建（core + UI 集成 + 前视回归）"
SMOKE_SRC=/tmp/blz_smoke_src
rm -rf "$SMOKE_SRC" && mkdir -p "$SMOKE_SRC"
cp "$ROOT/maptool-sim/CMakeLists_smoke.txt" "$SMOKE_SRC/CMakeLists.txt"
cp -R "$ROOT/maptool-sim/core" "$SMOKE_SRC/"
mkdir -p "$SMOKE_SRC/mac-tool-src"
cp -R "$ROOT/maptool-sim/mac-tool-src/mainwindow" \
      "$ROOT/maptool-sim/mac-tool-src/items" \
      "$ROOT/maptool-sim/mac-tool-src/dialog" "$SMOKE_SRC/mac-tool-src/"
if cmake -S "$SMOKE_SRC" -B "$SMOKE_SRC/out" -DCMAKE_PREFIX_PATH="$TC" > /dev/null 2>&1 \
   && cmake --build "$SMOKE_SRC/out" -j4 > /tmp/blz_smoke_build.log 2>&1; then
    ok "三个冒烟目标编译通过"
else
    bad "冒烟工程编译失败"; grep -m5 "error" /tmp/blz_smoke_build.log
fi

# ---------- 3. core 单元 ----------
step "3/8 test_core（投影 + raycast 18 断言）"
if "$SMOKE_SRC/out/test_core" > /tmp/blz_t1.log 2>&1; then
    grep -E "bench:" /tmp/blz_t1.log | sed 's/^/  /'
    ok "test_core PASS"
else
    bad "test_core 失败"; tail -5 /tmp/blz_t1.log
fi

# ---------- 4. UI 集成（标准样本） ----------
step "4/8 test_ui_integration（标准样本 pgm+json）"
if [ -f "$SAMPLE/1793346971223.pgm" ]; then
    if "$SMOKE_SRC/out/test_ui_integration" "$SAMPLE/1793346971223.pgm" "$SAMPLE/1793347176294.json" > /tmp/blz_t2.log 2>&1; then
        ok "test_ui_integration PASS"
    else
        bad "test_ui_integration 失败"; grep -E "FAIL" /tmp/blz_t2.log | head -5
    fi
else
    bad "标准样本缺失：$SAMPLE/1793346971223.pgm"
fi


# ---------- 4.4 前视检测区几何口径（车身不参与 / 裕度默认0）----------
step "4.4/8 test_fov_geometry（车身侧墙不报警 / 裕度策略 / 分档生效）"
if "$SMOKE_SRC/out/test_fov_geometry" > /tmp/blz_t_geo.log 2>&1; then
    tail -1 /tmp/blz_t_geo.log | sed 's/^/  /'
    ok "test_fov_geometry PASS"
else
    bad "test_fov_geometry 失败"; grep -E "^FAIL" /tmp/blz_t_geo.log | head -5
fi

# ---------- 4.5 T3.4 前视距离与 planner 同口径 ----------
step "4.5/8 test_planner_limits（0.4×collision_time 四场景换算）"
if "$SMOKE_SRC/out/test_planner_limits" > /tmp/blz_t_pl.log 2>&1; then
    tail -1 /tmp/blz_t_pl.log | sed 's/^/  /'
    ok "test_planner_limits PASS"
else
    bad "test_planner_limits 失败"; grep -E "^FAIL" /tmp/blz_t_pl.log | head -5
fi

step "4.6/8 test_fov_planner_mode（FovLayer 按点属性切距离）"
if "$SMOKE_SRC/out/test_fov_planner_mode" > /tmp/blz_t_pm.log 2>&1; then
    tail -1 /tmp/blz_t_pm.log | sed 's/^/  /'
    ok "test_fov_planner_mode PASS"
else
    bad "test_fov_planner_mode 失败"; grep -E "^FAIL" /tmp/blz_t_pm.log | head -5
fi

# ---------- 5. 前视回归（真实点云） ----------
step "5/8 test_fov_regression（真实 pcd + 场景 clear 悬垂/z 值回归）"
if [ -f "$PCD" ]; then
    if "$SMOKE_SRC/out/test_fov_regression" "$PCD" > /tmp/blz_t3.log 2>&1; then
        ok "test_fov_regression PASS"
    else
        bad "test_fov_regression 失败（退出码 $?）"; grep -E "^FAIL" /tmp/blz_t3.log | head -5
    fi
else
    bad "点云缺失（从 Downloads 取）：$PCD"
fi

# ---------- 5.5 直播跟车冒烟 ----------
step "5.5/8 test_live_layer（坐标换算/场景 clear 重建/z 分层）"
if "$SMOKE_SRC/out/test_live_layer" > /tmp/blz_t4.log 2>&1; then
    ok "test_live_layer PASS"
else
    bad "test_live_layer 失败"; grep -E "^FAIL" /tmp/blz_t4.log | head -5
fi

# ---------- 5.6 直播跟车真实 WS 连接（补测「点连接直播没响应」）----------
step "5.6/8 test_live_ws（真实 WS 握手 + 出帧 + 状态机）"
RUN_ID=""
if curl -sf "http://127.0.0.1:9100/health" -H "X-Bridge-Token: dev-token" > /dev/null 2>&1; then
    RUN_ID=$(curl -s -X POST "http://127.0.0.1:9100/runs" \
        -H "X-Bridge-Token: dev-token" -H "Content-Type: application/json" \
        -d '{"venue_id":"scenario-1793346971223","bag":"bags/scenario-1793346971223.bag","branch":"main","traj_json":{},"duration_s":30}' \
        2>/dev/null | sed -n 's/.*"run_id":"\([^"]*\)".*/\1/p')
fi
if [ -z "$RUN_ID" ]; then
    ok "跳过 test_live_ws（bridge 未运行或无可用 run）"
elif "$SMOKE_SRC/out/test_live_ws" ws://127.0.0.1:9100 "$RUN_ID" dev-token > /tmp/blz_t5.log 2>&1; then
    grep -c "^ok:" /tmp/blz_t5.log | sed 's/^/  断言通过 /'
    ok "test_live_ws PASS"
else
    bad "test_live_ws 失败"; grep -E "^FAIL" /tmp/blz_t5.log | head -5
fi

# ---------- 6. bridge pytest ----------
step "7/8 sim-bridge pytest（43 用例：T2.1 接口/T2.2 runner/T2.3 直播；docker 用例默认跳过）"
cd "$ROOT/sim-bridge" || exit 1
if [ -x .venv/bin/python ]; then
    if ./.venv/bin/python -m pytest -q > /tmp/blz_pytest.log 2>&1; then
        tail -1 /tmp/blz_pytest.log | sed 's/^/  /'
        ok "bridge pytest PASS"
    else
        bad "bridge pytest 失败"; tail -8 /tmp/blz_pytest.log
    fi
else
    bad "venv 缺失：先 cd sim-bridge && python3 -m venv .venv && .venv/bin/pip install -r requirements.txt"
fi

# ---------- 汇总 ----------
printf "\n\033[1m================ 冒烟汇总 ================\033[0m\n"
if [ "$FAIL" -eq 0 ]; then
    printf "\033[32m全部通过（0 失败）\033[0m\n"
    exit 0
else
    printf "\033[31m%d 项失败\033[0m\n" "$FAIL"
    exit 1
fi
