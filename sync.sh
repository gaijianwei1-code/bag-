#!/bin/bash
# 一键提交并同步到 GitHub
#
# 用法：
#   bash sync.sh                  # 自动生成提交信息
#   bash sync.sh "修复 xxx"       # 指定提交信息
#
# 行为：
#   1. 检查工作区是否有改动；无改动则跳过提交，但仍尝试推送未推的提交
#   2. git add -A + commit（有信息用信息，没有则自动摘要最近改动）
#   3. git push 到 origin/main
#   4. 校验远端与本地一致
set -o pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$HERE" || exit 1

BRANCH="$(git branch --show-current)"
MSG="$1"

echo "=== 仓库：$HERE（分支 $BRANCH）==="

# ---- 1. 有无改动 ----
if [ -n "$(git status --porcelain)" ]; then
    CHANGED=$(git status --porcelain | wc -l | tr -d ' ')
    echo "检测到 $CHANGED 处改动："
    git status --short | head -15
    [ "$CHANGED" -gt 15 ] && echo "  …（共 $CHANGED 处）"

    if [ -z "$MSG" ]; then
        # 自动生成：用改动最多的顶层目录 + 文件清单摘要
        DIRS=$(git status --porcelain | awk '{print $2}' | cut -d/ -f1 | sort -u | tr '\n' ' ')
        MSG="sync: 更新 ${DIRS}（$CHANGED 个文件）"
    fi

    git add -A
    git commit -q -m "$MSG" || { echo "❌ 提交失败"; exit 1; }
    echo "✅ 已提交：$(git log --format='%h %s' -1)"
else
    echo "工作区无改动"
    if [ -n "$MSG" ]; then
        echo "（忽略提交信息：没有改动可提交）"
    fi
fi

# ---- 2. 推送 ----
if ! git remote get-url origin > /dev/null 2>&1; then
    echo "❌ 未配置 origin 远端"
    exit 1
fi

LOCAL=$(git rev-parse HEAD)
git fetch origin -q 2>/dev/null
REMOTE=$(git rev-parse "origin/$BRANCH" 2>/dev/null || echo "none")

if [ "$LOCAL" = "$REMOTE" ]; then
    echo "✅ 远端已是最新（$LOCAL），无需推送"
    exit 0
fi

echo "推送中…"
if git push origin "$BRANCH" 2>&1 | tail -5; then
    AFTER=$(git rev-parse "origin/$BRANCH" 2>/dev/null || echo "none")
    if [ "$LOCAL" = "$AFTER" ]; then
        echo "✅ 同步完成：origin/$BRANCH = ${LOCAL:0:7}"
        echo "   https://github.com/gaijianwei1-code/bag-"
        exit 0
    fi
    echo "⚠️  推送返回成功但远端哈希不一致，请检查"
    exit 1
else
    echo "❌ 推送失败（可能是远端有新提交，先 git pull --rebase）"
    exit 1
fi
