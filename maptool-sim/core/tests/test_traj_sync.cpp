#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <cmath>
#include <cstdio>

#include "traj_sync_dialog.h"

namespace {

int g_fail = 0;

void check(bool cond, const char* msg)
{
    if (!cond) {
        std::fprintf(stderr, "FAIL: %s\n", msg);
        ++g_fail;
    } else {
        std::printf("ok: %s\n", msg);
    }
}

QJsonObject makePoint(double x, double y, int roadId, int bypass, int extra = -1)
{
    QJsonObject o;
    o["x"] = x;
    o["y"] = y;
    o["yaw"] = 0.5;
    o["pixel_x"] = x * 20.0;
    o["pixel_y"] = 5000.0 - y * 20.0;
    o["road_id"] = roadId;
    o["bypass"] = bypass;
    if (extra >= 0) {
        o["force_pass"] = extra;
    }
    return o;
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // 构造源轨迹 (20 点)
    QJsonArray srcPts;
    for (int i = 0; i < 20; ++i) {
        srcPts.append(makePoint(i * 1.0, (i >= 5 && i <= 12) ? 10.0 : 0.0, 2, 1, (i == 8 ? 1 : -1)));
    }

    // 构造目标轨迹 A: 偏移 +10 点且中间少 2 点 (验证 AutoStartFixedLen 与 AutoStartEnd)
    QJsonArray dstPts;
    for (int i = 0; i < 10; ++i) {
        dstPts.append(makePoint(-10.0 + i * 1.0, 0.0, 1, 0));
    }
    // 对应 src[0..4]
    for (int i = 0; i < 5; ++i) {
        dstPts.append(makePoint(i * 1.0, 0.0, 1, 0));
    }
    // 对应 src[5..12] (8 点) 在目标里只有 6 点且 y=2.0
    for (int i = 0; i < 6; ++i) {
        dstPts.append(makePoint(5.0 + i * (7.0 / 5.0), 2.0, 1, 0));
    }
    // 对应 src[13..19]
    for (int i = 13; i < 20; ++i) {
        dstPts.append(makePoint(i * 1.0, 0.0, 1, 0));
    }

    QJsonObject dstRoot;
    dstRoot["points"] = dstPts;
    dstRoot["post_process"] = 0;

    // 1. AutoStartFixedLen: src[5..12] (8 点) -> 自动匹配起点 dst[15] (x=5.0)
    {
        auto plan = blz_sync::computeSyncPlan(srcPts, 5, 12, dstPts, blz_sync::AlignMode::AutoStartFixedLen);
        check(plan.valid, "AutoStartFixedLen valid");
        check(plan.dstStart == 15 && plan.dstEnd == 22, "AutoStartFixedLen matched dst[15..22]");
        QJsonObject out;
        QString err;
        check(blz_sync::applySyncPlan(srcPts, 5, 12, dstRoot, plan, true, true, out, err),
              "AutoStartFixedLen applySyncPlan ok");
        check(out["points"].toArray().size() == dstPts.size(), "AutoStartFixedLen keeps total count");
    }

    // 2. AutoStartEnd: src[5..12] (8 点) -> 匹配起点 dst[15] 和终点 dst[20] (6 点)，无缝接入 dst[21]
    {
        auto plan = blz_sync::computeSyncPlan(srcPts, 5, 12, dstPts, blz_sync::AlignMode::AutoStartEnd);
        check(plan.valid, "AutoStartEnd valid");
        check(plan.dstStart == 15 && plan.dstEnd == 20, "AutoStartEnd matched dst[15..20]");
        check(std::abs(plan.joinNextDist - std::hypot(1.0, 10.0)) < 1e-6, "AutoStartEnd joinNextDist == hypot(1,10)");
        QJsonObject out;
        QString err;
        check(blz_sync::applySyncPlan(srcPts, 5, 12, dstRoot, plan, true, true, out, err),
              "AutoStartEnd applySyncPlan ok");
        const QJsonArray outPts = out["points"].toArray();
        check(outPts.size() == dstPts.size() + 2, "AutoStartEnd updates total count (+2)");
        check(outPts[18].toObject()["force_pass"].toInt() == 1, "AutoStartEnd preserved optional force_pass");
        check(!outPts[17].toObject().contains("force_pass"), "AutoStartEnd did not fabricate force_pass");
    }

    // 3. 仅同步位置（保留目标属性）与仅同步属性（保留目标位置）
    {
        auto plan = blz_sync::computeSyncPlan(srcPts, 5, 12, dstPts, blz_sync::AlignMode::ManualStartFixed, 15);
        check(plan.valid, "ManualStartFixed valid");
        QJsonObject outPosOnly, outAttrOnly;
        QString err;
        check(blz_sync::applySyncPlan(srcPts, 5, 12, dstRoot, plan, true, false, outPosOnly, err),
              "pos-only apply ok");
        const QJsonObject pPos = outPosOnly["points"].toArray()[18].toObject();
        check(std::abs(pPos["y"].toDouble() - 10.0) < 1e-6, "pos-only updated y to 10.0");
        check(pPos["road_id"].toInt() == 1, "pos-only preserved target road_id=1");

        check(blz_sync::applySyncPlan(srcPts, 5, 12, dstRoot, plan, false, true, outAttrOnly, err),
              "attr-only apply ok");
        const QJsonObject pAttr = outAttrOnly["points"].toArray()[18].toObject();
        check(std::abs(pAttr["y"].toDouble() - 2.0) < 1e-6, "attr-only preserved target y=2.0");
        check(pAttr["road_id"].toInt() == 2 && pAttr["force_pass"].toInt() == 1,
              "attr-only copied src road_id=2 and force_pass=1");
    }

    // 4. 仅同步位置时，不同长度的 AutoStartEnd 应被拦截
    {
        auto plan = blz_sync::computeSyncPlan(srcPts, 5, 12, dstPts, blz_sync::AlignMode::AutoStartEnd);
        QJsonObject out;
        QString err;
        check(!blz_sync::applySyncPlan(srcPts, 5, 12, dstRoot, plan, true, false, out, err),
              "pos-only rejects unequal segment length");
    }

    // 5. 文件保存 + .bak 备份往返测试
    {
        QTemporaryDir tmp;
        check(tmp.isValid(), "temp dir valid");
        const QString fpath = tmp.filePath("traj.json");
        QString err;
        check(blz_sync::saveTrajectoryJson(fpath, dstRoot, false, err), "initial save ok");
        auto plan = blz_sync::computeSyncPlan(srcPts, 5, 12, dstPts, blz_sync::AlignMode::AutoStartEnd);
        QJsonObject out;
        check(blz_sync::applySyncPlan(srcPts, 5, 12, dstRoot, plan, true, true, out, err), "apply ok");
        check(blz_sync::saveTrajectoryJson(fpath, out, true, err), "save with backup ok");
        check(QFile::exists(fpath + ".bak"), ".bak created");
        QJsonObject reloaded, reloadedBak;
        check(blz_sync::loadTrajectoryJson(fpath, reloaded, err), "reload saved json ok");
        check(blz_sync::loadTrajectoryJson(fpath + ".bak", reloadedBak, err), "reload .bak json ok");
        check(reloaded["points"].toArray().size() == dstPts.size() + 2, "saved json has updated count");
        check(reloadedBak["points"].toArray().size() == dstPts.size(), ".bak retains original count");
    }

    return g_fail == 0 ? 0 : 1;
}
