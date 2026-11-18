#ifndef FOV_LAYER_H
#define FOV_LAYER_H

// T1.5: 前视命中区渲染图层（集成 T1.3 grid_projection + T1.4 fov_raycast）
// 纯 UI 图层：挂接 MapTool 的 BQGraphicsScene，沿轨迹点画视锥扇形与命中区。
// 对 mainwindow 的改动只做"接线"（成员 + 菜单/工具栏入口 + 信号槽）。

#include <QObject>
#include <QGraphicsItemGroup>
#include <QList>
#include <QColor>

// 前向声明，避免拖入重型头文件
class BQGraphicsScene;
class BPathItem;
class QDockWidget;
class QDoubleSpinBox;
class QToolButton;
class QMainWindow;

namespace blz { struct GridProjection; }

class FovLayer : public QObject
{
    Q_OBJECT
public:
    explicit FovLayer(QObject* parent = nullptr);
    ~FovLayer() override;

    // ---- 接线接口（mainwindow 调用）----
    // 挂接到场景（item 全部 add 到该 scene；重复调用会先 detach）
    void attachScene(BQGraphicsScene* scene);
    // 数据源：单条轨迹（当前编辑轨迹 m_path）
    void setPath(BPathItem* path);
    // 数据源：多条轨迹（m_path_list），内部合并遍历
    void setPathList(const QList<BPathItem*>& paths);
    // pcd 占据网格注入（已 project_to_grid），layer 不持有所有权
    void setGrid(const blz::GridProjection* grid);
    // 世界坐标 ← 场景(像素)坐标换算所需的地图参数（与 mainwindow 的 yaml 语义一致）
    void setMapInfo(double resolution, double origin_x, double origin_y, int pgm_height);
    // 控制面板（dock 内容 QWidget），参数改动即时触发 recalcAll()
    QDockWidget* createControlDock(QMainWindow* win);

    // 状态
    bool enabled() const { return m_enabled; }
    int itemCount() const { return m_group ? m_group->childItems().size() : 0; }
    int hitCount() const { return m_lastHitCount; }

public slots:
    // "前视快算"开关（工具栏按钮 / menu action toggled）
    void setEnabled(bool on);
    // 全轨迹重算（毫秒级，≤100 点 × μs/cast）
    void recalcAll();
    // 参数槽（spinbox valueChanged 接入）
    void setFrontDist(double v);
    void setHalfAngleDeg(double v);
    void setZmax(double v);

signals:
    // 重算完成后发出（冒烟测试断言用）
    void recalculated(int hitCount);

private:
    void detachItems();
    void clearPathRefs();

    BQGraphicsScene* m_scene = nullptr;
    QList<BPathItem*> m_paths;
    const blz::GridProjection* m_grid = nullptr;

    // 地图参数（像素 → 世界米）
    double m_resolution = 0;
    double m_origin_x = 0, m_origin_y = 0;
    int m_pgm_height = 0;

    // FOV 参数
    double m_front_dist = 2.0;      // 米
    double m_half_angle_deg = 30.0; // 度
    double m_zmax = 2.0;            // 米（高度过滤上限，重投影用）
    bool m_enabled = false;
    int m_lastHitCount = 0;

    // 渲染
    QGraphicsItemGroup* m_group = nullptr; // 所有 FOV item 的容器，z 值压在轨迹下面
    QColor m_sectorColor;    // 视锥扇形（半透明）
    QColor m_hitColor;       // 命中区（红）
    QColor m_freeColor;      // 无命中的淡色

    // 控制面板控件（不持有所有权，dock 持有）
    QDoubleSpinBox* m_spinFront = nullptr;
    QDoubleSpinBox* m_spinHalf = nullptr;
    QDoubleSpinBox* m_spinZmax = nullptr;
    QToolButton* m_btnToggle = nullptr;
};

#endif // FOV_LAYER_H
