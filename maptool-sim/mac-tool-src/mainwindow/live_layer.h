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
class QCheckBox;
class QSlider;
class QTimer;

// ---------- 单帧时间轴数据结构（T3.3 边收边存与逐帧回放） ----------
struct LiveFrame {
    double t = 0.0;
    double x = 0.0;
    double y = 0.0;
    double yaw = 0.0;
    double speed = 0.0;
    double lookahead = -1.0;        // < 0 表示缺失
    bool hasCarrot = false;
    QPointF carrotWorld;
    QVector<QPointF> hitsWorld;
    double firstHitDist = -1.0;     // < 0 表示无命中
};

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
    void setLookahead(double distMeters, double distPixels, bool valid);
    void setLayerVisibility(bool showHits, bool showCarrot, bool showLookahead);
    void clearAll();

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    int hitCount() const { return m_hits.size(); }
    bool hasVehicle() const { return m_hasVehicle; }
    bool hasLookahead() const { return m_hasLookahead; }
    bool hasCarrot() const { return m_hasCarrot; }

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
    double m_lookaheadM = 0.0;
    double m_lookaheadPx = 0.0;
    bool m_hasLookahead = false;
    bool m_showHits = true;
    bool m_showCarrot = true;
    bool m_showLookahead = true;
    QRectF m_brect;
};

// ---------- 直播控制器（WS 客户端 + 坐标换算 + T3.3 三层归因视图） ----------
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
    int timelineCount() const { return m_timeline.size(); }
    int currentFrameIndex() const { return m_currentFrame; }
    QString lastError() const { return m_lastError; }
    QString metricsSummary() const { return m_metricsSummary; }
    QString stateLabelText() const;
    bool overlayAlive() const { return m_overlayGuard != nullptr; }
    bool overlayHasVehicle() const { return m_overlay && m_overlay->hasVehicle(); }
    bool overlayHasLookahead() const { return m_overlay && m_overlay->hasLookahead(); }
    bool overlayHasCarrot() const { return m_overlay && m_overlay->hasCarrot(); }
    double overlayZ() const { return m_overlayGuard ? m_overlayGuard->zValue() : -999.0; }
    QString state() const { return m_state; }
    bool exportFrameScreenshot(const QString& filePath);

public slots:
    void startLive(const QString& baseUrl, const QString& runId, const QString& token);
    void stopLive();
    void setRunIdAndConnect(const QString& runId);
    void seekToFrame(int index);
    void onTextMessage(const QString& msg);

signals:
    void stateChanged(const QString& state, double progress, const QString& msg);
    void statsChanged(int frames, int hits);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(const QString& err);

private:
    void rebuildOverlay();
    void destroyOverlay();
    void updateTimelineUi();
    void togglePlay();
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
    QString m_metricsSummary;

    // T3.3 时间轴缓冲与三层控制
    QVector<LiveFrame> m_timeline;
    QVector<QPointF> m_recordedPath;
    int m_currentFrame = -1;
    bool m_followLatest = true;
    bool m_showHits = true;
    bool m_showCarrot = true;
    bool m_showLookahead = true;
    QTimer* m_playTimer = nullptr;

    // 控件
    QLineEdit* m_editUrl = nullptr;
    QLineEdit* m_editRun = nullptr;
    QLineEdit* m_editToken = nullptr;
    QPushButton* m_btnConnect = nullptr;
    QLabel* m_lblState = nullptr;
    QLabel* m_lblStats = nullptr;
    QCheckBox* m_chkHits = nullptr;
    QCheckBox* m_chkCarrot = nullptr;
    QCheckBox* m_chkLookahead = nullptr;
    QSlider* m_slider = nullptr;
    QLabel* m_lblFramePos = nullptr;
    QLabel* m_lblAttr = nullptr;
    QPushButton* m_btnPrev = nullptr;
    QPushButton* m_btnPlay = nullptr;
    QPushButton* m_btnNext = nullptr;
    QPushButton* m_btnShot = nullptr;
};

#endif // LIVE_LAYER_H
