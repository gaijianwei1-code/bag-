#include "traj_sync_dialog.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <limits>

namespace blz_sync {

namespace {

constexpr const char* kPosKeys[] = {"x", "y", "yaw", "pixel_x", "pixel_y"};

bool isPositionKey(const QString& key)
{
    for (const char* pk : kPosKeys) {
        if (key == QLatin1String(pk)) return true;
    }
    return false;
}

bool pointXY(const QJsonArray& pts, int idx, double& xOut, double& yOut)
{
    if (idx < 0 || idx >= pts.size()) return false;
    const QJsonObject obj = pts.at(idx).toObject();
    if (!obj.contains(QStringLiteral("x")) || !obj.contains(QStringLiteral("y")))
        return false;
    xOut = obj.value(QStringLiteral("x")).toDouble();
    yOut = obj.value(QStringLiteral("y")).toDouble();
    return std::isfinite(xOut) && std::isfinite(yOut);
}

double pointDist(const QJsonArray& a, int ia, const QJsonArray& b, int ib)
{
    double ax = 0, ay = 0, bx = 0, by = 0;
    if (!pointXY(a, ia, ax, ay) || !pointXY(b, ib, bx, by)) return 0.0;
    return std::hypot(ax - bx, ay - by);
}

} // namespace

bool loadTrajectoryJson(const QString& path, QJsonObject& rootOut, QString& errOut)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        errOut = QObject::tr("无法打开文件：%1").arg(f.errorString());
        return false;
    }
    const QByteArray raw = f.readAll();
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        errOut = QObject::tr("JSON 解析失败：%1").arg(pe.errorString());
        return false;
    }
    const QJsonObject obj = doc.object();
    const QJsonValue ptsVal = obj.value(QStringLiteral("points"));
    if (!ptsVal.isArray()) {
        errOut = QObject::tr("缺少有效的 points 数组");
        return false;
    }
    if (ptsVal.toArray().isEmpty()) {
        errOut = QObject::tr("points 数组为空");
        return false;
    }
    rootOut = obj;
    return true;
}

bool saveTrajectoryJson(const QString& path, const QJsonObject& root,
                        bool makeBackup, QString& errOut)
{
    if (makeBackup && QFile::exists(path)) {
        const QString bakPath = path + QStringLiteral(".bak");
        if (QFile::exists(bakPath) && !QFile::remove(bakPath)) {
            errOut = QObject::tr("无法覆盖旧备份文件：%1").arg(bakPath);
            return false;
        }
        if (!QFile::copy(path, bakPath)) {
            errOut = QObject::tr("无法创建备份文件：%1").arg(bakPath);
            return false;
        }
    }

    QSaveFile sf(path);
    if (!sf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        errOut = QObject::tr("无法写入文件：%1").arg(sf.errorString());
        return false;
    }
    const QByteArray outBytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (sf.write(outBytes) != outBytes.size()) {
        errOut = QObject::tr("写入数据不完整：%1").arg(sf.errorString());
        sf.cancelWriting();
        return false;
    }
    if (!sf.commit()) {
        errOut = QObject::tr("提交保存失败：%1").arg(sf.errorString());
        return false;
    }
    return true;
}

int findNearestPointIndex(const QJsonArray& pts, double x, double y,
                          int minIdx, double* distOut)
{
    if (pts.isEmpty()) return -1;
    const int start = std::max(0, minIdx);
    int bestIdx = -1;
    double bestD2 = std::numeric_limits<double>::infinity();
    for (int i = start; i < pts.size(); ++i) {
        double px = 0, py = 0;
        if (!pointXY(pts, i, px, py)) continue;
        const double dx = px - x;
        const double dy = py - y;
        const double d2 = dx * dx + dy * dy;
        if (d2 < bestD2) {
            bestD2 = d2;
            bestIdx = i;
        }
    }
    if (distOut) {
        *distOut = (bestIdx >= 0) ? std::sqrt(bestD2) : 0.0;
    }
    return bestIdx;
}

SyncPlan computeSyncPlan(const QJsonArray& srcPts, int srcStart, int srcEnd,
                         const QJsonArray& dstPts, AlignMode mode,
                         int manualDstStart)
{
    SyncPlan plan;
    if (srcPts.isEmpty() || dstPts.isEmpty()) {
        plan.error = QObject::tr("源或目标轨迹为空");
        return plan;
    }
    if (srcStart < 0 || srcEnd < srcStart || srcEnd >= srcPts.size()) {
        plan.error = QObject::tr("源下标范围无效 [%1..%2]").arg(srcStart).arg(srcEnd);
        return plan;
    }

    const int count = srcEnd - srcStart + 1;
    plan.srcCount = count;

    double sx0 = 0, sy0 = 0, sx1 = 0, sy1 = 0;
    if (!pointXY(srcPts, srcStart, sx0, sy0) || !pointXY(srcPts, srcEnd, sx1, sy1)) {
        plan.error = QObject::tr("源起终点缺少有效 x/y 坐标");
        return plan;
    }

    int dStart = -1;
    int dEnd = -1;

    switch (mode) {
    case AlignMode::AutoStartFixedLen:
        dStart = findNearestPointIndex(dstPts, sx0, sy0, 0);
        dEnd = (dStart >= 0) ? (dStart + count - 1) : -1;
        break;
    case AlignMode::AutoStartEnd:
        dStart = findNearestPointIndex(dstPts, sx0, sy0, 0);
        if (dStart >= 0) {
            dEnd = findNearestPointIndex(dstPts, sx1, sy1, dStart);
        }
        break;
    case AlignMode::ManualStartFixed:
        dStart = manualDstStart;
        dEnd = dStart + count - 1;
        break;
    case AlignMode::SameAsSource:
        dStart = srcStart;
        dEnd = srcEnd;
        break;
    }

    if (dStart < 0 || dEnd < dStart || dEnd >= dstPts.size()) {
        plan.dstStart = dStart;
        plan.dstEnd = dEnd;
        plan.error = QObject::tr("目标区间越界 [%1..%2]（目标共 %3 点）")
                         .arg(dStart).arg(dEnd).arg(dstPts.size());
        return plan;
    }

    plan.valid = true;
    plan.dstStart = dStart;
    plan.dstEnd = dEnd;
    plan.dstCount = dEnd - dStart + 1;
    plan.startMatchDist = pointDist(srcPts, srcStart, dstPts, dStart);
    plan.endMatchDist = pointDist(srcPts, srcEnd, dstPts, dEnd);
    plan.joinPrevDist = (dStart > 0) ? pointDist(dstPts, dStart - 1, srcPts, srcStart) : 0.0;
    plan.joinNextDist = (dEnd + 1 < dstPts.size()) ? pointDist(srcPts, srcEnd, dstPts, dEnd + 1) : 0.0;
    return plan;
}

bool applySyncPlan(const QJsonArray& srcPts, int srcStart, int srcEnd,
                   const QJsonObject& dstRoot, const SyncPlan& plan,
                   bool syncPos, bool syncAttr,
                   QJsonObject& outRoot, QString& errOut)
{
    if (!syncPos && !syncAttr) {
        errOut = QObject::tr("请至少勾选「同步位置」或「同步属性」");
        return false;
    }
    if (!plan.valid) {
        errOut = plan.error.isEmpty() ? QObject::tr("同步方案无效") : plan.error;
        return false;
    }

    const QJsonArray dstPts = dstRoot.value(QStringLiteral("points")).toArray();
    if (srcStart < 0 || srcEnd < srcStart || srcEnd >= srcPts.size() ||
        plan.dstStart < 0 || plan.dstEnd < plan.dstStart || plan.dstEnd >= dstPts.size()) {
        errOut = QObject::tr("点位下标越界");
        return false;
    }

    const int srcCount = srcEnd - srcStart + 1;
    const int dstCount = plan.dstEnd - plan.dstStart + 1;
    if ((!syncPos || !syncAttr) && srcCount != dstCount) {
        errOut = QObject::tr("仅同步位置或仅同步属性时，源与目标点数必须相同（源 %1 点 vs 目标 %2 点）；请改用等长对齐模式或同时勾选位置与属性")
                     .arg(srcCount).arg(dstCount);
        return false;
    }

    QJsonArray newPts;
    for (int i = 0; i < plan.dstStart; ++i) {
        newPts.append(dstPts.at(i));
    }

    if (syncPos && syncAttr) {
        for (int i = srcStart; i <= srcEnd; ++i) {
            newPts.append(srcPts.at(i));
        }
    } else if (syncPos && !syncAttr) {
        for (int k = 0; k < srcCount; ++k) {
            const QJsonObject srcObj = srcPts.at(srcStart + k).toObject();
            QJsonObject merged = dstPts.at(plan.dstStart + k).toObject();
            for (const char* pk : kPosKeys) {
                const QString key = QLatin1String(pk);
                if (srcObj.contains(key)) {
                    merged.insert(key, srcObj.value(key));
                } else {
                    merged.remove(key);
                }
            }
            newPts.append(merged);
        }
    } else { // !syncPos && syncAttr
        for (int k = 0; k < srcCount; ++k) {
            const QJsonObject srcObj = srcPts.at(srcStart + k).toObject();
            const QJsonObject dstObj = dstPts.at(plan.dstStart + k).toObject();
            QJsonObject merged;
            for (const char* pk : kPosKeys) {
                const QString key = QLatin1String(pk);
                if (dstObj.contains(key)) {
                    merged.insert(key, dstObj.value(key));
                }
            }
            for (auto it = srcObj.constBegin(); it != srcObj.constEnd(); ++it) {
                if (!isPositionKey(it.key())) {
                    merged.insert(it.key(), it.value());
                }
            }
            newPts.append(merged);
        }
    }

    for (int i = plan.dstEnd + 1; i < dstPts.size(); ++i) {
        newPts.append(dstPts.at(i));
    }

    outRoot = dstRoot;
    outRoot.insert(QStringLiteral("points"), newPts);
    return true;
}

} // namespace blz_sync

// ============================================================================
// 局部轨迹 2D 预览控件
// ============================================================================

class TrajSyncPreviewWidget : public QWidget
{
public:
    explicit TrajSyncPreviewWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(320, 260);
    }

    void setPreviewData(const QJsonArray& srcPts, int srcStart, int srcEnd,
                        const QJsonArray& dstPts, const blz_sync::SyncPlan& plan,
                        const QString& title)
    {
        m_srcPts = srcPts;
        m_srcStart = srcStart;
        m_srcEnd = srcEnd;
        m_dstPts = dstPts;
        m_plan = plan;
        m_title = title;
        update();
    }

    void clearPreview()
    {
        m_srcPts = QJsonArray();
        m_dstPts = QJsonArray();
        m_plan = blz_sync::SyncPlan();
        m_title.clear();
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.fillRect(rect(), QColor(248, 250, 252));
        p.setPen(QColor(210, 216, 224));
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        if (m_srcPts.isEmpty() || m_srcStart < 0 || m_srcEnd < m_srcStart ||
            m_srcEnd >= m_srcPts.size()) {
            p.setPen(QColor(120, 130, 145));
            p.drawText(rect(), Qt::AlignCenter, tr("选择源轨迹区间与目标文件后\n在此预览局部几何衔接"));
            return;
        }

        double minX = std::numeric_limits<double>::infinity();
        double maxX = -std::numeric_limits<double>::infinity();
        double minY = std::numeric_limits<double>::infinity();
        double maxY = -std::numeric_limits<double>::infinity();

        auto expandBox = [&](const QJsonArray& arr, int a, int b) {
            const int lo = std::max(0, a);
            const int hi = std::min<int>(arr.size() - 1, b);
            for (int i = lo; i <= hi; ++i) {
                const QJsonObject o = arr.at(i).toObject();
                const double x = o.value(QStringLiteral("x")).toDouble();
                const double y = o.value(QStringLiteral("y")).toDouble();
                if (!std::isfinite(x) || !std::isfinite(y)) continue;
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }
        };

        expandBox(m_srcPts, m_srcStart, m_srcEnd);
        const int ctxPad = 12;
        int dstCtxA = -1, dstCtxB = -1;
        if (m_plan.valid && !m_dstPts.isEmpty()) {
            dstCtxA = std::max(0, m_plan.dstStart - ctxPad);
            dstCtxB = std::min<int>(m_dstPts.size() - 1, m_plan.dstEnd + ctxPad);
            expandBox(m_dstPts, dstCtxA, dstCtxB);
        }

        if (!std::isfinite(minX)) return;

        const double spanX = std::max(2.0, maxX - minX);
        const double spanY = std::max(2.0, maxY - minY);
        const int padPx = 28;
        const double availW = std::max(20.0, width() - 2.0 * padPx);
        const double availH = std::max(20.0, height() - 2.0 * padPx - 22.0);
        const double scale = std::min(availW / spanX, availH / spanY);
        const double cx = 0.5 * (minX + maxX);
        const double cy = 0.5 * (minY + maxY);
        const double midPxX = width() * 0.5;
        const double midPxY = (height() + 22.0) * 0.5;

        auto toScreen = [&](double wx, double wy) -> QPointF {
            return QPointF(midPxX + (wx - cx) * scale,
                           midPxY - (wy - cy) * scale);
        };

        auto ptAt = [&](const QJsonArray& arr, int idx) -> QPointF {
            const QJsonObject o = arr.at(idx).toObject();
            return toScreen(o.value(QStringLiteral("x")).toDouble(),
                            o.value(QStringLiteral("y")).toDouble());
        };

        // 1. 绘制目标轨迹上下文（蓝色：保留前后段 + 虚线灰蓝：被替换旧段）
        if (m_plan.valid && !m_dstPts.isEmpty()) {
            // 前保留段
            p.setPen(QPen(QColor(37, 99, 235), 2.0));
            for (int i = dstCtxA; i < m_plan.dstStart - 1; ++i) {
                p.drawLine(ptAt(m_dstPts, i), ptAt(m_dstPts, i + 1));
            }
            // 后保留段
            for (int i = m_plan.dstEnd + 1; i < dstCtxB; ++i) {
                p.drawLine(ptAt(m_dstPts, i), ptAt(m_dstPts, i + 1));
            }
            // 被替换的旧段
            QPen oldPen(QColor(148, 163, 184), 1.5, Qt::DashLine);
            p.setPen(oldPen);
            for (int i = std::max(0, m_plan.dstStart - 1); i < std::min<int>(m_dstPts.size() - 1, m_plan.dstEnd + 1); ++i) {
                p.drawLine(ptAt(m_dstPts, i), ptAt(m_dstPts, i + 1));
            }
            // 保留段点
            p.setBrush(QColor(37, 99, 235));
            p.setPen(Qt::NoPen);
            for (int i = dstCtxA; i <= dstCtxB; ++i) {
                if (i >= m_plan.dstStart && i <= m_plan.dstEnd) continue;
                p.drawEllipse(ptAt(m_dstPts, i), 2.5, 2.5);
            }
            // 衔接跳变线（橙色虚线）
            QPen joinPen(QColor(234, 88, 12), 2.0, Qt::DashLine);
            p.setPen(joinPen);
            if (m_plan.dstStart > 0) {
                p.drawLine(ptAt(m_dstPts, m_plan.dstStart - 1), ptAt(m_srcPts, m_srcStart));
            }
            if (m_plan.dstEnd + 1 < m_dstPts.size()) {
                p.drawLine(ptAt(m_srcPts, m_srcEnd), ptAt(m_dstPts, m_plan.dstEnd + 1));
            }
        }

        // 2. 绘制源轨迹同步段（红色）
        p.setPen(QPen(QColor(220, 38, 38), 2.2));
        for (int i = m_srcStart; i < m_srcEnd; ++i) {
            p.drawLine(ptAt(m_srcPts, i), ptAt(m_srcPts, i + 1));
        }
        p.setBrush(QColor(220, 38, 38));
        p.setPen(Qt::NoPen);
        for (int i = m_srcStart; i <= m_srcEnd; ++i) {
            const double r = (i == m_srcStart || i == m_srcEnd) ? 4.5 : 2.5;
            p.drawEllipse(ptAt(m_srcPts, i), r, r);
        }

        // 起终点标注
        p.setPen(QColor(153, 27, 27));
        const QPointF pS = ptAt(m_srcPts, m_srcStart);
        const QPointF pE = ptAt(m_srcPts, m_srcEnd);
        p.drawText(pS + QPointF(6, -4), QString("S(%1)").arg(m_srcStart));
        p.drawText(pE + QPointF(6, -4), QString("E(%1)").arg(m_srcEnd));

        // 顶部图例
        p.setPen(QColor(30, 41, 59));
        const QString legend = m_title.isEmpty()
            ? tr("红=源同步段")
            : tr("%1  |  红=源同步段  蓝=目标保留段  灰虚线=被替换旧段  橙虚线=前后衔接").arg(m_title);
        p.drawText(QRect(8, 4, width() - 16, 20), Qt::AlignLeft | Qt::AlignVCenter, legend);
    }

private:
    QJsonArray m_srcPts;
    int m_srcStart = 0;
    int m_srcEnd = 0;
    QJsonArray m_dstPts;
    blz_sync::SyncPlan m_plan;
    QString m_title;
};

// ============================================================================
// TrajSyncDialog 实现
// ============================================================================

namespace {

enum Col {
    COL_FILE = 0,
    COL_TOTAL,
    COL_MODE,
    COL_RANGE,
    COL_MANUAL_START,
    COL_JOIN_DIST,
    COL_STATUS,
    COL_COUNT
};

QString modeLabel(blz_sync::AlignMode m)
{
    switch (m) {
    case blz_sync::AlignMode::AutoStartFixedLen:
        return QObject::tr("自动对齐起点(等长)");
    case blz_sync::AlignMode::AutoStartEnd:
        return QObject::tr("自动对齐起终点(可变长)");
    case blz_sync::AlignMode::ManualStartFixed:
        return QObject::tr("手动起始下标(等长)");
    case blz_sync::AlignMode::SameAsSource:
        return QObject::tr("与源同下标");
    }
    return QString();
}

} // namespace

TrajSyncDialog::TrajSyncDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("轨迹段属性与位置同步工具"));
    resize(1060, 680);
    setMinimumSize(900, 560);
    setupUi();
}

void TrajSyncDialog::setupUi()
{
    auto* mainLay = new QVBoxLayout(this);
    mainLay->setSpacing(8);

    // ---- 1. 源轨迹区 ----
    auto* srcBox = new QGroupBox(tr("1. 源轨迹与同步区间（0-based 下标，与 MapTool 一致）"), this);
    auto* srcLay = new QVBoxLayout(srcBox);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("源轨迹:"), srcBox));
    m_editSrcPath = new QLineEdit(srcBox);
    m_editSrcPath->setReadOnly(true);
    m_editSrcPath->setPlaceholderText(tr("请选择源 .json 文件，或从当前画布读取"));
    row1->addWidget(m_editSrcPath, 1);

    m_btnSrcBrowse = new QPushButton(tr("选择源 JSON..."), srcBox);
    m_btnSrcCanvas = new QPushButton(tr("使用当前画布"), srcBox);
    m_lblSrcTotal = new QLabel(tr("总点数: -"), srcBox);
    m_lblSrcTotal->setMinimumWidth(95);
    row1->addWidget(m_btnSrcBrowse);
    row1->addWidget(m_btnSrcCanvas);
    row1->addWidget(m_lblSrcTotal);
    srcLay->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("起始下标:"), srcBox));
    m_spinSrcStart = new QSpinBox(srcBox);
    m_spinSrcStart->setRange(0, 0);
    m_spinSrcStart->setMinimumWidth(85);
    row2->addWidget(m_spinSrcStart);

    row2->addWidget(new QLabel(tr("结束下标:"), srcBox));
    m_spinSrcEnd = new QSpinBox(srcBox);
    m_spinSrcEnd->setRange(0, 0);
    m_spinSrcEnd->setMinimumWidth(85);
    row2->addWidget(m_spinSrcEnd);

    m_btnPickCanvasRange = new QPushButton(tr("读取画布选区"), srcBox);
    m_btnPickCanvasRange->setToolTip(tr("读取主画布「点选点(选定开始/结束点)」的下标范围"));
    row2->addWidget(m_btnPickCanvasRange);

    m_lblSrcInfo = new QLabel(tr("请先加载源轨迹"), srcBox);
    row2->addWidget(m_lblSrcInfo, 1);
    srcLay->addLayout(row2);
    mainLay->addWidget(srcBox);

    // ---- 2. 同步内容与默认对齐模式 ----
    auto* optBox = new QGroupBox(tr("2. 同步选项"), this);
    auto* optLay = new QHBoxLayout(optBox);

    m_chkSyncPos = new QCheckBox(tr("同步位置 (x, y, yaw, pixel_x, pixel_y)"), optBox);
    m_chkSyncPos->setChecked(true);
    m_chkSyncAttr = new QCheckBox(tr("同步属性 (road_id, bypass, force_pass 等全部字段)"), optBox);
    m_chkSyncAttr->setChecked(true);
    m_chkBackup = new QCheckBox(tr("写入前生成 .bak 备份"), optBox);
    m_chkBackup->setChecked(true);

    optLay->addWidget(m_chkSyncPos);
    optLay->addWidget(m_chkSyncAttr);
    optLay->addWidget(m_chkBackup);
    optLay->addStretch(1);

    optLay->addWidget(new QLabel(tr("默认对齐:"), optBox));
    m_comboDefaultMode = new QComboBox(optBox);
    m_comboDefaultMode->addItem(modeLabel(blz_sync::AlignMode::AutoStartFixedLen),
                                static_cast<int>(blz_sync::AlignMode::AutoStartFixedLen));
    m_comboDefaultMode->addItem(modeLabel(blz_sync::AlignMode::AutoStartEnd),
                                static_cast<int>(blz_sync::AlignMode::AutoStartEnd));
    m_comboDefaultMode->addItem(modeLabel(blz_sync::AlignMode::SameAsSource),
                                static_cast<int>(blz_sync::AlignMode::SameAsSource));
    m_comboDefaultMode->addItem(modeLabel(blz_sync::AlignMode::ManualStartFixed),
                                static_cast<int>(blz_sync::AlignMode::ManualStartFixed));
    optLay->addWidget(m_comboDefaultMode);

    auto* btnApplyDefault = new QPushButton(tr("应用到全部目标"), optBox);
    optLay->addWidget(btnApplyDefault);
    mainLay->addWidget(optBox);

    // ---- 3. 目标列表 + 右侧实时预览 ----
    auto* split = new QSplitter(Qt::Horizontal, this);

    auto* leftPane = new QWidget(split);
    auto* leftLay = new QVBoxLayout(leftPane);
    leftLay->setContentsMargins(0, 0, 0, 0);

    auto* targetBar = new QHBoxLayout();
    targetBar->addWidget(new QLabel(tr("3. 目标文件列表（支持批量添加）"), leftPane));
    targetBar->addStretch(1);
    auto* btnAddTarget = new QPushButton(tr("+ 添加目标 JSON..."), leftPane);
    auto* btnRmTarget = new QPushButton(tr("移除选中"), leftPane);
    auto* btnClearTarget = new QPushButton(tr("清空"), leftPane);
    targetBar->addWidget(btnAddTarget);
    targetBar->addWidget(btnRmTarget);
    targetBar->addWidget(btnClearTarget);
    leftLay->addLayout(targetBar);

    m_table = new QTableWidget(0, COL_COUNT, leftPane);
    m_table->setHorizontalHeaderLabels({
        tr("目标文件"), tr("原点数"), tr("对齐方式"),
        tr("目标区间"), tr("起始下标"), tr("衔接距(前/后)"), tr("状态")
    });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(COL_FILE, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(COL_TOTAL, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_MODE, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_RANGE, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_MANUAL_START, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_JOIN_DIST, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_STATUS, QHeaderView::Stretch);
    leftLay->addWidget(m_table, 1);

    auto* rightPane = new QWidget(split);
    auto* rightLay = new QVBoxLayout(rightPane);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->addWidget(new QLabel(tr("局部衔接预览"), rightPane));
    m_preview = new TrajSyncPreviewWidget(rightPane);
    rightLay->addWidget(m_preview, 1);
    m_lblPreviewInfo = new QLabel(tr("选中目标行可查看衔接细节"), rightPane);
    m_lblPreviewInfo->setWordWrap(true);
    rightLay->addWidget(m_lblPreviewInfo);

    split->addWidget(leftPane);
    split->addWidget(rightPane);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);
    mainLay->addWidget(split, 1);

    // ---- 4. 底部操作栏 ----
    auto* bottomLay = new QHBoxLayout();
    m_lblStatus = new QLabel(tr("就绪"), this);
    bottomLay->addWidget(m_lblStatus, 1);
    auto* btnClose = new QPushButton(tr("关闭"), this);
    m_btnExecute = new QPushButton(tr("执行同步"), this);
    m_btnExecute->setDefault(true);
    m_btnExecute->setMinimumWidth(140);
    bottomLay->addWidget(btnClose);
    bottomLay->addWidget(m_btnExecute);
    mainLay->addLayout(bottomLay);

    // ---- 信号接线 ----
    connect(m_btnSrcBrowse, &QPushButton::clicked, this, &TrajSyncDialog::browseSourceFile);
    connect(m_btnSrcCanvas, &QPushButton::clicked, this, &TrajSyncDialog::loadSourceFromCanvas);
    connect(m_btnPickCanvasRange, &QPushButton::clicked, this, &TrajSyncDialog::pickRangeFromCanvas);
    connect(btnAddTarget, &QPushButton::clicked, this, &TrajSyncDialog::addTargetFiles);
    connect(btnRmTarget, &QPushButton::clicked, this, &TrajSyncDialog::removeSelectedTargets);
    connect(btnClearTarget, &QPushButton::clicked, this, &TrajSyncDialog::clearTargets);
    connect(btnApplyDefault, &QPushButton::clicked, this, &TrajSyncDialog::applyDefaultModeToAll);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::close);
    connect(m_btnExecute, &QPushButton::clicked, this, &TrajSyncDialog::executeSync);

    connect(m_spinSrcStart, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_spinSrcEnd->value() < val) {
            m_spinSrcEnd->setValue(val);
        }
        updateSourceRangeInfo();
        recomputeAllPlans();
    });
    connect(m_spinSrcEnd, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_spinSrcStart->value() > val) {
            m_spinSrcStart->setValue(val);
        }
        updateSourceRangeInfo();
        recomputeAllPlans();
    });
    connect(m_chkSyncPos, &QCheckBox::toggled, this, &TrajSyncDialog::recomputeAllPlans);
    connect(m_chkSyncAttr, &QCheckBox::toggled, this, &TrajSyncDialog::recomputeAllPlans);
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &TrajSyncDialog::onTargetSelectionChanged);
}

void TrajSyncDialog::setDefaultDir(const QString& dir)
{
    if (!dir.isEmpty() && QDir(dir).exists()) {
        m_defaultDir = dir;
    }
}

void TrajSyncDialog::setSegmentPicker(SegmentPicker picker)
{
    m_segmentPicker = std::move(picker);
}

void TrajSyncDialog::setCanvasExporter(CanvasExporter exporter)
{
    m_canvasExporter = std::move(exporter);
}

void TrajSyncDialog::setSourceFileIfEmpty(const QString& jsonPath)
{
    if (!m_srcPoints.isEmpty() || jsonPath.isEmpty() || !QFileInfo::exists(jsonPath))
        return;
    QJsonObject root;
    QString err;
    if (blz_sync::loadTrajectoryJson(jsonPath, root, err)) {
        loadSourceObject(root, jsonPath);
        m_srcFromCanvas = false;
    }
}

QString TrajSyncDialog::currentBrowseDir() const
{
    if (!m_defaultDir.isEmpty() && QDir(m_defaultDir).exists())
        return m_defaultDir;
    const QString saved = QSettings().value(QStringLiteral("trajSyncLastDir")).toString();
    if (!saved.isEmpty() && QDir(saved).exists())
        return saved;
    return QDir::homePath();
}

void TrajSyncDialog::browseSourceFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择源轨迹 JSON"), currentBrowseDir(), tr("JSON Files (*.json)"));
    if (path.isEmpty()) return;

    const QString dir = QFileInfo(path).absolutePath();
    m_defaultDir = dir;
    QSettings().setValue(QStringLiteral("trajSyncLastDir"), dir);

    QJsonObject root;
    QString err;
    if (!blz_sync::loadTrajectoryJson(path, root, err)) {
        QMessageBox::critical(this, tr("源轨迹加载失败"), err);
        return;
    }
    loadSourceObject(root, path);
    m_srcFromCanvas = false;
}

void TrajSyncDialog::loadSourceFromCanvas()
{
    if (!m_canvasExporter) {
        QMessageBox::warning(this, tr("提示"), tr("当前未接线画布导出器"));
        return;
    }
    const QJsonObject root = m_canvasExporter();
    const QJsonArray pts = root.value(QStringLiteral("points")).toArray();
    if (pts.isEmpty()) {
        QMessageBox::warning(this, tr("无法读取画布轨迹"),
                             tr("请确认主画布已打开底图、YAML 地图参数且存在轨迹点。"));
        return;
    }
    loadSourceObject(root, tr("<当前画布轨迹>"));
    m_srcFromCanvas = true;
    pickRangeFromCanvas();
}

void TrajSyncDialog::loadSourceObject(const QJsonObject& root, const QString& label)
{
    m_srcRoot = root;
    m_srcPoints = root.value(QStringLiteral("points")).toArray();
    m_editSrcPath->setText(label);
    const int n = m_srcPoints.size();
    m_lblSrcTotal->setText(tr("总点数: %1").arg(n));

    const int maxIdx = std::max(0, n - 1);
    m_spinSrcStart->blockSignals(true);
    m_spinSrcEnd->blockSignals(true);
    m_spinSrcStart->setRange(0, maxIdx);
    m_spinSrcEnd->setRange(0, maxIdx);
    if (m_spinSrcStart->value() > maxIdx) m_spinSrcStart->setValue(0);
    if (m_spinSrcEnd->value() > maxIdx || m_spinSrcEnd->value() == 0)
        m_spinSrcEnd->setValue(std::min(maxIdx, 0));
    m_spinSrcStart->blockSignals(false);
    m_spinSrcEnd->blockSignals(false);

    updateSourceRangeInfo();
    recomputeAllPlans();
}

void TrajSyncDialog::pickRangeFromCanvas()
{
    if (!m_segmentPicker) return;
    int fromIdx = 0, toIdx = 0, total = 0;
    if (!m_segmentPicker(fromIdx, toIdx, total)) {
        m_lblStatus->setText(tr("画布未选中起止点（可使用「点选点」先选中起止点，或手动输入下标）"));
        return;
    }
    if (m_srcPoints.isEmpty()) {
        m_lblStatus->setText(tr("已检测到画布选区 [%1..%2]，请先加载源轨迹").arg(fromIdx).arg(toIdx));
        return;
    }
    if (toIdx >= m_srcPoints.size()) {
        QMessageBox::warning(this, tr("下标超出范围"),
                             tr("画布选区 [%1..%2] 超出源轨迹总点数 %3")
                                 .arg(fromIdx).arg(toIdx).arg(m_srcPoints.size()));
        return;
    }
    m_spinSrcStart->blockSignals(true);
    m_spinSrcEnd->blockSignals(true);
    m_spinSrcStart->setValue(fromIdx);
    m_spinSrcEnd->setValue(toIdx);
    m_spinSrcStart->blockSignals(false);
    m_spinSrcEnd->blockSignals(false);
    updateSourceRangeInfo();
    recomputeAllPlans();
}

void TrajSyncDialog::updateSourceRangeInfo()
{
    if (m_srcPoints.isEmpty()) {
        m_lblSrcInfo->setText(tr("请先加载源轨迹"));
        return;
    }
    const int s = m_spinSrcStart->value();
    const int e = m_spinSrcEnd->value();
    const QJsonObject p0 = m_srcPoints.at(s).toObject();
    const QJsonObject p1 = m_srcPoints.at(e).toObject();
    m_lblSrcInfo->setText(
        tr("共 %1 点 | 起点 (%2, %3) → 终点 (%4, %5)")
            .arg(e - s + 1)
            .arg(p0.value(QStringLiteral("x")).toDouble(), 0, 'f', 3)
            .arg(p0.value(QStringLiteral("y")).toDouble(), 0, 'f', 3)
            .arg(p1.value(QStringLiteral("x")).toDouble(), 0, 'f', 3)
            .arg(p1.value(QStringLiteral("y")).toDouble(), 0, 'f', 3));
}

void TrajSyncDialog::addTargetFiles()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("添加目标轨迹 JSON（可多选）"), currentBrowseDir(), tr("JSON Files (*.json)"));
    if (paths.isEmpty()) return;

    const QString firstDir = QFileInfo(paths.first()).absolutePath();
    m_defaultDir = firstDir;
    QSettings().setValue(QStringLiteral("trajSyncLastDir"), firstDir);

    const auto defMode = static_cast<blz_sync::AlignMode>(
        m_comboDefaultMode->currentData().toInt());

    QStringList errs;
    for (const QString& p : paths) {
        if (!m_srcFromCanvas && QFileInfo(p).canonicalFilePath() == QFileInfo(m_editSrcPath->text()).canonicalFilePath()) {
            errs << tr("%1：与源轨迹为同一文件，已跳过").arg(QFileInfo(p).fileName());
            continue;
        }
        bool dup = false;
        for (const auto& t : m_targets) {
            if (QFileInfo(t.filePath).canonicalFilePath() == QFileInfo(p).canonicalFilePath()) {
                dup = true;
                break;
            }
        }
        if (dup) continue;

        QJsonObject root;
        QString err;
        if (!blz_sync::loadTrajectoryJson(p, root, err)) {
            errs << QString("%1：%2").arg(QFileInfo(p).fileName(), err);
            continue;
        }
        TargetEntry entry;
        entry.filePath = p;
        entry.root = root;
        entry.points = root.value(QStringLiteral("points")).toArray();
        entry.mode = defMode;
        entry.manualStart = m_spinSrcStart->value();
        m_targets.append(entry);
    }

    refreshTargetTable();
    recomputeAllPlans();
    if (!errs.isEmpty()) {
        QMessageBox::warning(this, tr("部分文件未添加"), errs.join(QLatin1Char('\n')));
    }
}

void TrajSyncDialog::removeSelectedTargets()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_targets.size()) return;
    m_targets.removeAt(row);
    refreshTargetTable();
    onTargetSelectionChanged();
}

void TrajSyncDialog::clearTargets()
{
    m_targets.clear();
    refreshTargetTable();
    m_preview->clearPreview();
    m_lblPreviewInfo->setText(tr("目标列表已清空"));
}

void TrajSyncDialog::applyDefaultModeToAll()
{
    const auto defMode = static_cast<blz_sync::AlignMode>(
        m_comboDefaultMode->currentData().toInt());
    for (auto& t : m_targets) {
        t.mode = defMode;
        t.synced = false;
        t.resultNote.clear();
    }
    refreshTargetTable();
    recomputeAllPlans();
}

void TrajSyncDialog::refreshTargetTable()
{
    m_updatingTable = true;
    m_table->setRowCount(m_targets.size());
    for (int r = 0; r < m_targets.size(); ++r) {
        const TargetEntry& t = m_targets.at(r);
        auto* fileItem = new QTableWidgetItem(QFileInfo(t.filePath).fileName());
        fileItem->setToolTip(t.filePath);
        m_table->setItem(r, COL_FILE, fileItem);

        auto* totalItem = new QTableWidgetItem(QString::number(t.points.size()));
        totalItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, COL_TOTAL, totalItem);

        auto* modeCombo = new QComboBox(m_table);
        modeCombo->addItem(modeLabel(blz_sync::AlignMode::AutoStartFixedLen),
                           static_cast<int>(blz_sync::AlignMode::AutoStartFixedLen));
        modeCombo->addItem(modeLabel(blz_sync::AlignMode::AutoStartEnd),
                           static_cast<int>(blz_sync::AlignMode::AutoStartEnd));
        modeCombo->addItem(modeLabel(blz_sync::AlignMode::SameAsSource),
                           static_cast<int>(blz_sync::AlignMode::SameAsSource));
        modeCombo->addItem(modeLabel(blz_sync::AlignMode::ManualStartFixed),
                           static_cast<int>(blz_sync::AlignMode::ManualStartFixed));
        const int idx = modeCombo->findData(static_cast<int>(t.mode));
        if (idx >= 0) modeCombo->setCurrentIndex(idx);
        m_table->setCellWidget(r, COL_MODE, modeCombo);
        connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, r, modeCombo](int) {
            if (m_updatingTable || r < 0 || r >= m_targets.size()) return;
            m_targets[r].mode = static_cast<blz_sync::AlignMode>(
                modeCombo->currentData().toInt());
            m_targets[r].synced = false;
            m_targets[r].resultNote.clear();
            updateRowPlan(r);
            if (m_table->currentRow() == r) updatePreviewForRow(r);
        });

        m_table->setItem(r, COL_RANGE, new QTableWidgetItem());

        auto* startSpin = new QSpinBox(m_table);
        startSpin->setRange(0, std::max(0, static_cast<int>(t.points.size()) - 1));
        startSpin->setValue(t.manualStart);
        startSpin->setEnabled(t.mode == blz_sync::AlignMode::ManualStartFixed);
        m_table->setCellWidget(r, COL_MANUAL_START, startSpin);
        connect(startSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this, r](int val) {
            if (m_updatingTable || r < 0 || r >= m_targets.size()) return;
            m_targets[r].manualStart = val;
            m_targets[r].synced = false;
            m_targets[r].resultNote.clear();
            updateRowPlan(r);
            if (m_table->currentRow() == r) updatePreviewForRow(r);
        });

        m_table->setItem(r, COL_JOIN_DIST, new QTableWidgetItem());
        m_table->setItem(r, COL_STATUS, new QTableWidgetItem());
    }
    m_updatingTable = false;

    if (!m_targets.isEmpty() && m_table->currentRow() < 0) {
        m_table->selectRow(0);
    }
}

void TrajSyncDialog::recomputeAllPlans()
{
    for (int r = 0; r < m_targets.size(); ++r) {
        m_targets[r].synced = false;
        m_targets[r].resultNote.clear();
        updateRowPlan(r);
    }
    const int cur = m_table->currentRow();
    if (cur >= 0 && cur < m_targets.size()) {
        updatePreviewForRow(cur);
    } else if (!m_targets.isEmpty()) {
        updatePreviewForRow(0);
    } else {
        m_preview->clearPreview();
    }
    m_btnExecute->setText(m_targets.isEmpty()
        ? tr("执行同步")
        : tr("执行同步 (%1 个文件)").arg(m_targets.size()));
}

void TrajSyncDialog::updateRowPlan(int row)
{
    if (row < 0 || row >= m_targets.size()) return;
    TargetEntry& t = m_targets[row];
    auto* startSpin = qobject_cast<QSpinBox*>(m_table->cellWidget(row, COL_MANUAL_START));
    if (startSpin) {
        startSpin->setEnabled(t.mode == blz_sync::AlignMode::ManualStartFixed);
    }

    if (m_srcPoints.isEmpty()) {
        m_table->item(row, COL_RANGE)->setText("-");
        m_table->item(row, COL_JOIN_DIST)->setText("-");
        m_table->item(row, COL_STATUS)->setText(tr("等待源轨迹"));
        return;
    }

    const int sStart = m_spinSrcStart->value();
    const int sEnd = m_spinSrcEnd->value();
    t.plan = blz_sync::computeSyncPlan(m_srcPoints, sStart, sEnd,
                                       t.points, t.mode, t.manualStart);

    if (startSpin && t.mode != blz_sync::AlignMode::ManualStartFixed && t.plan.dstStart >= 0) {
        startSpin->blockSignals(true);
        startSpin->setValue(t.plan.dstStart);
        startSpin->blockSignals(false);
        t.manualStart = t.plan.dstStart;
    }

    auto* rangeItem = m_table->item(row, COL_RANGE);
    auto* joinItem = m_table->item(row, COL_JOIN_DIST);
    auto* statusItem = m_table->item(row, COL_STATUS);

    if (!t.plan.valid) {
        rangeItem->setText(t.plan.dstStart >= 0
            ? QString("%1..%2").arg(t.plan.dstStart).arg(t.plan.dstEnd)
            : QString("-"));
        joinItem->setText("-");
        statusItem->setText(tr("❌ %1").arg(t.plan.error));
        statusItem->setForeground(QBrush(QColor(200, 30, 30)));
        return;
    }

    const int delta = t.plan.srcCount - t.plan.dstCount;
    const QString deltaStr = (delta == 0)
        ? tr("%1点").arg(t.plan.dstCount)
        : tr("%1→%2点").arg(t.plan.dstCount).arg(t.plan.srcCount);
    rangeItem->setText(QString("[%1..%2] (%3)")
        .arg(t.plan.dstStart).arg(t.plan.dstEnd).arg(deltaStr));

    joinItem->setText(QString("%1m / %2m")
        .arg(t.plan.joinPrevDist, 0, 'f', 2)
        .arg(t.plan.joinNextDist, 0, 'f', 2));

    const bool needEqualLen = (!m_chkSyncPos->isChecked() || !m_chkSyncAttr->isChecked());
    if (!m_chkSyncPos->isChecked() && !m_chkSyncAttr->isChecked()) {
        statusItem->setText(tr("❌ 未勾选同步内容"));
        statusItem->setForeground(QBrush(QColor(200, 30, 30)));
    } else if (needEqualLen && delta != 0) {
        statusItem->setText(tr("❌ 仅同步位置/属性需等长"));
        statusItem->setForeground(QBrush(QColor(200, 30, 30)));
    } else if (t.synced) {
        statusItem->setText(t.resultNote);
        statusItem->setForeground(QBrush(QColor(22, 163, 74)));
    } else if (t.plan.joinPrevDist > 1.5 || t.plan.joinNextDist > 1.5 || t.plan.startMatchDist > 3.0) {
        statusItem->setText(tr("⚠️ 衔接有较大跳变（可右图确认）"));
        statusItem->setForeground(QBrush(QColor(217, 119, 6)));
    } else {
        statusItem->setText(tr("就绪"));
        statusItem->setForeground(QBrush(QColor(22, 163, 74)));
    }
}

void TrajSyncDialog::onTargetSelectionChanged()
{
    const int row = m_table->currentRow();
    updatePreviewForRow(row);
}

void TrajSyncDialog::updatePreviewForRow(int row)
{
    if (row < 0 || row >= m_targets.size() || m_srcPoints.isEmpty()) {
        m_preview->clearPreview();
        m_lblPreviewInfo->setText(tr("选中目标行可查看衔接细节"));
        return;
    }
    const TargetEntry& t = m_targets.at(row);
    const int sStart = m_spinSrcStart->value();
    const int sEnd = m_spinSrcEnd->value();
    m_preview->setPreviewData(m_srcPoints, sStart, sEnd, t.points, t.plan,
                              QFileInfo(t.filePath).fileName());
    if (t.plan.valid) {
        m_lblPreviewInfo->setText(
            tr("目标区间 [%1..%2] | 起点偏差 %3m，终点偏差 %4m | 衔接间距：前 %5m，后 %6m")
                .arg(t.plan.dstStart).arg(t.plan.dstEnd)
                .arg(t.plan.startMatchDist, 0, 'f', 2)
                .arg(t.plan.endMatchDist, 0, 'f', 2)
                .arg(t.plan.joinPrevDist, 0, 'f', 2)
                .arg(t.plan.joinNextDist, 0, 'f', 2));
    } else {
        m_lblPreviewInfo->setText(t.plan.error);
    }
}

void TrajSyncDialog::executeSync()
{
    if (m_srcPoints.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请先加载源轨迹"));
        return;
    }
    if (m_targets.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请先添加至少一个目标 JSON 文件"));
        return;
    }
    const bool syncPos = m_chkSyncPos->isChecked();
    const bool syncAttr = m_chkSyncAttr->isChecked();
    if (!syncPos && !syncAttr) {
        QMessageBox::warning(this, tr("提示"), tr("请至少勾选「同步位置」或「同步属性」"));
        return;
    }

    const int sStart = m_spinSrcStart->value();
    const int sEnd = m_spinSrcEnd->value();
    const bool makeBackup = m_chkBackup->isChecked();

    int okCount = 0;
    QStringList errs;
    for (int r = 0; r < m_targets.size(); ++r) {
        TargetEntry& t = m_targets[r];
        QJsonObject outRoot;
        QString err;
        if (!blz_sync::applySyncPlan(m_srcPoints, sStart, sEnd, t.root, t.plan,
                                     syncPos, syncAttr, outRoot, err)) {
            errs << QString("%1：%2").arg(QFileInfo(t.filePath).fileName(), err);
            continue;
        }
        if (!blz_sync::saveTrajectoryJson(t.filePath, outRoot, makeBackup, err)) {
            errs << QString("%1：%2").arg(QFileInfo(t.filePath).fileName(), err);
            continue;
        }
        const int oldN = t.points.size();
        t.root = outRoot;
        t.points = outRoot.value(QStringLiteral("points")).toArray();
        const int newN = t.points.size();
        t.synced = true;
        t.resultNote = (oldN == newN)
            ? tr("✅ 已同步 (%1点)").arg(newN)
            : tr("✅ 已同步 (%1→%2点)").arg(oldN).arg(newN);
        updateRowPlan(r);
        ++okCount;
    }

    onTargetSelectionChanged();
    m_lblStatus->setText(tr("同步完成：成功 %1 个，失败 %2 个").arg(okCount).arg(errs.size()));
    if (!errs.isEmpty()) {
        QMessageBox::warning(this, tr("部分文件同步失败"), errs.join(QLatin1Char('\n')));
    } else {
        QMessageBox::information(this, tr("同步完成"),
                                 tr("已成功同步 %1 个目标文件%2。")
                                     .arg(okCount)
                                     .arg(makeBackup ? tr("（已生成 .bak 备份）") : QString()));
    }
}
