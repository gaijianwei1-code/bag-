// T1.5 实现：前视命中区渲染图层
// 坐标换算复用 MapTool 现有语义（与 mainwindow.cpp 的 json 保存/加载一致）：
//   world_x = pixel_x * resolution + origin_x
//   world_y = (pgm_height - 1 - pixel_y) * resolution + origin_y
//   反变换：pixel_x = (wx - origin_x) / resolution
//           pixel_y = pgm_height - 1 - (wy - origin_y) / resolution
// 视锥方向：BPointItem::yaw 保存的是世界坐标 yaw（json 直接回读），
// 像素坐标 y 向下 → 像素平面角 = -yaw。
#include "fov_layer.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QMainWindow>
#include <QTimer>
#include <QtMath>
#include <QDebug>
#include <algorithm>

#include "../items/GraphicsType.h"     // BPathItem / BPointItem / BQGraphicsScene
#include "../../core/grid_projection.h"
#include "../../core/fov_raycast.h"

// ================= FovOverlayItem =================

FovOverlayItem::FovOverlayItem(QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
    // 不参与命中测试：图层纯展示，鼠标事件全部落到下层的轨迹点/地图
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setZValue(-1.0);   // 压在轨迹点/线之下
}

void FovOverlayItem::setEntries(const QVector<Entry>& entries)
{
    m_entries = entries;
    m_brect = unionRect();
    prepareGeometryChange();
}

int FovOverlayItem::dotTotal() const
{
    int n = 0;
    for (const auto& e : m_entries) n += e.dots.size();
    return n;
}

QRectF FovOverlayItem::unionRect() const
{
    QRectF r;
    bool has = false;
    for (const auto& e : m_entries) {
        if (!e.sector.isEmpty()) {
            const QRectF sr = e.sector.boundingRect();
            r = has ? r.united(sr) : sr;
            has = true;
        }
        for (const auto& d : e.dots) {
            const QRectF dr(d.x() - 4, d.y() - 4, 8, 8);
            r = has ? r.united(dr) : dr;
            has = true;
        }
    }
    return r;
}

QRectF FovOverlayItem::boundingRect() const
{
    return m_brect;
}

void FovOverlayItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    for (const auto& e : m_entries) {
        if (!e.sector.isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(e.sectorColor);
            painter->drawPolygon(e.sector);
        }
        painter->setPen(Qt::NoPen);
        painter->setBrush(e.sectorColor.darker(140)); // 命中红点（深色）
        for (const auto& d : e.dots)
            painter->drawEllipse(d, 3.0, 3.0);
    }
}

// ================= FovLayer =================

FovLayer::FovLayer(QObject* parent)
    : QObject(parent)
{
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(30);
    connect(m_debounce, &QTimer::timeout, this, &FovLayer::recalcAll);
}

FovLayer::~FovLayer()
{
    destroyOverlay();
}

void FovLayer::attachScene(BQGraphicsScene* scene)
{
    destroyOverlay();
    m_scene = scene;
    rebuildOverlay();

    // 拖点即时刷新：场景任何几何变化 → 防抖后全轨迹重算（≤100 点 × μs/cast）
    if (m_scene) {
        connect(m_scene, &QGraphicsScene::changed, this, [this]() {
            scheduleRecalc();
        });
    }
}

void FovLayer::destroyOverlay()
{
    if (m_overlay) {
        if (m_scene && m_overlay->scene() == m_scene)
            m_scene->removeItem(m_overlay);
        delete m_overlay;
        m_overlay = nullptr;
    }
}

void FovLayer::setPathList(const QList<BPathItem*>& paths)
{
    m_paths = paths;
    if (m_enabled) scheduleRecalc();
}

void FovLayer::setGrid(const blz::GridProjection* grid)
{
    m_grid = grid;
    if (m_enabled) scheduleRecalc();
}

void FovLayer::setMapInfo(double resolution, double origin_x, double origin_y, int pgm_height)
{
    m_resolution = resolution;
    m_origin_x = origin_x;
    m_origin_y = origin_y;
    m_pgm_height = pgm_height;
}

QDockWidget* FovLayer::createControlDock(QMainWindow* win)
{
    auto* dock = new QDockWidget(QObject::tr("前视快算"), win);
    dock->setObjectName(QStringLiteral("fovControlDock"));
    auto* body = new QWidget(dock);
    auto* form = new QFormLayout(body);
    form->setContentsMargins(8, 8, 8, 8);

    auto mkSpin = [](double lo, double hi, double step, double val) {
        auto* s = new QDoubleSpinBox;
        s->setRange(lo, hi);
        s->setSingleStep(step);
        s->setDecimals(2);
        s->setValue(val);
        return s;
    };
    m_spinFront = mkSpin(0.2, 20.0, 0.5, m_front_dist);
    m_spinHalf  = mkSpin(1.0, 89.0, 5.0, m_half_angle_deg);
    m_spinZmax  = mkSpin(0.0, 30.0, 0.5, m_zmax);

    form->addRow(QObject::tr("前视距离 (m)"), m_spinFront);
    form->addRow(QObject::tr("视锥半角 (°)"), m_spinHalf);
    form->addRow(QObject::tr("高度过滤 zmax (m)"), m_spinZmax);

    m_btnToggle = new QToolButton;
    m_btnToggle->setText(QObject::tr("前视快算：关"));
    m_btnToggle->setCheckable(true);
    m_btnToggle->setChecked(m_enabled);
    form->addRow(m_btnToggle);

    connect(m_spinFront, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FovLayer::setFrontDist);
    connect(m_spinHalf, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FovLayer::setHalfAngleDeg);
    connect(m_spinZmax, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FovLayer::setZmax);
    connect(m_btnToggle, &QToolButton::toggled, this, [this](bool on) {
        setEnabled(on);
        m_btnToggle->setText(on ? QObject::tr("前视快算：开") : QObject::tr("前视快算：关"));
    });

    dock->setWidget(body);
    return dock;
}

void FovLayer::setEnabled(bool on)
{
    m_enabled = on;
    if (!on) {
        m_debounce->stop();
        destroyOverlay();
        m_lastHitCount = 0;
        if (m_scene) m_scene->update();
        return;
    }
    recalcAll();
}

void FovLayer::setFrontDist(double v) { m_front_dist = v; if (m_enabled) scheduleRecalc(); }
void FovLayer::setHalfAngleDeg(double v) { m_half_angle_deg = v; if (m_enabled) scheduleRecalc(); }
void FovLayer::setZmax(double v)
{
    m_zmax = v;
    emit zmaxChanged(v);   // mainwindow 拿去重投影 pcd，然后回调 setGrid + recalcAll
}

void FovLayer::scheduleRecalc()
{
    if (!m_enabled || m_recalculating) return;
    m_debounce->start();
}

void FovLayer::rebuildOverlay()
{
    if (!m_scene) return;
    if (!m_overlay) {
        m_overlay = new FovOverlayItem;
        m_overlay->setZValue(-1.0);
        m_scene->addItem(m_overlay);
    }
}

void FovLayer::recalcAll()
{
    if (!m_enabled || !m_scene) return;

    // 防止重算 → scene changed → scheduleRecalc 回环
    m_recalculating = true;
    m_debounce->stop();

    rebuildOverlay();

    QVector<FovOverlayItem::Entry> entries;
    const double rad = m_half_angle_deg * M_PI / 180.0;
    int hits = 0;

    for (BPathItem* path : m_paths) {
        if (!path) continue;
        const int n = path->pointsNumber();
        for (int i = 0; i < n; ++i) {
            BPointItem* pt = path->getElement(i);
            if (!pt) continue;
            const QPointF pix = pt->getPoint();
            const qreal yaw = pt->yaw();

            FovOverlayItem::Entry e;
            // 像素坐标视锥扇形（屏幕角 = -yaw）
            QPolygonF sector;
            sector << pix;
            const qreal a0 = -yaw - rad, a1 = -yaw + rad;
            const qreal reach = (m_resolution > 0) ? m_front_dist / m_resolution : m_front_dist;
            for (int k = 0; k <= 16; ++k) {
                const qreal a = a0 + (a1 - a0) * k / 16.0;
                sector << pix + QPointF(reach * std::cos(a), reach * std::sin(a));
            }
            e.sector = sector;

            if (m_grid && m_grid->width > 0 && m_resolution > 0) {
                // 像素 → 世界
                const double wx = pix.x() * m_resolution + m_origin_x;
                const double wy = (m_pgm_height - 1 - pix.y()) * m_resolution + m_origin_y;

                blz::FovParams fp;
                fp.front_dist = m_front_dist;
                fp.half_angle = rad;
                fp.hit_dist_max = std::max(m_front_dist, 3.0);
                auto res = blz::cast_fov(*m_grid, wx, wy, yaw, fp);

                if (res.hit) {
                    e.sectorColor = QColor(30, 144, 255, 46);  // 命中：半透明蓝视锥
                    ++hits;
                    for (const auto& h : res.hits) {
                        // 命中点世界 → 像素
                        const double hx = (h.x - m_origin_x) / m_resolution;
                        const double hy = m_pgm_height - 1 - (h.y - m_origin_y) / m_resolution;
                        e.dots << QPointF(hx, hy);
                    }
                } else {
                    e.sectorColor = QColor(200, 200, 200, 26); // 无命中淡灰
                }
            } else {
                e.sectorColor = QColor(200, 200, 200, 26);     // 无 pcd：淡灰占位
            }
            entries.push_back(e);
        }
    }

    if (m_overlay) m_overlay->setEntries(entries);
    m_lastHitCount = hits;
    m_recalculating = false;
    emit recalculated(hits);
}
