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
#include <QComboBox>
#include <QLabel>
#include <QToolButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QMainWindow>
#include <QTimer>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QtMath>
#include <QDebug>
#include <algorithm>

#include "../items/GraphicsType.h"     // BPathItem / BPointItem / BQGraphicsScene
#include "../../core/grid_projection.h"
#include "../../core/fov_raycast.h"
#include "../../core/planner_limits.h"

// ================= FovOverlayItem =================

FovOverlayItem::FovOverlayItem(QGraphicsItem* parent)
    : QGraphicsObject(parent)
{
    // 不参与命中测试：图层纯展示，鼠标事件全部落到下层的轨迹点/地图
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptHoverEvents(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    // z=0.5：必须高于地图 pixmap（默认 0），否则扇形被地图盖住看不见；
    // 又低于轨迹线与轨迹点（默认 0 但后添加者在上，且轨迹点带 pen 绘制），
    // 实测轨迹点/线仍可见且可拖拽（本 item 不接受鼠标事件）。
    setZValue(0.5);
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
    // 先画安全区（极淡不糊图），再画危险区与红点（置顶突出）
    for (int pass = 0; pass < 2; ++pass) {
        const bool drawHit = (pass == 1);
        for (const auto& e : m_entries) {
            if (e.isHit != drawHit) continue;
            if (!e.sector.isEmpty()) {
                if (e.borderColor.alpha() > 0) {
                    QPen bp(e.borderColor, drawHit ? 1.6 : 0.8);
                    bp.setCosmetic(true);
                    painter->setPen(bp);
                } else {
                    painter->setPen(Qt::NoPen);
                }
                painter->setBrush(e.sectorColor);
                painter->drawPolygon(e.sector);
            }
            if (drawHit && !e.dots.isEmpty()) {
                QPen dp(QColor(255, 255, 255, 220), 1.0);
                dp.setCosmetic(true);
                painter->setPen(dp);
                painter->setBrush(QColor(235, 45, 45, 235));
                for (const auto& d : e.dots)
                    painter->drawEllipse(d, 3.0, 3.0);
            }
            if (drawHit && !e.label.isEmpty()) {
                painter->setPen(QColor(200, 25, 25, 245));
                painter->setFont(QFont("Sans Serif", 8, QFont::Bold));
                painter->drawText(e.labelPos, e.label);
            }
        }
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
    // 用 QPointer 判定：场景 clear() 会 delete item，此时 m_overlay 已悬垂，
    // 不能再 removeItem/delete
    if (m_overlayGuard) {
        if (m_scene && m_overlayGuard->scene() == m_scene)
            m_scene->removeItem(m_overlayGuard);
        delete m_overlayGuard.data();
    }
    m_overlayGuard = nullptr;
    m_overlay = nullptr;
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

void FovLayer::setPgmGrid(const blz::GridProjection* grid)
{
    m_pgmGrid = grid;
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
    m_comboSource = new QComboBox;
    m_comboSource->addItem(QObject::tr("修过的地图 (.pgm)"), SourcePgm);
    m_comboSource->addItem(QObject::tr("原始点云 (.pcd)"), SourcePcd);
    m_comboSource->addItem(QObject::tr("PGM ∩ PCD (交集)"), SourcePgmAndPcd);
    m_comboSource->setCurrentIndex(m_obstacleSource);

    m_comboMode = new QComboBox;
    m_comboMode->addItem(QObject::tr("实车矩形 (1.85×1.50m)"), true);
    m_comboMode->addItem(QObject::tr("扇形视锥"), false);
    m_comboMode->setCurrentIndex(m_rectMode ? 0 : 1);

    m_spinFront  = mkSpin(0.2, 20.0, 0.5, m_front_dist);
    m_spinMargin = mkSpin(0.0, 1.50, 0.05, m_margin);
    m_spinMargin->setEnabled(m_rectMode);
    m_spinHalf   = mkSpin(1.0, 89.0, 5.0, m_half_angle_deg);
    m_spinHalf->setEnabled(!m_rectMode);
    m_spinZmax   = mkSpin(0.0, 30.0, 0.5, m_zmax);
    form->addRow(QObject::tr("障碍源"), m_comboSource);
    form->addRow(QObject::tr("检测模式"), m_comboMode);

    // T3.4：前视距离来源切换
    m_chkPlanner = new QCheckBox(QObject::tr("按 planner 分场景（0.4×collision_time）"));
    m_chkPlanner->setToolTip(QObject::tr(
        "开启后前视检测距离与 PursuitLocalPlanner 的碰撞检测完全同口径：\n"
        "距离 = 0.4 m/s × collision_time_*，按轨迹点的 curve_paths / turnAround /\n"
        "bypass 属性分四档。此时「前视距离」输入框仅作兜底。\n"
        "注意 bypass=1 在工程里表示「不允许绕行」。"));
    form->addRow(QString(), m_chkPlanner);
    m_lblPlannerHint = new QLabel;
    m_lblPlannerHint->setStyleSheet("color:#6b7280; font-size:11px;");
    m_lblPlannerHint->setWordWrap(true);
    m_lblPlannerHint->setVisible(false);
    form->addRow(QString(), m_lblPlannerHint);

    form->addRow(QObject::tr("前视距离 (m)"), m_spinFront);
    form->addRow(QObject::tr("安全裕度 (m)"), m_spinMargin);
    form->addRow(QObject::tr("视锥半角 (°)"), m_spinHalf);
    form->addRow(QObject::tr("高度过滤 zmax (m)"), m_spinZmax);

    m_btnToggle = new QToolButton;
    m_btnToggle->setText(QObject::tr("前视快算：关"));
    m_btnToggle->setCheckable(true);
    m_btnToggle->setChecked(m_enabled);
    form->addRow(m_btnToggle);

    m_lblSummary = new QLabel(QObject::tr("未开启（打开地图/.pcd 后开启即可净空排雷）"));
    m_lblSummary->setWordWrap(true);
    m_lblSummary->setStyleSheet("color: #6b7280; font-size: 12px;");
    form->addRow(m_lblSummary);

    connect(m_comboSource, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        setObstacleSource(m_comboSource->itemData(idx).toInt());
    });

    connect(m_comboMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        const bool rect = m_comboMode->itemData(idx).toBool();
        setRectMode(rect);
    });

    connect(m_spinFront, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FovLayer::setFrontDist);
    connect(m_spinMargin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FovLayer::setSafetyMargin);
    connect(m_spinHalf, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FovLayer::setHalfAngleDeg);
    connect(m_spinZmax, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FovLayer::setZmax);
    connect(m_chkPlanner, &QCheckBox::toggled, this, &FovLayer::setPlannerMode);

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

void FovLayer::setRectMode(bool on)
{
    m_rectMode = on;
    if (m_spinMargin) m_spinMargin->setEnabled(on);
    if (m_spinHalf) m_spinHalf->setEnabled(!on);
    if (m_enabled) scheduleRecalc();
}

void FovLayer::setSafetyMargin(double v)
{
    m_margin = std::max(0.0, v);
    if (m_enabled && m_rectMode) scheduleRecalc();
}

void FovLayer::setObstacleSource(int src)
{
    m_obstacleSource = src;
    if (m_enabled) scheduleRecalc();
}

// ---------- T3.4 planner 同口径 ----------
void FovLayer::setPlannerMode(bool on)
{
    m_plannerMode = on;
    if (m_spinFront) {
        // 同口径下界面值不再直接生效，明确告知，避免"改了没反应"的困惑
        m_spinFront->setEnabled(!on);
        m_spinFront->setToolTip(on
            ? QObject::tr("planner 同口径模式：距离=0.4 m/s × collision_time，"
                          "按轨迹点属性分四档（直线/曲线/不绕行/掉头），此值仅作兜底")
            : QString());
    }
    if (m_lblPlannerHint) {
        m_lblPlannerHint->setText(on
            ? QObject::tr("同口径：直线 %1m / 曲线 %2m / 不绕行 %3m / 掉头 %4m")
                  .arg(blz::sceneFrontDist(m_limits, blz::DriveScene::Straight), 0, 'f', 2)
                  .arg(blz::sceneFrontDist(m_limits, blz::DriveScene::Curve), 0, 'f', 2)
                  .arg(blz::sceneFrontDist(m_limits, blz::DriveScene::CurveNoBypass), 0, 'f', 2)
                  .arg(blz::sceneFrontDist(m_limits, blz::DriveScene::CurveTurnAround), 0, 'f', 2)
            : QString());
        m_lblPlannerHint->setVisible(on);
    }
    if (m_chkPlanner && m_chkPlanner->isChecked() != on) {
        QSignalBlocker block(m_chkPlanner);
        m_chkPlanner->setChecked(on);
    }
    if (m_enabled) scheduleRecalc();
}

void FovLayer::setPlannerLimits(const blz::PlannerLimits& lim)
{
    m_limits = lim;
    if (m_lblPlannerHint && m_plannerMode) {
        m_lblPlannerHint->setText(
            QObject::tr("同口径：直线 %1m / 曲线 %2m / 不绕行 %3m / 掉头 %4m")
                .arg(blz::sceneFrontDist(m_limits, blz::DriveScene::Straight), 0, 'f', 2)
                .arg(blz::sceneFrontDist(m_limits, blz::DriveScene::Curve), 0, 'f', 2)
                .arg(blz::sceneFrontDist(m_limits, blz::DriveScene::CurveNoBypass), 0, 'f', 2)
                .arg(blz::sceneFrontDist(m_limits, blz::DriveScene::CurveTurnAround), 0, 'f', 2));
    }
    if (m_enabled && m_plannerMode) scheduleRecalc();
}

double FovLayer::frontDistForScene(blz::DriveScene s) const
{
    return blz::sceneFrontDist(m_limits, s);
}

void FovLayer::scheduleRecalc()
{
    if (!m_enabled || m_recalculating) return;
    m_debounce->start();
}

void FovLayer::rebuildOverlay()
{
    if (!m_scene) return;
    // 关键：场景 clear()（打开新图时）会 delete 所有 item，m_overlay 变悬垂。
    // QPointer 已自动置空 → 这里必须重建，绝不能凭 m_overlay 非空就跳过。
    if (!m_overlayGuard) {
        auto* item = new FovOverlayItem;
        item->setZValue(0.5);
        m_scene->addItem(item);
        m_overlayGuard = item;
        m_overlay = item;
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
    int totalPts = 0;
    double minHitDist = 1e18;

    constexpr double kHalfL = 1.85 * 0.5;   // 实车半长 0.925m
    constexpr double kHalfW = 1.50 * 0.5;   // 实车半宽 0.750m

    // 选择生效的障碍网格：
    // SourcePgm(默认): 优先用修过的 pgm 栅格，无 pgm 时自动退回 pcd（兼容纯 pcd 冒烟测试）
    // SourcePcd: 仅用原始 pcd
    // SourcePgmAndPcd: 先扫 pgm，再用 pcd 做交集过滤（若缺其一则退回可用者）
    const blz::GridProjection* primaryGrid = nullptr;
    const blz::GridProjection* intersectGrid = nullptr;
    if (m_obstacleSource == SourcePgm) {
        primaryGrid = (m_pgmGrid && m_pgmGrid->width > 0) ? m_pgmGrid : m_grid;
    } else if (m_obstacleSource == SourcePcd) {
        primaryGrid = m_grid;
    } else {
        if (m_pgmGrid && m_pgmGrid->width > 0 && m_grid && m_grid->width > 0) {
            primaryGrid = m_pgmGrid;
            intersectGrid = m_grid;
        } else {
            primaryGrid = (m_pgmGrid && m_pgmGrid->width > 0) ? m_pgmGrid : m_grid;
        }
    }

    for (BPathItem* path : m_paths) {
        if (!path) continue;
        if (path->scene() != m_scene) continue;
        const int n = path->pointsNumber();
        for (int i = 0; i < n; ++i) {
            BPointItem* pt = path->getElement(i);
            if (!pt) continue;
            ++totalPts;
            const QPointF pix = pt->getPoint();
            // 读取轨迹点属性：按左/右贴边、狭口强通、忽略障碍动态调整左右安全裕度
            int roadId = 0;
            bool forcePass = false;
            bool ignoreObs = false;
            if (auto* attr = pt->attr) {
                roadId = attr->road_id;
                ignoreObs = (attr->ignore_obstacles != 0);
                auto it = attr->self_define.find(QStringLiteral("force_pass"));
                if (it != attr->self_define.end() && it->second.toInt() != 0)
                    forcePass = true;
            }
            double leftMargin = m_margin;
            double rightMargin = m_margin;
            if (forcePass) {
                leftMargin = 0.0;
                rightMargin = 0.0;
            } else if (roadId == 1) {
                rightMargin = 0.0;   // 右贴边：右侧贴边裁剪
            } else if (roadId == 2) {
                leftMargin = 0.0;    // 左贴边：左侧贴边裁剪
            }

            // T3.4：前视距离。planner 同口径模式下按该点的路段属性取
            // 0.4 × collision_time（直线/曲线/不绕行/掉头四档），
            // 与 pursuit_local_planner.cpp 的碰撞检测距离完全一致；
            // 关闭时沿用旧的固定 m_front_dist。
            blz::PointFlags flags;
            double frontDist = m_front_dist;
            if (auto* attr = pt->attr) {
                flags.curve_paths = attr->curve_paths;
                flags.turnAround = attr->turnAround;
                flags.bypass = attr->bypass;   // 反相语义在 pickScene 内处理
            }
            if (m_plannerMode) {
                frontDist = blz::frontDistForPoint(m_limits, flags);
            }
            // 兜底：算出来 <= 0（参数错误/该场景被禁用）时退回界面值，
            // 避免"永不报警"这种静默失效。
            if (!(frontDist > 0.0)) frontDist = m_front_dist;

            // 实时按相邻轨迹点重算切线朝向（拖动轨迹点后红框立即随弯旋转）
            qreal yaw = pt->yaw();
            if (n >= 2) {
                BPointItem* pA = (i + 1 < n) ? pt : path->getElement(i - 1);
                BPointItem* pB = (i + 1 < n) ? path->getElement(i + 1) : pt;
                if (pA && pB) {
                    const QPointF dp = pB->getPoint() - pA->getPoint();
                    if (std::hypot(dp.x(), dp.y()) > 1e-3)
                        yaw = std::atan2(-dp.y(), dp.x());  // 像素 y 向下，世界 dy = -dp.y
                }
            }
            const double screenYaw = -yaw;
            const double c = std::cos(screenYaw), s = std::sin(screenYaw);
            const double invRes = (m_resolution > 0.0) ? (1.0 / m_resolution) : 1.0;

            FovOverlayItem::Entry e;
            if (m_rectMode) {
                // 屏幕坐标系 y 向下：车体左侧(+y_world) 对应屏幕 -ly，右侧(-y_world) 对应屏幕 +ly
                const double xMinPx = -(kHalfL + (forcePass ? 0.0 : m_margin)) * invRes;
                const double xMaxPx = (kHalfL + frontDist) * invRes;
                const double leftPx  = (kHalfW + leftMargin) * invRes;
                const double rightPx = (kHalfW + rightMargin) * invRes;
                auto rot = [&](double lx, double ly) {
                    return pix + QPointF(lx * c - ly * s, lx * s + ly * c);
                };
                QPolygonF box;
                box << rot(xMinPx, -leftPx) << rot(xMaxPx, -leftPx)
                    << rot(xMaxPx,  rightPx) << rot(xMinPx,  rightPx);
                e.sector = box;
                e.labelPos = rot(xMaxPx, -leftPx) + QPointF(4, -3);
            } else {
                // 传统扇形视锥
                QPolygonF sector;
                sector << pix;
                const qreal a0 = screenYaw - rad, a1 = screenYaw + rad;
                const qreal reach = frontDist * invRes;
                for (int k = 0; k <= 16; ++k) {
                    const qreal a = a0 + (a1 - a0) * k / 16.0;
                    sector << pix + QPointF(reach * std::cos(a), reach * std::sin(a));
                }
                e.sector = sector;
                e.labelPos = pix + QPointF(reach * c + 4, reach * s - 3);
            }

            if (ignoreObs) {
                // 忽略障碍点：淡灰显示，不触发红框报警
                e.sectorColor = QColor(156, 163, 175, 16);
                e.borderColor = QColor(0, 0, 0, 0);
            } else if (primaryGrid && primaryGrid->width > 0 && m_resolution > 0) {
                const double wx = pix.x() * m_resolution + m_origin_x;
                const double wy = (m_pgm_height - 1 - pix.y()) * m_resolution + m_origin_y;

                blz::FovResult res;
                if (m_rectMode) {
                    blz::RectParams rp;
                    rp.x_min = -(kHalfL + (forcePass ? 0.0 : m_margin));
                    rp.x_max = kHalfL + frontDist;
                    rp.y_min = -(kHalfW + rightMargin);
                    rp.y_max =  (kHalfW + leftMargin);
                    rp.max_hits = 48;
                    rp.ref_x = kHalfL;
                    res = blz::cast_rect(*primaryGrid, wx, wy, yaw, rp);
                } else {
                    blz::FovParams fp;
                    fp.front_dist = frontDist;
                    fp.half_angle = rad;
                    fp.hit_dist_max = std::max(frontDist, 3.0);
                    res = blz::cast_fov(*primaryGrid, wx, wy, yaw, fp);
                }

                // 若启用 PGM ∩ PCD 交集模式，仅保留同时在另一网格也占据的命中点
                if (res.hit && intersectGrid) {
                    std::vector<blz::Vec3> kept;
                    kept.reserve(res.hits.size());
                    const double cw = std::cos(yaw), sw = std::sin(yaw);
                    double bestD = 1e18;
                    for (const auto& h : res.hits) {
                        if (intersectGrid->occupied_at(h.x, h.y)) {
                            kept.push_back(h);
                            const double lx = (h.x - wx) * cw + (h.y - wy) * sw;
                            const double d = std::max(0.0, lx - kHalfL);
                            if (d < bestD) bestD = d;
                        }
                    }
                    res.hits = std::move(kept);
                    res.hit = !res.hits.empty();
                    res.first_hit_dist = res.hit ? bestD : -1.0;
                }

                if (res.hit) {
                    e.isHit = true;
                    ++hits;
                    if (res.first_hit_dist >= 0.0 && res.first_hit_dist < minHitDist)
                        minHitDist = res.first_hit_dist;
                    if (m_rectMode) {
                        e.sectorColor = QColor(239, 68, 68, 52);    // 危险红框
                        e.borderColor = QColor(220, 38, 38, 210);
                        if (res.first_hit_dist >= 0.0 && (i % 3 == 0 || hits <= 5)) {
                            e.label = (res.first_hit_dist <= 0.01)
                                ? QObject::tr("车身擦碰!")
                                : QObject::tr("前视 %1m").arg(res.first_hit_dist, 0, 'f', 2);
                        }
                    } else {
                        e.sectorColor = QColor(30, 144, 255, 46);
                        e.borderColor = QColor(0, 0, 0, 0);
                    }
                    for (const auto& h : res.hits) {
                        const double hx = (h.x - m_origin_x) / m_resolution;
                        const double hy = m_pgm_height - 1 - (h.y - m_origin_y) / m_resolution;
                        e.dots << QPointF(hx, hy);
                    }
                } else {
                    e.sectorColor = m_rectMode ? QColor(34, 197, 94, 8) : QColor(200, 200, 200, 26);
                    e.borderColor = QColor(0, 0, 0, 0);
                }
            } else {
                e.sectorColor = m_rectMode ? QColor(156, 163, 175, 16) : QColor(200, 200, 200, 26);
                e.borderColor = QColor(0, 0, 0, 0);
            }
            entries.push_back(e);
        }
    }

    if (m_overlayGuard) m_overlayGuard->setEntries(entries);
    m_lastHitCount = hits;
    if (m_lblSummary) {
        if (!primaryGrid || primaryGrid->width <= 0) {
            m_lblSummary->setText(QObject::tr("尚未加载地图或 .pcd（仅显示检测框轮廓）"));
            m_lblSummary->setStyleSheet("color: #b45309; font-size: 12px;");
        } else if (hits > 0) {
            const QString distTxt = (minHitDist <= 0.01)
                ? QObject::tr("存在车身擦碰!")
                : QObject::tr("最近前视 %1m").arg(minHitDist < 1e17 ? minHitDist : 0.0, 0, 'f', 2);
            m_lblSummary->setText(QObject::tr("危险点：%1 / %2（%3）")
                                      .arg(hits).arg(totalPts).arg(distTxt));
            m_lblSummary->setStyleSheet("color: #dc2626; font-weight: bold; font-size: 12px;");
        } else {
            m_lblSummary->setText(QObject::tr("全线 %1 个轨迹点净空安全（0 命中）").arg(totalPts));
            m_lblSummary->setStyleSheet("color: #15803d; font-weight: bold; font-size: 12px;");
        }
    }
    m_recalculating = false;
    emit recalculated(hits);
}
