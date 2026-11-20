// T2.4 MapTool 直播跟车实现
#include "live_layer.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPath>
#include <QDockWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMainWindow>
#include <QWebSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QtMath>

#include "../items/GraphicsType.h"

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
    if (m_hasVehicle) add(QRectF(m_vehicle.x() - 20, m_vehicle.y() - 20, 40, 40));
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

void LiveOverlayItem::clearAll()
{
    prepareGeometryChange();
    m_hasVehicle = false;
    m_hasCarrot = false;
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

    // 3. 前视命中点（红点）
    if (!m_hits.isEmpty()) {
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(230, 40, 40, 220));
        for (const auto& h : m_hits)
            p->drawEllipse(h, 3.0, 3.0);
    }

    // 4. carrot 目标点（绿色十字）
    if (m_hasCarrot) {
        p->setPen(QPen(QColor(30, 180, 60, 240), 0.0));
        const double s = 6.0;
        p->drawLine(m_carrot + QPointF(-s, 0), m_carrot + QPointF(s, 0));
        p->drawLine(m_carrot + QPointF(0, -s), m_carrot + QPointF(0, s));
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(m_carrot, s, s);
    }

    // 5. 车辆图标（蓝色三角，指向车头）——放大便于调试
    if (m_hasVehicle) {
        const double len = 24.0, wid = 14.0;
        const double c = std::cos(m_yawPix), s = std::sin(m_yawPix);
        auto rot = [&](double x, double y) {
            return m_vehicle + QPointF(x * c - y * s, x * s + y * c);
        };
        QPolygonF tri;
        tri << rot(len, 0) << rot(-len * 0.6, wid) << rot(-len * 0.6, -wid);
        p->setPen(QPen(QColor(255, 255, 255, 230), 1.5));
        p->setBrush(QColor(0, 110, 255, 235));
        p->drawPolygon(tri);
    }
}

// ================= LiveLayer =================

LiveLayer::LiveLayer(QObject* parent) : QObject(parent) {}

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
    m_lblStats = new QLabel(QObject::tr("帧：0  命中：0"));
    v->addWidget(m_lblState);
    v->addWidget(m_lblStats);
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
        if (m_btnConnect && m_btnConnect->isChecked()) m_btnConnect->setChecked(false);
        return;
    }
    if (m_resolution <= 0.0) {
        m_lastError = QStringLiteral("未加载地图（请先打开 .pgm 与 .yaml 文件后再连接直播）");
        m_state = QStringLiteral("error");
        emit stateChanged(m_state, 0.0, m_lastError);
        if (m_btnConnect && m_btnConnect->isChecked()) m_btnConnect->setChecked(false);
        return;
    }
    m_baseUrl = baseUrl;
    m_runId = runId;
    m_frames = 0;
    m_hitTotal = 0;
    m_lastError.clear();

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
    // 已因 run 失败而断开时，保留真实失败原因，不要改成含糊的"连接断开"
    if (m_state == QStringLiteral("failed") && !m_lastError.isEmpty()) {
        emit stateChanged(m_state, 0.0, m_lastError);
    } else {
        m_state = QStringLiteral("disconnected");
        emit stateChanged(m_state, 0.0, QStringLiteral("连接断开"));
    }
    if (m_btnConnect && m_btnConnect->isChecked()) m_btnConnect->setChecked(false);
}

void LiveLayer::onError(const QString& err)
{
    // 服务端已明确告知失败原因时，不要再被 socket 的 "远端关闭" 盖掉——
    // 那会让用户看不到真正的错误（如"缺 container.bag"）。
    if (m_state == QStringLiteral("failed") && !m_lastError.isEmpty()) {
        emit stateChanged(m_state, 0.0, m_lastError);
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
        // 让按钮弹起来，用户能再次点击（否则看起来像卡死）
        if (m_btnConnect && m_btnConnect->isChecked()) m_btnConnect->setChecked(false);
        return;
    }

    if (type == QStringLiteral("status")) {
        m_state = o.value(QStringLiteral("state")).toString();
        emit stateChanged(m_state,
                          o.value(QStringLiteral("progress")).toDouble(),
                          o.value(QStringLiteral("msg")).toString());
        return;
    }

    if (type == QStringLiteral("frame")) {
        ++m_frames;
        const double wx = o.value(QStringLiteral("x")).toDouble();
        const double wy = o.value(QStringLiteral("y")).toDouble();
        const double yaw = o.value(QStringLiteral("yaw")).toDouble();
        const QPointF pix = worldToPixel(wx, wy);
        // 像素平面角 = -yaw（屏幕 y 向下）
        m_overlay->setVehicle(pix, -yaw, true);

        // carrot（世界系 → 像素）
        if (o.contains(QStringLiteral("carrot")) && !o.value(QStringLiteral("carrot")).isNull()) {
            const QJsonArray c = o.value(QStringLiteral("carrot")).toArray();
            if (c.size() >= 2) {
                m_overlay->setCarrot(worldToPixel(c.at(0).toDouble(),
                                                  c.at(1).toDouble()), true);
            }
        }
        // front_hits 随帧捎带
        if (o.contains(QStringLiteral("front_hits")) &&
            !o.value(QStringLiteral("front_hits")).isNull()) {
            QVector<QPointF> hits;
            for (const auto& v : o.value(QStringLiteral("front_hits")).toArray()) {
                const QJsonArray hp = v.toArray();
                if (hp.size() >= 2)
                    hits << worldToPixel(hp.at(0).toDouble(), hp.at(1).toDouble());
            }
            m_hitTotal += hits.size();
            m_overlay->setHits(hits);
        }
        emit statsChanged(m_frames, m_hitTotal);
        return;
    }

    if (type == QStringLiteral("front_hits")) {
        QVector<QPointF> hits;
        for (const auto& v : o.value(QStringLiteral("hits")).toArray()) {
            const QJsonArray hp = v.toArray();
            if (hp.size() >= 2)
                hits << worldToPixel(hp.at(0).toDouble(), hp.at(1).toDouble());
        }
        m_hitTotal += hits.size();
        m_overlay->setHits(hits);
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
        // 指标：行程 / 偏差 RMS / 命令数 / 综合判定
        const QString gates = o.value(QStringLiteral("gates")).toString();
        auto num = [&](const char* k) {
            const QJsonValue v = o.value(QLatin1String(k));
            return v.isNull() || v.isUndefined() ? QStringLiteral("-")
                                                 : QString::number(v.toDouble(), 'f', 3);
        };
        QString detail = QStringLiteral("行程 %1/%2 m  偏差RMS %3  命令 %4/%5")
                             .arg(num("travel_m")).arg(num("path_length_m"))
                             .arg(num("edge_lateral_rms"))
                             .arg(num("cmd_nonzero")).arg(num("cmd_count"));
        m_state = QStringLiteral("done");
        emit stateChanged(m_state, 1.0,
                          gates.isEmpty() ? detail : gates + QStringLiteral("  ") + detail);
        return;
    }
}
