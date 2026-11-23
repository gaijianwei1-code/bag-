// T2.4 MapTool 直播跟车实现
#include "live_layer.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>
#include <QDockWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QTimer>
#include <QMainWindow>
#include <QWebSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QtMath>
#include <QSignalBlocker>
#include <QFileDialog>
#include <QStandardPaths>
#include <QGraphicsView>
#include <QImage>
#include <QDir>
#include <QFileInfo>

#include "../items/GraphicsType.h"

namespace {
void uncheckQuietly(QPushButton* btn)
{
    if (btn && btn->isChecked()) {
        QSignalBlocker blocker(btn);
        btn->setChecked(false);
    }
}
} // namespace

// ================= LiveOverlayItem =================

LiveOverlayItem::LiveOverlayItem(QGraphicsItem* parent)
    : QGraphicsObject(parent)
{
    setAcceptedMouseButtons(Qt::NoButton);   // 全透传，不影响编辑
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    // z=0.8：高于地图(0)与前视层(0.5)，低于选区工具(1000)
    setZValue(0.8);
}

QRectF LiveOverlayItem::contentRect() const
{
    QRectF r;
    bool has = false;
    auto add = [&](const QRectF& rr) {
        r = has ? r.united(rr) : rr;
        has = true;
    };
    if (m_hasVehicle) {
        const double rBody = 0.5 * std::hypot(m_bodyLenPx, m_bodyWidPx) + 10.0;
        add(QRectF(m_vehicle.x() - rBody, m_vehicle.y() - rBody, rBody * 2.0, rBody * 2.0));
    }
    if (m_hasVehicle && m_hasLookahead && m_lookaheadPx > 0.0) {
        const double frontEnd = m_bodyLenPx * 0.5 + m_lookaheadPx;
        const double rLa = std::hypot(frontEnd, m_bodyWidPx * 0.5) + 16.0;
        add(QRectF(m_vehicle.x() - rLa, m_vehicle.y() - rLa, rLa * 2.0, rLa * 2.0));
        const QPointF tip = m_vehicle + QPointF(frontEnd * std::cos(m_yawPix),
                                                frontEnd * std::sin(m_yawPix));
        add(QRectF(tip.x() - 48, tip.y() - 24, 96, 48));
    }
    if (m_hasCarrot) add(QRectF(m_carrot.x() - 8, m_carrot.y() - 8, 16, 16));
    for (const auto& p : m_truthPath) add(QRectF(p.x() - 2, p.y() - 2, 4, 4));
    for (const auto& p : m_livePath) add(QRectF(p.x() - 2, p.y() - 2, 4, 4));
    for (const auto& p : m_hits) add(QRectF(p.x() - 4, p.y() - 4, 8, 8));
    return r.adjusted(-30, -30, 30, 30);
}

QRectF LiveOverlayItem::boundingRect() const
{
    return m_brect;
}

void LiveOverlayItem::setVehicle(const QPointF& px, double yawPix, bool valid)
{
    prepareGeometryChange();
    m_vehicle = px;
    m_yawPix = yawPix;
    m_hasVehicle = valid;
    m_brect = contentRect();
    update();
}

void LiveOverlayItem::setVehicleSize(double lengthPx, double widthPx)
{
    if (lengthPx <= 0.0 || widthPx <= 0.0) return;
    prepareGeometryChange();
    m_bodyLenPx = lengthPx;
    m_bodyWidPx = widthPx;
    m_brect = contentRect();
    update();
}

void LiveOverlayItem::setTruthPath(const QVector<QPointF>& pix)
{
    prepareGeometryChange();
    m_truthPath = pix;
    m_brect = contentRect();
    update();
}

void LiveOverlayItem::setLivePath(const QVector<QPointF>& pix)
{
    prepareGeometryChange();
    m_livePath = pix;
    m_brect = contentRect();
    update();
}

void LiveOverlayItem::setHits(const QVector<QPointF>& pix)
{
    prepareGeometryChange();
    m_hits = pix;
    m_brect = contentRect();
    update();
}

void LiveOverlayItem::setCarrot(const QPointF& pix, bool valid)
{
    prepareGeometryChange();
    m_carrot = pix;
    m_hasCarrot = valid;
    m_brect = contentRect();
    update();
}

void LiveOverlayItem::setLookahead(double distMeters, double distPixels, bool valid)
{
    prepareGeometryChange();
    m_lookaheadM = distMeters;
    m_lookaheadPx = distPixels;
    m_hasLookahead = valid;
    m_brect = contentRect();
    update();
}

void LiveOverlayItem::setLayerVisibility(bool showHits, bool showCarrot, bool showLookahead)
{
    m_showHits = showHits;
    m_showCarrot = showCarrot;
    m_showLookahead = showLookahead;
    update();
}

void LiveOverlayItem::clearAll()
{
    prepareGeometryChange();
    m_hasVehicle = false;
    m_hasCarrot = false;
    m_hasLookahead = false;
    m_lookaheadM = 0.0;
    m_lookaheadPx = 0.0;
    m_truthPath.clear();
    m_livePath.clear();
    m_hits.clear();
    m_brect = QRectF();
    update();
}

void LiveOverlayItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*)
{
    p->setRenderHint(QPainter::Antialiasing, true);

    // 1. 真值轨迹（灰虚线）
    if (m_truthPath.size() > 1) {
        QPen pen(QColor(140, 140, 140, 200), 0.0, Qt::DashLine);
        p->setPen(pen);
        p->setBrush(Qt::NoBrush);
        p->drawPolyline(m_truthPath.constData(), m_truthPath.size());
    }

    // 2. 仿真实际轨迹（橙色实线）
    if (m_livePath.size() > 1) {
        p->setPen(QPen(QColor(255, 140, 0, 220), 0.0));
        p->drawPolyline(m_livePath.constData(), m_livePath.size());
    }

    // 3. 车身框（按实车尺寸 1.85m × 1.50m 按地图分辨率精确换算，半透明蓝框 + 车头朝向标记）
    if (m_hasVehicle) {
        const double halfL = m_bodyLenPx * 0.5;
        const double halfW = m_bodyWidPx * 0.5;
        const double c = std::cos(m_yawPix), s = std::sin(m_yawPix);
        auto rot = [&](double x, double y) {
            return m_vehicle + QPointF(x * c - y * s, x * s + y * c);
        };
        QPolygonF bodyRect;
        bodyRect << rot(halfL, -halfW)
                 << rot(halfL,  halfW)
                 << rot(-halfL, halfW)
                 << rot(-halfL, -halfW);
        QPen bodyPen(QColor(0, 110, 255, 235), 1.8);
        bodyPen.setCosmetic(true);
        p->setPen(bodyPen);
        p->setBrush(QColor(0, 110, 255, 38));
        p->drawPolygon(bodyRect);

        // 车头前沿加粗边线 + 小前指三角，明确车头方向
        QPen frontPen(QColor(0, 85, 220, 250), 3.0);
        frontPen.setCosmetic(true);
        p->setPen(frontPen);
        p->drawLine(rot(halfL, -halfW), rot(halfL, halfW));
        QPolygonF nose;
        nose << rot(halfL + 4.0, 0.0)
             << rot(halfL - 3.0, -3.5)
             << rot(halfL - 3.0,  3.5);
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(0, 95, 240, 220));
        p->drawPolygon(nose);
    }

    // 4. 三层归因 - 层 3：前视距离（LA）半透明红色长方形引导框（从车头前沿往前伸 LA，宽=车身宽 1.50m）
    if (m_showLookahead && m_hasVehicle && m_hasLookahead && m_lookaheadPx > 0.0) {
        const double halfL = m_bodyLenPx * 0.5;
        const double frontEnd = halfL + m_lookaheadPx;
        const double halfW = m_bodyWidPx * 0.5;
        const double c = std::cos(m_yawPix), s = std::sin(m_yawPix);
        auto rot = [&](double x, double y) {
            return m_vehicle + QPointF(x * c - y * s, x * s + y * c);
        };
        QPolygonF laRect;
        laRect << rot(halfL,    -halfW)
               << rot(frontEnd, -halfW)
               << rot(frontEnd,  halfW)
               << rot(halfL,     halfW);
        QPen laPen(QColor(220, 38, 38, 210), 1.6, Qt::DashLine);
        laPen.setCosmetic(true);
        p->setPen(laPen);
        p->setBrush(QColor(239, 68, 68, 60));
        p->drawPolygon(laRect);

        // 前端 LA 实线边 + 数值标注
        QPen tipPen(QColor(220, 38, 38, 245), 2.2);
        tipPen.setCosmetic(true);
        p->setPen(tipPen);
        p->drawLine(rot(frontEnd, -halfW), rot(frontEnd, halfW));
        p->setFont(QFont("Sans Serif", 8, QFont::Bold));
        p->drawText(rot(frontEnd, -halfW) + QPointF(4, -4),
                    QStringLiteral("LA %1m").arg(m_lookaheadM, 0, 'f', 2));
    }

    // 5. 三层归因 - 层 1：前视命中点（红点 + 白色外圈）
    if (m_showHits && !m_hits.isEmpty()) {
        QPen hPen(QColor(255, 255, 255, 220), 1.2);
        hPen.setCosmetic(true);
        p->setPen(hPen);
        p->setBrush(QColor(235, 45, 45, 235));
        for (const auto& h : m_hits)
            p->drawEllipse(h, 3.0, 3.0);
    }

    // 6. 三层归因 - 层 2：carrot 目标点（绿十字圆标 + 中心点 + 到车虚线，置顶最清晰）
    if (m_showCarrot && m_hasCarrot) {
        if (m_hasVehicle) {
            QPen linkPen(QColor(22, 163, 74, 200), 1.8, Qt::DashLine);
            linkPen.setCosmetic(true);
            p->setPen(linkPen);
            p->drawLine(m_vehicle, m_carrot);
        }
        QPen cPen(QColor(22, 163, 74, 250), 2.2);
        cPen.setCosmetic(true);
        p->setPen(cPen);
        const double s = 5.0;
        p->drawLine(m_carrot + QPointF(-s, 0), m_carrot + QPointF(s, 0));
        p->drawLine(m_carrot + QPointF(0, -s), m_carrot + QPointF(0, s));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(m_carrot, s, s);
        p->setBrush(QColor(22, 163, 74, 240));
        p->drawEllipse(m_carrot, 1.5, 1.5);
    }
}

// ================= LiveLayer =================

LiveLayer::LiveLayer(QObject* parent) : QObject(parent)
{
    m_playTimer = new QTimer(this);
    m_playTimer->setInterval(200);
    connect(m_playTimer, &QTimer::timeout, this, [this]() {
        if (m_timeline.isEmpty() || m_currentFrame + 1 >= m_timeline.size()) {
            m_playTimer->stop();
            if (m_btnPlay) m_btnPlay->setText(QStringLiteral("▶"));
            return;
        }
        m_followLatest = false;
        seekToFrameInternal(m_currentFrame + 1, /*animate=*/true);
    });

    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(16);   // ~60 FPS 平滑插值
    connect(m_animTimer, &QTimer::timeout, this, &LiveLayer::stepAnimation);
}

LiveLayer::~LiveLayer()
{
    disconnectNow();
    destroyOverlay();
}

void LiveLayer::attachScene(BQGraphicsScene* scene)
{
    destroyOverlay();
    m_scene = scene;
    rebuildOverlay();
}

void LiveLayer::setMapInfo(double resolution, double originX, double originY, int pgmHeight)
{
    m_resolution = resolution;
    m_originX = originX;
    m_originY = originY;
    m_pgmHeight = pgmHeight;
    // 必须用 m_overlayGuard 判活：openYaml() 调用前 scene->clear() 可能已析构旧图元
    if (m_overlayGuard && m_resolution > 0.0)
        m_overlayGuard->setVehicleSize(1.85 / m_resolution, 1.50 / m_resolution);
}

void LiveLayer::destroyOverlay()
{
    // 场景 clear() 会 delete item，用 QPointer 判定，避免悬垂（T1.5 教训）
    if (m_overlayGuard) {
        if (m_scene && m_overlayGuard->scene() == m_scene)
            m_scene->removeItem(m_overlayGuard);
        delete m_overlayGuard.data();
    }
    m_overlayGuard = nullptr;
    m_overlay = nullptr;
}

void LiveLayer::rebuildOverlay()
{
    if (!m_scene) return;
    if (!m_overlayGuard) {
        auto* item = new LiveOverlayItem;
        item->setZValue(0.8);
        if (m_resolution > 0.0)
            item->setVehicleSize(1.85 / m_resolution, 1.50 / m_resolution);
        item->setLayerVisibility(m_showHits, m_showCarrot, m_showLookahead);
        m_scene->addItem(item);
        m_overlayGuard = item;
        m_overlay = item;
    }
}

QPointF LiveLayer::worldToPixel(double wx, double wy) const
{
    if (m_resolution <= 0.0) return QPointF();
    const double px = (wx - m_originX) / m_resolution;
    const double py = m_pgmHeight - 1 - (wy - m_originY) / m_resolution;
    return QPointF(px, py);
}

QDockWidget* LiveLayer::createControlDock(QMainWindow* win)
{
    auto* dock = new QDockWidget(QObject::tr("直播跟车"), win);
    dock->setObjectName(QStringLiteral("liveControlDock"));
    auto* body = new QWidget(dock);
    auto* v = new QVBoxLayout(body);
    auto* form = new QFormLayout;

    m_editUrl = new QLineEdit(QStringLiteral("ws://127.0.0.1:9100"));
    m_editRun = new QLineEdit;
    m_editRun->setPlaceholderText(QObject::tr("run_id（提交 run 后获得）"));
    m_editToken = new QLineEdit(QStringLiteral("dev-token"));
    m_editToken->setEchoMode(QLineEdit::Password);

    form->addRow(QObject::tr("Bridge 地址"), m_editUrl);
    form->addRow(QObject::tr("run_id"), m_editRun);
    form->addRow(QObject::tr("Token"), m_editToken);
    v->addLayout(form);

    m_btnConnect = new QPushButton(QObject::tr("连接直播"));
    m_btnConnect->setCheckable(true);
    v->addWidget(m_btnConnect);

    m_lblState = new QLabel(QObject::tr("状态：未连接"));
    m_lblState->setWordWrap(true);
    m_lblStats = new QLabel(QObject::tr("帧：0  命中：0"));
    v->addWidget(m_lblState);
    v->addWidget(m_lblStats);

    // ---- T3.3 三层归因视图 ----
    auto* grpAttr = new QGroupBox(QObject::tr("三层归因视图"));
    auto* vAttr = new QVBoxLayout(grpAttr);

    // 1. 图层开关行
    auto* hChk = new QHBoxLayout;
    m_chkHits = new QCheckBox(QObject::tr("命中点(红)"));
    m_chkCarrot = new QCheckBox(QObject::tr("Carrot(绿)"));
    m_chkLookahead = new QCheckBox(QObject::tr("前视框(红)"));
    m_chkHits->setChecked(m_showHits);
    m_chkCarrot->setChecked(m_showCarrot);
    m_chkLookahead->setChecked(m_showLookahead);
    hChk->addWidget(m_chkHits);
    hChk->addWidget(m_chkCarrot);
    hChk->addWidget(m_chkLookahead);
    vAttr->addLayout(hChk);

    auto applyVis = [this]() {
        m_showHits = m_chkHits ? m_chkHits->isChecked() : true;
        m_showCarrot = m_chkCarrot ? m_chkCarrot->isChecked() : true;
        m_showLookahead = m_chkLookahead ? m_chkLookahead->isChecked() : true;
        if (m_overlayGuard)
            m_overlayGuard->setLayerVisibility(m_showHits, m_showCarrot, m_showLookahead);
    };
    connect(m_chkHits, &QCheckBox::toggled, this, applyVis);
    connect(m_chkCarrot, &QCheckBox::toggled, this, applyVis);
    connect(m_chkLookahead, &QCheckBox::toggled, this, applyVis);

    // 2. 时间轴滑块 + 帧位置
    auto* hTime = new QHBoxLayout;
    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(0, 0);
    m_slider->setEnabled(false);
    m_lblFramePos = new QLabel(QObject::tr("帧 0/0  t=0.00s"));
    hTime->addWidget(m_slider, 1);
    hTime->addWidget(m_lblFramePos);
    vAttr->addLayout(hTime);

    connect(m_slider, &QSlider::valueChanged, this, [this](int val) {
        if (m_playTimer && m_playTimer->isActive()) {
            m_playTimer->stop();
            if (m_btnPlay) m_btnPlay->setText(QStringLiteral("▶"));
        }
        m_followLatest = (val + 1 >= m_timeline.size());
        seekToFrame(val);
    });

    // 3. 播放控制与单帧截图行
    auto* hPlay = new QHBoxLayout;
    m_btnPrev = new QPushButton(QStringLiteral("◀"));
    m_btnPrev->setToolTip(QObject::tr("上一帧"));
    m_btnPlay = new QPushButton(QStringLiteral("▶"));
    m_btnPlay->setToolTip(QObject::tr("播放 / 暂停"));
    m_btnNext = new QPushButton(QStringLiteral(">"));
    m_btnNext->setToolTip(QObject::tr("下一帧"));
    m_btnShot = new QPushButton(QObject::tr("截图"));
    m_btnPrev->setFixedWidth(36);
    m_btnPlay->setFixedWidth(36);
    m_btnNext->setFixedWidth(36);
    hPlay->addWidget(m_btnPrev);
    hPlay->addWidget(m_btnPlay);
    hPlay->addWidget(m_btnNext);
    hPlay->addStretch();
    hPlay->addWidget(m_btnShot);
    vAttr->addLayout(hPlay);

    connect(m_btnPrev, &QPushButton::clicked, this, [this]() {
        if (m_playTimer && m_playTimer->isActive()) {
            m_playTimer->stop();
            if (m_btnPlay) m_btnPlay->setText(QStringLiteral("▶"));
        }
        if (m_currentFrame > 0) {
            m_followLatest = false;
            seekToFrame(m_currentFrame - 1);
        }
    });
    connect(m_btnNext, &QPushButton::clicked, this, [this]() {
        if (m_playTimer && m_playTimer->isActive()) {
            m_playTimer->stop();
            if (m_btnPlay) m_btnPlay->setText(QStringLiteral("▶"));
        }
        if (m_currentFrame + 1 < m_timeline.size()) {
            m_followLatest = (m_currentFrame + 2 >= m_timeline.size());
            seekToFrame(m_currentFrame + 1);
        }
    });
    connect(m_btnPlay, &QPushButton::clicked, this, &LiveLayer::togglePlay);
    connect(m_btnShot, &QPushButton::clicked, this, [this, body]() {
        const QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        const QString rid = m_runId.isEmpty() ? QStringLiteral("live") : m_runId;
        const int idx = (m_currentFrame >= 0) ? m_currentFrame + 1 : 0;
        const QString defPath = (desktop.isEmpty() ? QDir::homePath() : desktop)
                              + QStringLiteral("/frame_%1_f%2.png").arg(rid).arg(idx);
        const QString path = QFileDialog::getSaveFileName(
            body, QObject::tr("导出单帧截图"), defPath, QObject::tr("PNG 图片 (*.png)"));
        if (path.isEmpty()) return;
        const bool ok = exportFrameScreenshot(path);
        if (m_lblAttr) {
            m_lblAttr->setText(ok ? QObject::tr("已导出截图：%1").arg(path)
                                  : QObject::tr("导出截图失败（当前无可用场景）"));
        }
    });

    // 4. 当前帧归因读数
    m_lblAttr = new QLabel(QObject::tr("速度: -  | LA: -\nCarrot: - | 命中: 0"));
    m_lblAttr->setWordWrap(true);
    vAttr->addWidget(m_lblAttr);

    v->addWidget(grpAttr);
    v->addStretch();

    connect(m_btnConnect, &QPushButton::toggled, this, [this](bool on) {
        if (on) startLive(m_editUrl->text().trimmed(), m_editRun->text().trimmed(),
                          m_editToken->text());
        else stopLive();
    });
    connect(this, &LiveLayer::stateChanged, this,
            [this](const QString& st, double prog, const QString& msg) {
        m_state = st;
        QString text = QObject::tr("状态：%1").arg(st);
        if (prog > 0.0) text += QStringLiteral("  %1%").arg(int(prog * 100));
        if (!msg.isEmpty()) text += QStringLiteral("  ") + msg;
        if (m_lblState) m_lblState->setText(text);
    });
    connect(this, &LiveLayer::statsChanged, this, [this](int f, int h) {
        if (m_lblStats)
            m_lblStats->setText(QObject::tr("帧：%1  命中：%2").arg(f).arg(h));
    });

    dock->setWidget(body);
    return dock;
}

bool LiveLayer::connected() const
{
    return m_ws && m_ws->state() == QAbstractSocket::ConnectedState;
}

void LiveLayer::setRunIdAndConnect(const QString& runId)
{
    if (m_editRun)
        m_editRun->setText(runId);
    const QString baseUrl = m_editUrl ? m_editUrl->text().trimmed() : QStringLiteral("ws://127.0.0.1:9100");
    const QString token = m_editToken ? m_editToken->text() : QStringLiteral("dev-token");
    if (m_btnConnect && !m_btnConnect->isChecked()) {
        m_btnConnect->setChecked(true);
    } else {
        startLive(baseUrl, runId, token);
    }
}

void LiveLayer::startLive(const QString& baseUrl, const QString& runId, const QString& token)
{
    disconnectNow();
    if (baseUrl.isEmpty() || runId.isEmpty()) {
        m_lastError = QStringLiteral("地址或 run_id 为空");
        m_state = QStringLiteral("error");       // 不依赖 dock 回调，无 dock 时也要可读
        emit stateChanged(m_state, 0.0, m_lastError);
        uncheckQuietly(m_btnConnect);
        return;
    }
    if (m_resolution <= 0.0) {
        m_lastError = QStringLiteral("未加载地图（请先打开 .pgm 与 .yaml 文件后再连接直播）");
        m_state = QStringLiteral("error");
        emit stateChanged(m_state, 0.0, m_lastError);
        uncheckQuietly(m_btnConnect);
        return;
    }
    m_baseUrl = baseUrl;
    m_runId = runId;
    m_frames = 0;
    m_hitTotal = 0;
    m_lastError.clear();
    m_metricsSummary.clear();
    m_timeline.clear();
    m_recordedPath.clear();
    m_currentFrame = -1;
    m_followLatest = true;
    m_animInit = false;
    if (m_animTimer && m_animTimer->isActive()) m_animTimer->stop();
    if (m_playTimer && m_playTimer->isActive()) m_playTimer->stop();
    if (m_btnPlay) m_btnPlay->setText(QStringLiteral("▶"));
    updateTimelineUi();
    if (m_lblAttr)
        m_lblAttr->setText(QObject::tr("速度: -  | LA: -\nCarrot: - | 命中: 0"));

    rebuildOverlay();
    if (m_overlay) m_overlay->clearAll();

    // ws://host:port → ws://host:port/ws/runs/{runId}/stream
    QString url = baseUrl;
    if (url.endsWith('/')) url.chop(1);
    url += QStringLiteral("/ws/runs/%1/stream").arg(runId);

    m_ws = new QWebSocket;
    connect(m_ws, &QWebSocket::connected, this, &LiveLayer::onConnected);
    connect(m_ws, &QWebSocket::disconnected, this, &LiveLayer::onDisconnected);
    connect(m_ws, &QWebSocket::textMessageReceived, this, &LiveLayer::onTextMessage);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(m_ws, &QWebSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) { onError(m_ws->errorString()); });
#else
    connect(m_ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, [this](QAbstractSocket::SocketError) { onError(m_ws->errorString()); });
#endif

    QNetworkRequest req{QUrl(url)};
    req.setRawHeader("X-Bridge-Token", token.toUtf8());
    m_ws->open(req);
    m_state = QStringLiteral("connecting");
    emit stateChanged(m_state, 0.0, QStringLiteral("连接中…"));
}

void LiveLayer::stopLive()
{
    disconnectNow();
    m_state = QStringLiteral("idle");
    emit stateChanged(m_state, 0.0, QStringLiteral("已断开"));
}

void LiveLayer::disconnectNow()
{
    if (m_ws) {
        m_ws->disconnect(this);
        m_ws->close();
        m_ws->deleteLater();
        m_ws = nullptr;
    }
}

void LiveLayer::onConnected()
{
    // 必须同时更新 m_state：以前只发信号不改状态，
    // state() 永远停在 "connecting"，测试和界面判断都会失效。
    m_state = QStringLiteral("connected");
    m_lastError.clear();
    emit stateChanged(m_state, 0.0, QStringLiteral("已连接"));
}

void LiveLayer::onDisconnected()
{
    // 已因 run 失败或已完成而断开时，保留真实状态与指标摘要，不改写成含糊的"连接断开"
    if (m_state == QStringLiteral("failed") && !m_lastError.isEmpty()) {
        emit stateChanged(m_state, 0.0, m_lastError);
    } else if (m_state == QStringLiteral("done") || !m_metricsSummary.isEmpty()) {
        m_state = QStringLiteral("done");
        emit stateChanged(m_state, 1.0,
                          m_metricsSummary.isEmpty() ? QStringLiteral("已完成") : m_metricsSummary);
    } else {
        m_state = QStringLiteral("disconnected");
        emit stateChanged(m_state, 0.0, QStringLiteral("连接断开"));
    }
    uncheckQuietly(m_btnConnect);
}

void LiveLayer::onError(const QString& err)
{
    // 服务端已明确告知失败或已完成时，不要再被 socket 的 "远端关闭" 盖掉——
    // 否则用户看不到真正的错误（如"缺 container.bag"）或完成态指标摘要。
    if (m_state == QStringLiteral("failed") && !m_lastError.isEmpty()) {
        emit stateChanged(m_state, 0.0, m_lastError);
        return;
    }
    if (m_state == QStringLiteral("done") || !m_metricsSummary.isEmpty()) {
        m_state = QStringLiteral("done");
        emit stateChanged(m_state, 1.0,
                          m_metricsSummary.isEmpty() ? QStringLiteral("已完成") : m_metricsSummary);
        return;
    }
    m_lastError = err;
    emit stateChanged(QStringLiteral("error"), 0.0, err);
}

void LiveLayer::onTextMessage(const QString& msg)
{
    const QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
    if (!doc.isObject()) return;
    const QJsonObject o = doc.object();
    const QString type = o.value(QStringLiteral("type")).toString();

    rebuildOverlay();
    if (!m_overlay) return;

    // 坐标换算未就绪时的诊断提示（m_resolution=0 时 worldToPixel 返回 (0,0)）
    static bool warned = false;
    if (m_resolution <= 0.0 && type == QStringLiteral("frame") && !warned) {
        warned = true;
        qWarning("[LiveLayer] m_resolution=0，坐标换算无效！"
                 "请确认已打开 yaml 文件（setMapInfo 需要 resolution/origin）");
    }

    if (type == QStringLiteral("error")) {
        // bridge 明确告知失败原因（如场地缺 container.bag）。
        // 以前不处理这类消息，用户只看到"点连接直播没响应"。
        const QString msg = o.value(QStringLiteral("msg")).toString();
        const QString code = o.value(QStringLiteral("code")).toString();
        m_lastError = code.isEmpty() ? msg
                                     : QStringLiteral("%1（%2）").arg(msg, code);
        m_state = QStringLiteral("failed");
        emit stateChanged(m_state, 0.0, m_lastError);
        // 静默弹起按钮，防止触发 toggled(false) -> stopLive() 把真实失败原因盖成 "idle 已断开"
        uncheckQuietly(m_btnConnect);
        return;
    }

    if (type == QStringLiteral("status")) {
        const QString st = o.value(QStringLiteral("state")).toString();
        QString dispMsg = o.value(QStringLiteral("msg")).toString();
        double prog = o.value(QStringLiteral("progress")).toDouble();
        // 已拿到 metrics 摘要时，不要被末尾的 "finished" / "replay finished" 覆盖掉行程与均速
        if (st == QStringLiteral("done") && !m_metricsSummary.isEmpty()) {
            dispMsg = m_metricsSummary;
            prog = 1.0;
        }
        m_state = st;
        emit stateChanged(m_state, prog, dispMsg);
        return;
    }

    if (type == QStringLiteral("frame")) {
        LiveFrame f;
        f.t = o.value(QStringLiteral("t")).toDouble();
        f.x = o.value(QStringLiteral("x")).toDouble();
        f.y = o.value(QStringLiteral("y")).toDouble();
        f.yaw = o.value(QStringLiteral("yaw")).toDouble();
        f.speed = o.contains(QStringLiteral("speed"))
                      ? o.value(QStringLiteral("speed")).toDouble()
                      : o.value(QStringLiteral("v")).toDouble();

        QJsonValue laVal = o.value(QStringLiteral("lookahead"));
        if (laVal.isNull() || laVal.isUndefined())
            laVal = o.value(QStringLiteral("lookahead_dist"));
        if (!laVal.isNull() && !laVal.isUndefined())
            f.lookahead = laVal.toDouble();

        if (o.contains(QStringLiteral("carrot")) && !o.value(QStringLiteral("carrot")).isNull()) {
            const QJsonArray c = o.value(QStringLiteral("carrot")).toArray();
            if (c.size() >= 2) {
                f.hasCarrot = true;
                f.carrotWorld = QPointF(c.at(0).toDouble(), c.at(1).toDouble());
            }
        }
        if (o.contains(QStringLiteral("front_hits")) &&
            !o.value(QStringLiteral("front_hits")).isNull()) {
            for (const auto& v : o.value(QStringLiteral("front_hits")).toArray()) {
                const QJsonArray hp = v.toArray();
                if (hp.size() >= 2)
                    f.hitsWorld << QPointF(hp.at(0).toDouble(), hp.at(1).toDouble());
            }
        }
        const QJsonValue fhd = o.value(QStringLiteral("first_hit_dist"));
        if (!fhd.isNull() && !fhd.isUndefined())
            f.firstHitDist = fhd.toDouble();

        ++m_frames;
        m_hitTotal += f.hitsWorld.size();
        m_timeline.append(f);
        m_recordedPath.append(worldToPixel(f.x, f.y));
        updateTimelineUi();

        if (m_followLatest)
            seekToFrameInternal(m_timeline.size() - 1, /*animate=*/m_animInit);

        emit statsChanged(m_frames, m_hitTotal);
        return;
    }

    if (type == QStringLiteral("front_hits")) {
        QVector<QPointF> worldHits;
        for (const auto& v : o.value(QStringLiteral("hits")).toArray()) {
            const QJsonArray hp = v.toArray();
            if (hp.size() >= 2)
                worldHits << QPointF(hp.at(0).toDouble(), hp.at(1).toDouble());
        }
        m_hitTotal += worldHits.size();
        if (!m_timeline.isEmpty()) {
            m_timeline.last().hitsWorld = worldHits;
            const QJsonValue fhd = o.value(QStringLiteral("first_hit_dist"));
            if (!fhd.isNull() && !fhd.isUndefined())
                m_timeline.last().firstHitDist = fhd.toDouble();
        }
        if (m_timeline.isEmpty() || m_currentFrame == m_timeline.size() - 1) {
            QVector<QPointF> pixHits;
            pixHits.reserve(worldHits.size());
            for (const auto& wh : worldHits) pixHits << worldToPixel(wh.x(), wh.y());
            m_overlay->setHits(pixHits);
            if (!m_timeline.isEmpty() && m_currentFrame == m_timeline.size() - 1)
                seekToFrame(m_currentFrame);
        }
        emit statsChanged(m_frames, m_hitTotal);
        return;
    }

    if (type == QStringLiteral("path")) {
        QVector<QPointF> pts;
        for (const auto& v : o.value(QStringLiteral("path")).toArray()) {
            const QJsonArray pp = v.toArray();
            if (pp.size() >= 2)
                pts << worldToPixel(pp.at(0).toDouble(), pp.at(1).toDouble());
        }
        m_overlay->setLivePath(pts);
        return;
    }

    if (type == QStringLiteral("metrics")) {
        // 指标：行程 / 均速 / 偏差 RMS / 命令数 / 综合判定
        const QString gates = o.value(QStringLiteral("gates")).toString();
        auto num = [&](const char* k) {
            const QJsonValue v = o.value(QLatin1String(k));
            return v.isNull() || v.isUndefined() ? QStringLiteral("-")
                                                 : QString::number(v.toDouble(), 'f', 3);
        };
        QString detail = QStringLiteral("行程 %1/%2 m  均速 %3 m/s  偏差RMS %4  命令 %5/%6")
                             .arg(num("travel_m")).arg(num("path_length_m"))
                             .arg(num("mean_speed"))
                             .arg(num("edge_lateral_rms"))
                             .arg(num("cmd_nonzero")).arg(num("cmd_count"));
        m_metricsSummary = gates.isEmpty() ? detail : gates + QStringLiteral("  ") + detail;
        m_state = QStringLiteral("done");
        emit stateChanged(m_state, 1.0, m_metricsSummary);
        return;
    }
}

void LiveLayer::updateTimelineUi()
{
    if (!m_slider) return;
    const QSignalBlocker blocker(m_slider);
    const int n = m_timeline.size();
    m_slider->setEnabled(n > 0);
    m_slider->setRange(0, n > 0 ? n - 1 : 0);
    if (n == 0) {
        m_slider->setValue(0);
        if (m_lblFramePos) m_lblFramePos->setText(QObject::tr("帧 0/0  t=0.00s"));
    }
}

void LiveLayer::applyOverlayPose(double x, double y, double yaw, double la,
                                 bool hasCarrot, const QPointF& carrotWorld)
{
    if (!m_overlayGuard) return;
    m_overlayGuard->setVehicle(worldToPixel(x, y), -yaw, true);
    if (hasCarrot)
        m_overlayGuard->setCarrot(worldToPixel(carrotWorld.x(), carrotWorld.y()), true);
    else
        m_overlayGuard->setCarrot(QPointF(), false);

    const bool hasLa = (la >= 0.0);
    const double laPx = (hasLa && m_resolution > 0.0) ? (la / m_resolution) : 0.0;
    m_overlayGuard->setLookahead(hasLa ? la : 0.0, laPx, hasLa);
}

void LiveLayer::stepAnimation()
{
    if (!m_animInit) {
        if (m_animTimer) m_animTimer->stop();
        return;
    }
    // 60 FPS 指数平滑，约 180ms 追齐 5 Hz 原始帧，消除一卡一卡的顿挫感
    constexpr double kAlpha = 0.22;
    const double dx = m_tgtX - m_dispX;
    const double dy = m_tgtY - m_dispY;
    const double dyaw = std::atan2(std::sin(m_tgtYaw - m_dispYaw),
                                   std::cos(m_tgtYaw - m_dispYaw));

    m_dispX += dx * kAlpha;
    m_dispY += dy * kAlpha;
    m_dispYaw += dyaw * kAlpha;

    double dla = 0.0;
    if (m_tgtLa >= 0.0) {
        if (m_dispLa < 0.0) m_dispLa = m_tgtLa;
        dla = m_tgtLa - m_dispLa;
        m_dispLa += dla * kAlpha;
    } else {
        m_dispLa = -1.0;
    }

    double dcar = 0.0;
    if (m_tgtHasCarrot) {
        if (!m_dispHasCarrot) {
            m_dispCarrot = m_tgtCarrot;
            m_dispHasCarrot = true;
        } else {
            const QPointF dc = m_tgtCarrot - m_dispCarrot;
            dcar = std::hypot(dc.x(), dc.y());
            m_dispCarrot += dc * kAlpha;
        }
    } else {
        m_dispHasCarrot = false;
    }

    const bool settled = (std::hypot(dx, dy) < 0.002 &&
                          std::fabs(dyaw) < 0.002 &&
                          std::fabs(dla) < 0.003 &&
                          dcar < 0.003);
    if (settled) {
        m_dispX = m_tgtX;
        m_dispY = m_tgtY;
        m_dispYaw = m_tgtYaw;
        m_dispLa = m_tgtLa;
        m_dispHasCarrot = m_tgtHasCarrot;
        m_dispCarrot = m_tgtCarrot;
        if (m_animTimer) m_animTimer->stop();
    }
    applyOverlayPose(m_dispX, m_dispY, m_dispYaw, m_dispLa, m_dispHasCarrot, m_dispCarrot);
}

void LiveLayer::seekToFrame(int index)
{
    seekToFrameInternal(index, /*animate=*/false);
}

void LiveLayer::seekToFrameInternal(int index, bool animate)
{
    if (index < 0 || index >= m_timeline.size()) return;
    rebuildOverlay();
    m_currentFrame = index;
    const LiveFrame& f = m_timeline.at(index);

    m_tgtX = f.x;
    m_tgtY = f.y;
    m_tgtYaw = f.yaw;
    m_tgtLa = f.lookahead;
    m_tgtHasCarrot = f.hasCarrot;
    m_tgtCarrot = f.carrotWorld;

    if (m_overlayGuard) {
        if (m_recordedPath.size() >= index + 1)
            m_overlayGuard->setLivePath(m_recordedPath.mid(0, index + 1));

        QVector<QPointF> pixHits;
        pixHits.reserve(f.hitsWorld.size());
        for (const auto& wh : f.hitsWorld) pixHits << worldToPixel(wh.x(), wh.y());
        m_overlayGuard->setHits(pixHits);
    }

    if (animate && m_animInit && m_animTimer) {
        if (!m_animTimer->isActive()) m_animTimer->start();
    } else {
        if (m_animTimer && m_animTimer->isActive()) m_animTimer->stop();
        m_animInit = true;
        m_dispX = m_tgtX;
        m_dispY = m_tgtY;
        m_dispYaw = m_tgtYaw;
        m_dispLa = m_tgtLa;
        m_dispHasCarrot = m_tgtHasCarrot;
        m_dispCarrot = m_tgtCarrot;
        applyOverlayPose(m_dispX, m_dispY, m_dispYaw, m_dispLa, m_dispHasCarrot, m_dispCarrot);
    }

    if (m_slider) {
        const QSignalBlocker blocker(m_slider);
        m_slider->setValue(index);
    }
    if (m_lblFramePos) {
        m_lblFramePos->setText(QStringLiteral("帧 %1/%2  t=%3s")
                                   .arg(index + 1).arg(m_timeline.size()).arg(f.t, 0, 'f', 2));
    }
    if (m_lblAttr) {
        const QString laStr = (f.lookahead >= 0.0)
                                  ? QString::number(f.lookahead, 'f', 2) + QStringLiteral("m")
                                  : QStringLiteral("-");
        const QString carStr = f.hasCarrot
            ? QStringLiteral("(%1, %2)").arg(f.carrotWorld.x(), 0, 'f', 2).arg(f.carrotWorld.y(), 0, 'f', 2)
            : QStringLiteral("-");
        const QString hitStr = (f.firstHitDist >= 0.0)
            ? QStringLiteral("%1个(首%2m)").arg(f.hitsWorld.size()).arg(f.firstHitDist, 0, 'f', 2)
            : QString::number(f.hitsWorld.size());
        m_lblAttr->setText(QStringLiteral("速度: %1 m/s | LA: %2\nCarrot: %3 | 命中: %4")
                               .arg(f.speed, 0, 'f', 2)
                               .arg(laStr, carStr, hitStr));
    }
}

void LiveLayer::togglePlay()
{
    if (!m_playTimer || m_timeline.isEmpty()) return;
    if (m_playTimer->isActive()) {
        m_playTimer->stop();
        if (m_btnPlay) m_btnPlay->setText(QStringLiteral("▶"));
        return;
    }
    if (m_currentFrame < 0 || m_currentFrame + 1 >= m_timeline.size())
        seekToFrame(0);
    m_followLatest = false;
    m_playTimer->start();
    if (m_btnPlay) m_btnPlay->setText(QStringLiteral("⏸"));
}

QString LiveLayer::stateLabelText() const
{
    return m_lblState ? m_lblState->text() : QString();
}

bool LiveLayer::exportFrameScreenshot(const QString& filePath)
{
    if (!m_scene || filePath.isEmpty()) return false;
    // 导出单帧截图前确保画面精确贴合当前帧，不处于插值过渡态
    if (m_animTimer && m_animTimer->isActive() && m_currentFrame >= 0)
        seekToFrameInternal(m_currentFrame, /*animate=*/false);
    QFileInfo fi(filePath);
    fi.dir().mkpath(QStringLiteral("."));

    const auto views = m_scene->views();
    if (!views.isEmpty() && views.first()) {
        const QPixmap pm = views.first()->grab();
        return pm.save(filePath, "PNG");
    }
    QRectF sr = m_scene->sceneRect();
    if (sr.isEmpty()) sr = QRectF(0, 0, 800, 600);
    const QSize imgSize(qBound(100, int(sr.width()), 2048),
                        qBound(100, int(sr.height()), 2048));
    QImage img(imgSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::white);
    QPainter painter(&img);
    m_scene->render(&painter, QRectF(QPointF(0, 0), QSizeF(imgSize)), sr);
    painter.end();
    return img.save(filePath, "PNG");
}
