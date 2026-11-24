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

// T3.4：PlannerLimits 值成员（fov_layer 按点取检测距离），需要完整定义
#include "../../core/planner_limits.h"

class BQGraphicsScene;
class BPathItem;
class QDockWidget;
class QDoubleSpinBox;
class QComboBox;
class QLabel;
class QToolButton;
class QCheckBox;
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
        QPolygonF sector;        // 像素坐标检测区（实车矩形或视锥扇形）
        QColor sectorColor;
        QColor borderColor;      // 危险点边框色（透明表示无边框）
        QString label;           // 危险点距离标注（如 "0.35m"）
        QPointF labelPos;
        bool isHit = false;
        QVector<QPointF> dots;   // 命中点（像素坐标）
        // T3.4：该点用的场景档位（用于图上区分直线/弯道/掉头），
        // sceneTag 为空表示未启用分场景模式（旧行为，不标注）。
        QString sceneTag;        // "直" / "弯" / "弯·不绕行" / "掉头"
        QColor sceneColor;       // 该档位配色（画检测框边框用）
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

    enum ObstacleSource { SourcePgm = 0, SourcePcd = 1, SourcePgmAndPcd = 2 };

    // ---- 接线接口（mainwindow 调用）----
    void attachScene(BQGraphicsScene* scene);
    void setPathList(const QList<BPathItem*>& paths);   // 数据源（含当前编辑轨迹）
    void setGrid(const blz::GridProjection* grid);      // pcd 占据网格（不持有所有权）
    void setPgmGrid(const blz::GridProjection* grid);   // 修过的 pgm 占据网格（不持有所有权）
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
    void setRectMode(bool on);  // true=实车1.85×1.50m矩形检测，false=旧版扇形视锥
    void setSafetyMargin(double v); // 实车矩形单侧安全裕度（对应 costmap 膨胀半径，默认 0.25m）
    void setObstacleSource(int src); // 0=修过的地图(.pgm), 1=原始点云(.pcd), 2=PGM∩PCD交集
    // T3.4：前视距离改为与 PursuitLocalPlanner 同口径（0.4 × collision_time 分场景）。
    // 打开后 m_front_dist 退化为"未匹配场景时的兜底值"，实际每个点按自身属性取距离。
    void setPlannerMode(bool on);
    void setPlannerLimits(const blz::PlannerLimits& lim);
    const blz::PlannerLimits& plannerLimits() const { return m_limits; }
    bool plannerMode() const { return m_plannerMode; }
    // 冒烟用：某场景当前生效的检测距离（米）
    double frontDistForScene(blz::DriveScene s) const;
    // 冒烟用：上一轮重算各档点数（索引同 DriveScene 枚举）
    int scenePointCount(blz::DriveScene s) const {
        return m_sceneCount[static_cast<int>(s)];
    }
    QString sceneBreakdownText() const;   // "直线 3092点(1.20m) · 掉头 155点(0.40m)"

signals:
    void recalculated(int hitCount);   // 重算完成（冒烟断言用）
    void zmaxChanged(double v);        // mainwindow 收到后重投影 pcd 再触发 recalcAll

private:
    void destroyOverlay();
    void scheduleRecalc();      // 场景 changed 防抖（拖点即时刷新）
    void rebuildOverlay();
    QString plannerHintText() const;

    BQGraphicsScene* m_scene = nullptr;
    QList<BPathItem*> m_paths;
    const blz::GridProjection* m_grid = nullptr;
    const blz::GridProjection* m_pgmGrid = nullptr;

    // 地图参数（像素 ↔ 世界米，与 mainwindow yaml/json 语义一致）
    double m_resolution = 0;
    double m_origin_x = 0, m_origin_y = 0;
    int m_pgm_height = 0;

    // FOV 参数
    double m_front_dist = 2.0;
    double m_half_angle_deg = 30.0;
    double m_zmax = 2.0;
    double m_margin = 0.25;     // 实车矩形单侧安全裕度（对应 costmap 膨胀，默认 0.25m）
    // T3.4：planner 同口径模式。默认开启——前视距离本来就该跟随轨迹点属性；
    // 关掉才回到"全程固定值"的旧行为（保留给需要排除属性影响的排查场景）。
    bool m_plannerMode = true;
    blz::PlannerLimits m_limits;
    int m_sceneCount[4] = {0, 0, 0, 0};   // 按 DriveScene 索引的点数统计
    int m_obstacleSource = SourcePgm; // 默认优先用修过的地图 (.pgm)，无 pgm 时自动退回 .pcd
    bool m_rectMode = true;     // 默认实车矩形净空检测（1.85×1.50m + 前视框）
    bool m_enabled = false;
    int m_lastHitCount = 0;

    // 渲染
    FovOverlayItem* m_overlay = nullptr;   // 注意：场景 clear() 会 delete 它，用前必须查 QPointer
    QPointer<FovOverlayItem> m_overlayGuard; // 场景销毁时自动置空，防悬垂

    // 拖点防抖
    QTimer* m_debounce = nullptr;
    bool m_recalculating = false;

    // 控制面板控件（dock 持有所有权）
    QComboBox* m_comboSource = nullptr;
    QComboBox* m_comboMode = nullptr;
    QDoubleSpinBox* m_spinFront = nullptr;
    QCheckBox* m_chkPlanner = nullptr;      // T3.4 按 planner 场景取距离
    QLabel* m_lblPlannerHint = nullptr;
    QDoubleSpinBox* m_spinMargin = nullptr;
    QDoubleSpinBox* m_spinHalf = nullptr;
    QDoubleSpinBox* m_spinZmax = nullptr;
    QToolButton* m_btnToggle = nullptr;
    QLabel* m_lblSummary = nullptr;
};

#endif // FOV_LAYER_H
