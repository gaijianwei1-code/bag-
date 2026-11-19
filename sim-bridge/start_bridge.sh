#!/bin/bash
# 一键启动 bridge（MapTool 回归面板依赖它）
# 用法：bash start_bridge.sh
HERE="$(cd "$(dirname "$0")" && pwd)"
PORT=9100
TOKEN=dev-token

# 检查是否已在运行
if curl -sf "http://127.0.0.1:$PORT/health" -H "X-Bridge-Token: $TOKEN" > /dev/null 2>&1; then
    echo "bridge 已在运行（端口 $PORT）"
    exit 0
fi

echo "启动 sim-bridge（端口 $PORT）..."
cd "$HERE"
BRIDGE_TOKEN="$TOKEN" ./.venv/bin/uvicorn app:app --host 0.0.0.0 --port "$PORT" \
    --log-level warning > /tmp/blz_bridge.log 2>&1 &
PID=$!

for i in $(seq 1 15); do
    if curl -sf "http://127.0.0.1:$PORT/health" -H "X-Bridge-Token: $TOKEN" > /dev/null 2>&1; then
        echo "✅ bridge 就绪（PID=$PID，日志=/tmp/blz_bridge.log）"
        echo "   MapTool 回归面板 / 直播跟车可正常使用了"
        exit 0
    fi
    sleep 1
done

echo "❌ bridge 启动超时，查看日志：cat /tmp/blz_bridge.log"
exit 1
