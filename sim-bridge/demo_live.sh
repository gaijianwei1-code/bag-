#!/bin/bash
# 一键演示：起 bridge（常驻）→ 跑一次 run → 打印 run_id → 保持运行等 MapTool 连接
# 用法：bash demo_live.sh [duration_s]
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
DURATION="${1:-30}"
PORT=9100
TOKEN=dev-token

echo "=========================================="
echo "  B 线直播演示"
echo "=========================================="
echo
echo "[1/3] 启动 sim-bridge（后台常驻，端口 $PORT）..."
cd "$HERE"
BRIDGE_TOKEN="$TOKEN" ./.venv/bin/uvicorn app:app --host 0.0.0.0 --port "$PORT" \
    --log-level warning > /tmp/blz_bridge.log 2>&1 &
BRIDGE_PID=$!
echo "      PID=$BRIDGE_PID  日志=/tmp/blz_bridge.log"

# 等就绪
for i in $(seq 1 30); do
    if curl -sf "http://127.0.0.1:$PORT/health" -H "X-Bridge-Token: $TOKEN" > /dev/null 2>&1; then
        echo "      bridge 就绪"
        break
    fi
    sleep 1
done

echo
echo "[2/3] 提交一次 run（${DURATION}s 回放，约 $((DURATION+30))s 完成）..."
RESP=$(curl -s -X POST "http://127.0.0.1:$PORT/runs" \
    -H "X-Bridge-Token: $TOKEN" -H "Content-Type: application/json" \
    -d "{\"venue_id\":\"scenario-1793346971223\",\"bag\":\"bags/scenario-1793346971223.bag\",\"branch\":\"main\",\"traj_json\":{\"points\":[]},\"duration_s\":$DURATION}")
RUN_ID=$(echo "$RESP" | ./.venv/bin/python -c "import sys,json; print(json.load(sys.stdin).get('run_id',''))")

if [ -z "$RUN_ID" ]; then
    echo "      提交失败: $RESP"
    kill $BRIDGE_PID 2>/dev/null
    exit 1
fi
echo "      run_id = $RUN_ID"

echo
echo "[3/3] 等待 run 完成（这样 MapTool 可以按时间轴回放）..."
for i in $(seq 1 120); do
    ST=$(curl -s "http://127.0.0.1:$PORT/runs/$RUN_ID" -H "X-Bridge-Token: $TOKEN" \
         | ./.venv/bin/python -c "import sys,json; d=json.load(sys.stdin); print(d.get('state',''))" 2>/dev/null || echo "")
    if [ "$ST" = "done" ] || [ "$ST" = "failed" ]; then
        echo "      状态: $ST"
        break
    fi
    sleep 2
done

echo
echo "=========================================="
echo "  现在去 MapTool 操作"
echo "=========================================="
echo "  1. 打开地图: $HERE/../testdata/scenario_1793346971223/1793346971223.pgm"
echo "  2. 打开 yaml: $HERE/../testdata/scenario_1793346971223/1793346971223.yaml"
echo "  3. 菜单 → 直播跟车"
echo "  4. 填入:"
echo "       地址   : ws://127.0.0.1:$PORT"
echo "       run_id : $RUN_ID"
echo "       token  : $TOKEN"
echo "  5. 点「连接直播」"
echo
echo "  预期: 车图标在轨迹起点，红点/carrot 数据流动，"
echo "        状态栏显示「未达标：车未移动（虚拟雷达判满障碍）」"
echo "        （run 已跑完，会按 1x 速度回放 ~${DURATION}s）"
echo
echo "  bridge 仍在运行（PID=$BRIDGE_PID）。"
echo "  按 Ctrl+C 停止 bridge。"
echo "=========================================="

# 保持前台运行
trap "echo; echo '停止 bridge...'; kill $BRIDGE_PID 2>/dev/null; exit 0" INT TERM
wait $BRIDGE_PID
