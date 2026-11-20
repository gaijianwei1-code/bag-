#ifndef LIVE_LAYER_H
#define LIVE_LAYER_H

// T2.4 MapTool 直播跟车：连接 sim-bridge 的 WS，实时渲染车辆位置、
// 真值轨迹、前视命中点、carrot 目标点。
//
// 设计要点（沿用 T1.5 的教训）：
//  · 用单个 QGraphicsObject 承载所有直播图元，场景 clear() 后靠 QPointer 重建
//  · z 值分层明确：地图(0) < 前视(0.5) < 直播(0.8) < 轨迹点(默认) < 选区(1000)
//  · 不接受鼠标事件，不干扰原有编辑交互

#include <QObject>
#include <QGraphicsObject>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QPointer>
#include <QString>

class BQGraphicsScene;
class QWebSocket;
class QDockWidget;
class QMainWindow;
class QLabel;
class QPushButton;
class QLineEdit;

// ---------- 直播图元 ----------
class LiveOverlayItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit LiveOverlayItem(QGraphicsItem* parent = nullptr);

    void setVehicle(const QPointF& px, double yawPix, bool valid);
    void setTruthPath(const QVector<QPointF>& pix);
    void setLivePath(const QVector<QPointF>& pix);
    void setHits(const QVector<QPointF>& pix);
    void setCarrot(const QPointF& pix, bool valid);
    void clearAll();

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    int hitCount() const { return m_hits.size(); }
    bool hasVehicle() const { return m_hasVehicle; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*) override {}
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override {}
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override {}

private:
    QRectF contentRect() const;

    QPointF m_vehicle;
    double m_yawPix = 0.0;
    bool m_hasVehicle = false;
    QVector<QPointF> m_truthPath;
    QVector<QPointF> m_livePath;
    QVector<QPointF> m_hits;
    QPointF m_carrot;
    bool m_hasCarrot = false;
    QRectF m_brect;
};

// ---------- 直播控制器（WS 客户端 + 坐标换算 + 状态栏） ----------
class LiveLayer : public QObject
{
    Q_OBJECT
public:
    explicit LiveLayer(QObject* parent = nullptr);
    ~LiveLayer() override;

    void attachScene(BQGraphicsScene* scene);
    // 地图参数（同 FovLayer 语义）：像素 ↔ 世界
    void setMapInfo(double resolution, double originX, double originY, int pgmHeight);
    QDockWidget* createControlDock(QMainWindow* win);

    // 连接/断开
    bool connectTo(const QString& baseUrl, const QString& runId, const QString& token);
    void disconnectNow();
    bool connected() const;

    // 状态查询（冒烟断言用）
    int frameCount() const { return m_frames; }
    int hitTotal() const { return m_hitTotal; }
    QString lastError() const { return m_lastError; }
    bool overlayAlive() const { return m_overlayGuard != nullptr; }
    bool overlayHasVehicle() const { return m_overlay && m_overlay->hasVehicle(); }
    double overlayZ() const { return m_overlayGuard ? m_overlayGuard->zValue() : -999.0; }
    QString state() const { return m_state; }

public slots:
    void startLive(const QString& baseUrl, const QString& runId, const QString& token);
    void stopLive();
    void setRunIdAndConnect(const QString& runId);

signals:
    void stateChanged(const QString& state, double progress, const QString& msg);
    void statsChanged(int frames, int hits);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessage(const QString& msg);
    void onError(const QString& err);

private:
    void rebuildOverlay();
    void destroyOverlay();
    QPointF worldToPixel(double wx, double wy) const;

    BQGraphicsScene* m_scene = nullptr;
    LiveOverlayItem* m_overlay = nullptr;
    QPointer<LiveOverlayItem> m_overlayGuard;

    QWebSocket* m_ws = nullptr;
    QString m_baseUrl;
    QString m_runId;

    double m_resolution = 0.0;
    double m_originX = 0.0;
    double m_originY = 0.0;
    int m_pgmHeight = 0;

    int m_frames = 0;
    int m_hitTotal = 0;
    QString m_state = QStringLiteral("idle");
    QString m_lastError;

    // 控件
    QLineEdit* m_editUrl = nullptr;
    QLineEdit* m_editRun = nullptr;
    QLineEdit* m_editToken = nullptr;
    QPushButton* m_btnConnect = nullptr;
    QLabel* m_lblState = nullptr;
    QLabel* m_lblStats = nullptr;
};

#endif // LIVE_LAYER_H
