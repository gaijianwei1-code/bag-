#ifndef FOV_LAYER_H
#define FOV_LAYER_H

// T1.5: 前视命中区渲染图层（集成 T1.3 grid_projection + T1.4 fov_raycast）
// 渲染用单个非交互 QGraphicsItem（FovOverlayItem），不产生大量可点选 item，
// 不干扰 MapTool 原有的选点/拖拽/框选交互。
// 对 mainwindow 的改动只做"接线"（成员 + 菜单入口 + 信号槽 + 几处 refresh 调用）。

#include <QObject>
#include <QGraphicsObject>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QColor>
#include <QVector>
#include <QPolygonF>
#include <QPointer>

class BQGraphicsScene;
class BPathItem;
class QDockWidget;
class QDoubleSpinBox;
class QToolButton;
class QMainWindow;
class QTimer;

namespace blz { struct GridProjection; }

// ---------- 非交互覆盖 item：一次性画出所有视锥扇形与命中点 ----------
// 用 QGraphicsObject（= QObject + QGraphicsItem）以便被 QPointer 安全守卫：
// 场景 clear() 会 delete 本 item，QPointer 自动置空，避免悬垂指针访问崩溃。
class FovOverlayItem : public QGraphicsObject
{
    Q_OBJECT
public:
    struct Entry {
        QPolygonF sector;        // 像素坐标视锥扇形（含顶点）
        QColor sectorColor;
        QVector<QPointF> dots;   // 命中点（像素坐标）
    };

    explicit FovOverlayItem(QGraphicsItem* parent = nullptr);

    void setEntries(const QVector<Entry>& entries);
    int entryCount() const { return m_entries.size(); }
    int dotTotal() const;
    QRectF unionRect() const;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override { event->ignore(); }
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override { event->ignore(); }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override { event->ignore(); }

private:
    QVector<Entry> m_entries;
    QRectF m_brect;
};

// ---------- 图层控制器 ----------
class FovLayer : public QObject
{
    Q_OBJECT
public:
    explicit FovLayer(QObject* parent = nullptr);
    ~FovLayer() override;

    // ---- 接线接口（mainwindow 调用）----
    void attachScene(BQGraphicsScene* scene);
    void setPathList(const QList<BPathItem*>& paths);   // 数据源（含当前编辑轨迹）
    void setGrid(const blz::GridProjection* grid);      // pcd 占据网格（不持有所有权）
    void setMapInfo(double resolution, double origin_x, double origin_y, int pgm_height);
    QDockWidget* createControlDock(QMainWindow* win);

    // 状态（冒烟断言用）
    bool enabled() const { return m_enabled; }
    int entryCount() const { return m_overlayGuard ? m_overlayGuard->entryCount() : 0; }
    int hitCount() const { return m_lastHitCount; }
    double zmax() const { return m_zmax; }
    // 冒烟用：读取覆盖层当前 z 值（无覆盖层返回 -999）
    double overlayZValue() const { return m_overlayGuard ? m_overlayGuard->zValue() : -999.0; }

public slots:
    void setEnabled(bool on);   // "前视快算"开关
    void recalcAll();           // 全轨迹重算（毫秒级）
    void setFrontDist(double v);
    void setHalfAngleDeg(double v);
    void setZmax(double v);     // 仅存值 + 发 zmaxChanged（重投影由 mainwindow 做）

signals:
    void recalculated(int hitCount);   // 重算完成（冒烟断言用）
    void zmaxChanged(double v);        // mainwindow 收到后重投影 pcd 再触发 recalcAll

private:
    void destroyOverlay();
    void scheduleRecalc();      // 场景 changed 防抖（拖点即时刷新）
    void rebuildOverlay();

    BQGraphicsScene* m_scene = nullptr;
    QList<BPathItem*> m_paths;
    const blz::GridProjection* m_grid = nullptr;

    // 地图参数（像素 ↔ 世界米，与 mainwindow yaml/json 语义一致）
    double m_resolution = 0;
    double m_origin_x = 0, m_origin_y = 0;
    int m_pgm_height = 0;

    // FOV 参数
    double m_front_dist = 2.0;
    double m_half_angle_deg = 30.0;
    double m_zmax = 2.0;
    bool m_enabled = false;
    int m_lastHitCount = 0;

    // 渲染
    FovOverlayItem* m_overlay = nullptr;   // 注意：场景 clear() 会 delete 它，用前必须查 QPointer
    QPointer<FovOverlayItem> m_overlayGuard; // 场景销毁时自动置空，防悬垂

    // 拖点防抖
    QTimer* m_debounce = nullptr;
    bool m_recalculating = false;

    // 控制面板控件（dock 持有所有权）
    QDoubleSpinBox* m_spinFront = nullptr;
    QDoubleSpinBox* m_spinHalf = nullptr;
    QDoubleSpinBox* m_spinZmax = nullptr;
    QToolButton* m_btnToggle = nullptr;
};

#endif // FOV_LAYER_H
