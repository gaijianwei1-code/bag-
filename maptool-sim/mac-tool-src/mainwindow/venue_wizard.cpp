// T1.2 场地档案向导实现
#include "venue_wizard.h"

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QImage>
#include <QMessageBox>
#include <QDate>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <algorithm>

#include "../../core/grid_projection.h"

namespace {
QString makeEditableComboDefault(QComboBox* cb, const QString& placeholder)
{
    cb->setEditable(true);
    if (cb->lineEdit()) cb->lineEdit()->setPlaceholderText(placeholder);
    return QString();
}
} // namespace

VenueWizard::VenueWizard(const QString& bridgeUrl, const QString& token, QWidget* parent)
    : QDialog(parent), m_bridgeUrl(bridgeUrl), m_token(token)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &VenueWizard::onNetworkFinished);
    setupUi();
    updateStepUi();
}

void VenueWizard::setupUi()
{
    setWindowTitle(tr("场地档案向导（T1.2 — 三步录入新场地）"));
    setMinimumSize(620, 480);

    auto* root = new QVBoxLayout(this);

    m_lblStepBar = new QLabel;
    m_lblStepBar->setStyleSheet(
        "QLabel { background: #f1f5f9; padding: 10px 14px; border-radius: 6px; font-size: 13px; }");
    root->addWidget(m_lblStepBar);

    m_stack = new QStackedWidget;
    root->addWidget(m_stack, 1);

    // ================= 第 1 步：地图 =================
    auto* page1 = new QWidget;
    auto* v1 = new QVBoxLayout(page1);
    auto* grpBase = new QGroupBox(tr("基本信息"));
    auto* fBase = new QFormLayout(grpBase);
    m_editId = new QLineEdit;
    m_editId->setPlaceholderText(tr("小写字母、数字、连字符，如 site-east-01"));
    m_editName = new QLineEdit;
    m_editName->setPlaceholderText(tr("中文显示名，如 东区地下车库 B2"));
    fBase->addRow(tr("场地 ID *"), m_editId);
    fBase->addRow(tr("场地名称 *"), m_editName);
    v1->addWidget(grpBase);

    auto* grpMap = new QGroupBox(tr("选择地图文件（.pgm，自动读取同目录 .yaml）"));
    auto* fMap = new QFormLayout(grpMap);
    auto* hPgm = new QHBoxLayout;
    m_editPgm = new QLineEdit;
    m_editPgm->setPlaceholderText(tr("选择 .pgm 栅格地图文件"));
    auto* btnPgm = new QPushButton(tr("浏览…"));
    hPgm->addWidget(m_editPgm, 1);
    hPgm->addWidget(btnPgm);
    fMap->addRow(tr("地图 .pgm *"), hPgm);

    m_spinRes = new QDoubleSpinBox;
    m_spinRes->setRange(0.005, 0.200);
    m_spinRes->setDecimals(4);
    m_spinRes->setSingleStep(0.01);
    m_spinRes->setValue(0.05);
    m_spinOriginX = new QDoubleSpinBox;
    m_spinOriginX->setRange(-10000.0, 10000.0);
    m_spinOriginX->setDecimals(4);
    m_spinOriginY = new QDoubleSpinBox;
    m_spinOriginY->setRange(-10000.0, 10000.0);
    m_spinOriginY->setDecimals(4);

    auto* hPose = new QHBoxLayout;
    hPose->addWidget(new QLabel(tr("分辨率:")));
    hPose->addWidget(m_spinRes);
    hPose->addWidget(new QLabel(tr("origin X:")));
    hPose->addWidget(m_spinOriginX);
    hPose->addWidget(new QLabel(tr("Y:")));
    hPose->addWidget(m_spinOriginY);
    fMap->addRow(tr("坐标参数"), hPose);

    m_lblMapCheck = new QLabel(tr("请选择 .pgm 地图文件以自动校验坐标参数"));
    m_lblMapCheck->setWordWrap(true);
    m_lblMapCheck->setStyleSheet("color: #6b7280; font-size: 12px;");
    fMap->addRow(m_lblMapCheck);
    v1->addWidget(grpMap);
    v1->addStretch();
    m_stack->addWidget(page1);

    // ================= 第 2 步：点云 =================
    auto* page2 = new QWidget;
    auto* v2 = new QVBoxLayout(page2);
    auto* grpPcd = new QGroupBox(tr("选择点云文件（.pcd）与高度过滤"));
    auto* fPcd = new QFormLayout(grpPcd);
    auto* hPcd = new QHBoxLayout;
    m_editPcd = new QLineEdit;
    m_editPcd->setPlaceholderText(tr("选择 .pcd 点云文件（如 frames_no_ground.pcd）"));
    auto* btnPcd = new QPushButton(tr("浏览…"));
    hPcd->addWidget(m_editPcd, 1);
    hPcd->addWidget(btnPcd);
    fPcd->addRow(tr("点云 .pcd *"), hPcd);

    m_spinZmin = new QDoubleSpinBox;
    m_spinZmin->setRange(0.0, 10.0);
    m_spinZmin->setDecimals(2);
    m_spinZmin->setSingleStep(0.05);
    m_spinZmin->setValue(0.05);
    m_spinZmax = new QDoubleSpinBox;
    m_spinZmax->setRange(0.1, 30.0);
    m_spinZmax->setDecimals(2);
    m_spinZmax->setSingleStep(0.1);
    m_spinZmax->setValue(2.00);
    auto* hZ = new QHBoxLayout;
    hZ->addWidget(new QLabel(tr("最小高度 zmin (m):")));
    hZ->addWidget(m_spinZmin);
    hZ->addWidget(new QLabel(tr("最大高度 zmax (m):")));
    hZ->addWidget(m_spinZmax);
    hZ->addStretch();
    fPcd->addRow(tr("高度过滤"), hZ);

    m_lblPcdCheck = new QLabel(tr("选择 .pcd 后将即时校验点数与地图坐标重叠度"));
    m_lblPcdCheck->setWordWrap(true);
    m_lblPcdCheck->setStyleSheet("color: #6b7280; font-size: 12px;");
    fPcd->addRow(m_lblPcdCheck);
    v2->addWidget(grpPcd);
    v2->addStretch();
    m_stack->addWidget(page2);

    // ================= 第 3 步：Bag / 轨迹 + 容器路径 =================
    auto* page3 = new QWidget;
    auto* v3 = new QVBoxLayout(page3);
    auto* grpBag = new QGroupBox(tr("关联轨迹与 Bag"));
    auto* fBag = new QFormLayout(grpBag);

    auto* hTraj = new QHBoxLayout;
    m_editTraj = new QLineEdit;
    m_editTraj->setPlaceholderText(tr("选择默认轨迹 .json 文件"));
    auto* btnTraj = new QPushButton(tr("浏览…"));
    hTraj->addWidget(m_editTraj, 1);
    hTraj->addWidget(btnTraj);
    fBag->addRow(tr("默认轨迹 .json *"), hTraj);

    auto* hBag = new QHBoxLayout;
    m_editBagLocal = new QLineEdit;
    m_editBagLocal->setPlaceholderText(tr("点右侧「浏览…」选择本机 .bag 文件"));
    auto* btnBag = new QPushButton(tr("浏览…"));
    hBag->addWidget(m_editBagLocal, 1);
    hBag->addWidget(btnBag);
    fBag->addRow(tr("本机 Bag 文件 *"), hBag);

    m_editBagRel = new QLineEdit;
    m_editBagRel->setPlaceholderText(tr("自动生成，如 bags/site-east-01.bag"));
    fBag->addRow(tr("Bag 标识名 *"), m_editBagRel);

    m_chkAutoSync = new QCheckBox(tr("保存时自动同步地图 / 点云 / 轨迹 / Bag 进 Docker 容器（保存后立即可复跑）"));
    m_chkAutoSync->setChecked(true);
    fBag->addRow(QString(), m_chkAutoSync);

    auto* hDur = new QHBoxLayout;
    m_spinDuration = new QSpinBox;
    m_spinDuration->setRange(10, 600);
    m_spinDuration->setValue(76);
    m_spinDuration->setSuffix(tr(" 秒"));
    m_editParamSet = new QLineEdit(QStringLiteral("t31_tol_1p0"));
    hDur->addWidget(m_spinDuration);
    hDur->addWidget(new QLabel(tr("默认参数集:")));
    hDur->addWidget(m_editParamSet, 1);
    fBag->addRow(tr("默认时长"), hDur);

    m_lblTrajCheck = new QLabel(tr("请选择轨迹 .json 文件"));
    m_lblTrajCheck->setWordWrap(true);
    m_lblTrajCheck->setStyleSheet("color: #6b7280; font-size: 12px;");
    fBag->addRow(m_lblTrajCheck);
    v3->addWidget(grpBag);

    auto* grpCont = new QGroupBox(tr("容器仿真路径（可选；补全后可直接在回归面板跑 B 线复跑）"));
    auto* fCont = new QFormLayout(grpCont);
    auto* hScan = new QHBoxLayout;
    m_btnScanContainer = new QPushButton(tr("自动扫描容器可用资产"));
    m_lblContainerHint = new QLabel(tr("留空则仅支持 L0 前视快算"));
    m_lblContainerHint->setStyleSheet("color: #6b7280; font-size: 12px;");
    hScan->addWidget(m_btnScanContainer);
    hScan->addWidget(m_lblContainerHint, 1);
    fCont->addRow(hScan);

    m_comboContainerMapDir = new QComboBox;
    m_comboContainerBag = new QComboBox;
    m_comboContainerTraj = new QComboBox;
    m_comboContainerWorld = new QComboBox;
    makeEditableComboDefault(m_comboContainerMapDir, tr("/root/catkin_ws/scenes/..."));
    makeEditableComboDefault(m_comboContainerBag, tr("/root/bags/... 或 /root/catkin_ws/runs/.../evidence.bag"));
    makeEditableComboDefault(m_comboContainerTraj, tr("/root/catkin_ws/scenes/.../*.json"));
    makeEditableComboDefault(m_comboContainerWorld, tr("/root/catkin_ws/scenes/.../world_obstacles.pcd"));
    fCont->addRow(tr("container.map_dir"), m_comboContainerMapDir);
    fCont->addRow(tr("container.bag"), m_comboContainerBag);
    fCont->addRow(tr("container.trajectory"), m_comboContainerTraj);
    fCont->addRow(tr("container.world"), m_comboContainerWorld);
    v3->addWidget(grpCont);
    m_stack->addWidget(page3);

    // ================= 底部按钮栏 =================
    m_lblFooterStatus = new QLabel;
    m_lblFooterStatus->setWordWrap(true);
    root->addWidget(m_lblFooterStatus);

    auto* hNav = new QHBoxLayout;
    auto* btnCancel = new QPushButton(tr("取消"));
    m_btnPrev = new QPushButton(tr("← 上一步"));
    m_btnNext = new QPushButton(tr("下一步 →"));
    m_btnNext->setStyleSheet("QPushButton { background: #2563eb; color: white; font-weight: bold; padding: 6px 18px; }");
    hNav->addWidget(btnCancel);
    hNav->addStretch();
    hNav->addWidget(m_btnPrev);
    hNav->addWidget(m_btnNext);
    root->addLayout(hNav);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnPrev, &QPushButton::clicked, this, &VenueWizard::onPrevStep);
    connect(m_btnNext, &QPushButton::clicked, this, &VenueWizard::onNextOrSave);
    connect(btnPgm, &QPushButton::clicked, this, &VenueWizard::browsePgm);
    connect(btnPcd, &QPushButton::clicked, this, &VenueWizard::browsePcd);
    connect(btnTraj, &QPushButton::clicked, this, &VenueWizard::browseTraj);
    connect(btnBag, &QPushButton::clicked, this, &VenueWizard::browseBag);
    connect(m_chkAutoSync, &QCheckBox::toggled, this, [this](bool on) {
        m_comboContainerMapDir->setEnabled(!on);
        m_comboContainerBag->setEnabled(!on);
        m_comboContainerTraj->setEnabled(!on);
        m_comboContainerWorld->setEnabled(!on);
        m_btnScanContainer->setEnabled(!on);
        m_lblContainerHint->setText(on ? tr("已启用自动同步：保存时自动导入容器并生成 scene.yaml")
                                       : tr("手动模式：可扫描或填写容器内已有路径"));
    });
    // 初始同步容器区启用状态
    m_comboContainerMapDir->setEnabled(false);
    m_comboContainerBag->setEnabled(false);
    m_comboContainerTraj->setEnabled(false);
    m_comboContainerWorld->setEnabled(false);
    m_btnScanContainer->setEnabled(false);
    m_lblContainerHint->setText(tr("已启用自动同步：保存时自动导入容器并生成 scene.yaml"));
    connect(m_btnScanContainer, &QPushButton::clicked, this, &VenueWizard::scanContainerAssets);
    connect(m_editId, &QLineEdit::textChanged, this, [this](const QString& txt) {
        if (m_editBagRel->text().trimmed().isEmpty() && !txt.trimmed().isEmpty()) {
            m_editBagRel->setPlaceholderText(QStringLiteral("bags/%1.bag").arg(txt.trimmed()));
        }
    });
}

void VenueWizard::updateStepUi()
{
    const int idx = m_stack->currentIndex();
    auto fmt = [&](int step, const QString& label) {
        if (step == idx)
            return QStringLiteral("<b style='color:#2563eb;'>%1</b>").arg(label);
        if (step < idx)
            return QStringLiteral("<span style='color:#15803d;'>✓ %1</span>").arg(label);
        return QStringLiteral("<span style='color:#64748b;'>%1</span>").arg(label);
    };
    m_lblStepBar->setText(
        fmt(0, tr("① 选择地图")) + QStringLiteral("  →  ")
        + fmt(1, tr("② 选择点云")) + QStringLiteral("  →  ")
        + fmt(2, tr("③ 关联 Bag/轨迹与保存")));

    m_btnPrev->setEnabled(idx > 0);
    m_btnNext->setText(idx < 2 ? tr("下一步 →") : tr("完成并保存场地"));
    m_lblFooterStatus->clear();
}

void VenueWizard::browsePgm()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择地图 .pgm"), m_lastDir, tr("PGM 地图 (*.pgm);;All Files (*)"));
    if (path.isEmpty()) return;
    m_lastDir = QFileInfo(path).absolutePath();
    m_editPgm->setText(path);
    parseYamlNearPgm(path);

    // 若场地 ID 未填，按文件名自动建议合法 ID
    if (m_editId->text().trimmed().isEmpty()) {
        QString slug = QFileInfo(path).baseName().toLower();
        slug.replace(QRegularExpression("[^a-z0-9]+"), "-");
        slug.remove(QRegularExpression("^-+|-+$"));
        if (!slug.isEmpty()) {
            if (!slug.startsWith("site-")) slug = QStringLiteral("site-") + slug;
            m_editId->setText(slug);
        }
    }
    if (m_editName->text().trimmed().isEmpty()) {
        m_editName->setText(QFileInfo(path).baseName());
    }
}

void VenueWizard::parseYamlNearPgm(const QString& pgmPath)
{
    m_pgmWidth = 0;
    m_pgmHeight = 0;
    QImage img(pgmPath);
    if (!img.isNull()) {
        m_pgmWidth = img.width();
        m_pgmHeight = img.height();
    }

    const QFileInfo fi(pgmPath);
    const QString yamlPath = fi.absolutePath() + "/" + fi.completeBaseName() + ".yaml";
    bool yamlOk = false;
    if (QFile::exists(yamlPath)) {
        QFile f(yamlPath);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f);
            const QString content = in.readAll();
            QRegularExpression reRes(R"(resolution\s*:\s*([0-9.\-eE]+))");
            auto mRes = reRes.match(content);
            if (mRes.hasMatch()) {
                m_spinRes->setValue(mRes.captured(1).toDouble());
                yamlOk = true;
            }
            QRegularExpression reOrgInline(R"(origin\s*:\s*\[\s*([0-9.\-eE]+)\s*,\s*([0-9.\-eE]+))");
            auto mOrg = reOrgInline.match(content);
            if (!mOrg.hasMatch()) {
                // 兼容多行 YAML 列表格式：origin:\n- -66.43\n- -86.40
                QRegularExpression reOrgBlock(
                    R"(origin\s*:\s*(?:\r?\n)\s*-\s*([0-9.\-eE]+)\s*(?:\r?\n)\s*-\s*([0-9.\-eE]+))");
                mOrg = reOrgBlock.match(content);
            }
            if (mOrg.hasMatch()) {
                m_spinOriginX->setValue(mOrg.captured(1).toDouble());
                m_spinOriginY->setValue(mOrg.captured(2).toDouble());
                yamlOk = true;
            }
        }
    }

    if (m_pgmWidth > 0 && yamlOk) {
        const double wM = m_pgmWidth * m_spinRes->value();
        const double hM = m_pgmHeight * m_spinRes->value();
        m_lblMapCheck->setText(
            tr("✓ 已解析 %1（%2×%3 px，约 %4×%5 m，res=%6）")
                .arg(fi.completeBaseName() + ".yaml")
                .arg(m_pgmWidth).arg(m_pgmHeight)
                .arg(wM, 0, 'f', 1).arg(hM, 0, 'f', 1)
                .arg(m_spinRes->value(), 0, 'f', 3));
        m_lblMapCheck->setStyleSheet("color: #15803d; font-size: 12px;");
    } else if (m_pgmWidth > 0) {
        m_lblMapCheck->setText(
            tr("⚠ 已读取地图 %1×%2 px，未找到同名 .yaml，请确认上方分辨率与原点")
                .arg(m_pgmWidth).arg(m_pgmHeight));
        m_lblMapCheck->setStyleSheet("color: #b45309; font-size: 12px;");
    } else {
        m_lblMapCheck->setText(tr("✗ 无法读取 .pgm 图像，请检查文件"));
        m_lblMapCheck->setStyleSheet("color: #dc2626; font-size: 12px;");
    }
}

void VenueWizard::browsePcd()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择点云 .pcd"), m_lastDir, tr("Point Cloud (*.pcd);;All Files (*)"));
    if (path.isEmpty()) return;
    m_lastDir = QFileInfo(path).absolutePath();
    m_editPcd->setText(path);
    checkPcdAndOverlap();
}

void VenueWizard::checkPcdAndOverlap()
{
    m_pcdOk = false;
    const QString pcdPath = m_editPcd->text().trimmed();
    if (pcdPath.isEmpty() || !QFile::exists(pcdPath)) {
        m_lblPcdCheck->setText(tr("✗ 点云文件不存在"));
        m_lblPcdCheck->setStyleSheet("color: #dc2626; font-size: 12px;");
        return;
    }
    try {
        auto pts = blz::load_pcd(pcdPath.toStdString());
        if (pts.empty()) {
            m_lblPcdCheck->setText(tr("✗ 点云文件为空（0 点）"));
            m_lblPcdCheck->setStyleSheet("color: #dc2626; font-size: 12px;");
            return;
        }
        float xmin = pts[0].x, xmax = pts[0].x, ymin = pts[0].y, ymax = pts[0].y;
        const size_t step = std::max<size_t>(1, pts.size() / 50000);
        for (size_t i = 0; i < pts.size(); i += step) {
            xmin = std::min(xmin, pts[i].x); xmax = std::max(xmax, pts[i].x);
            ymin = std::min(ymin, pts[i].y); ymax = std::max(ymax, pts[i].y);
        }
        m_pcdOk = true;
        QString overlapMsg;
        if (m_pgmWidth > 0 && m_pgmHeight > 0) {
            const double mx0 = m_spinOriginX->value();
            const double my0 = m_spinOriginY->value();
            const double mx1 = mx0 + m_pgmWidth * m_spinRes->value();
            const double my1 = my0 + m_pgmHeight * m_spinRes->value();
            const bool overlaps = !(xmax < mx0 || xmin > mx1 || ymax < my0 || ymin > my1);
            overlapMsg = overlaps ? tr("；✓ 与地图坐标范围重叠")
                                  : tr("；⚠ 警告：与地图坐标范围无重叠，请核对 origin");
            m_lblPcdCheck->setStyleSheet(overlaps ? "color: #15803d; font-size: 12px;"
                                                  : "color: #b45309; font-size: 12px;");
        } else {
            m_lblPcdCheck->setStyleSheet("color: #15803d; font-size: 12px;");
        }
        m_lblPcdCheck->setText(
            tr("✓ 已校验 %1 点（X:[%2, %3] Y:[%4, %5]%6）")
                .arg(pts.size())
                .arg(xmin, 0, 'f', 1).arg(xmax, 0, 'f', 1)
                .arg(ymin, 0, 'f', 1).arg(ymax, 0, 'f', 1)
                .arg(overlapMsg));
    } catch (const std::exception& e) {
        m_lblPcdCheck->setText(tr("✗ 点云解析失败：%1").arg(e.what()));
        m_lblPcdCheck->setStyleSheet("color: #dc2626; font-size: 12px;");
    }
}

void VenueWizard::browseTraj()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择轨迹 .json"), m_lastDir, tr("Trajectory JSON (*.json);;All Files (*)"));
    if (path.isEmpty()) return;
    m_lastDir = QFileInfo(path).absolutePath();
    m_editTraj->setText(path);
    checkTrajFile();
}

void VenueWizard::browseBag()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("选择 ROS Bag (.bag)"), m_lastDir, tr("ROS Bag (*.bag);;All Files (*)"));
    if (path.isEmpty()) return;
    m_lastDir = QFileInfo(path).absolutePath();
    m_editBagLocal->setText(path);
    const QFileInfo fi(path);
    m_editBagRel->setText(QStringLiteral("bags/%1").arg(fi.fileName()));
    const double mb = fi.size() / (1024.0 * 1024.0);
    m_lblTrajCheck->setText(
        tr("✓ 已选择 Bag：%1（%2 MB）").arg(fi.fileName()).arg(mb, 0, 'f', 1));
    m_lblTrajCheck->setStyleSheet("color: #15803d; font-size: 12px;");
}

void VenueWizard::checkTrajFile()
{
    const QString p = m_editTraj->text().trimmed();
    QFile f(p);
    if (!f.open(QIODevice::ReadOnly)) {
        m_lblTrajCheck->setText(tr("✗ 无法打开轨迹文件"));
        m_lblTrajCheck->setStyleSheet("color: #dc2626; font-size: 12px;");
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    QJsonArray pts;
    if (doc.isObject()) pts = doc.object().value(QStringLiteral("points")).toArray();
    else if (doc.isArray()) pts = doc.array();
    if (pts.size() < 2) {
        m_lblTrajCheck->setText(tr("✗ 轨迹 JSON 缺少有效 points 数组（点数 < 2）"));
        m_lblTrajCheck->setStyleSheet("color: #dc2626; font-size: 12px;");
        return;
    }
    m_lblTrajCheck->setText(tr("✓ 轨迹有效：共 %1 个点").arg(pts.size()));
    m_lblTrajCheck->setStyleSheet("color: #15803d; font-size: 12px;");
}

void VenueWizard::scanContainerAssets()
{
    m_btnScanContainer->setEnabled(false);
    m_lblContainerHint->setText(tr("正在扫描容器内资产…"));
    QNetworkRequest req{QUrl(m_bridgeUrl + QStringLiteral("/container/assets"))};
    req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
    m_nam->get(req);
}

bool VenueWizard::validateStep1(QString& errOut) const
{
    const QString vid = m_editId->text().trimmed();
    QRegularExpression idRe(QStringLiteral("^[a-z0-9]+(-[a-z0-9]+)*$"));
    if (!idRe.match(vid).hasMatch()) {
        errOut = tr("场地 ID 仅允许小写字母、数字和连字符（如 site-east-01）");
        return false;
    }
    if (m_editName->text().trimmed().isEmpty()) {
        errOut = tr("请填写场地名称");
        return false;
    }
    if (m_editPgm->text().trimmed().isEmpty()) {
        errOut = tr("请选择地图 .pgm 文件");
        return false;
    }
    return true;
}

bool VenueWizard::validateStep2(QString& errOut) const
{
    if (m_editPcd->text().trimmed().isEmpty()) {
        errOut = tr("请选择点云 .pcd 文件");
        return false;
    }
    if (m_spinZmax->value() <= m_spinZmin->value()) {
        errOut = tr("高度过滤非法：zmax 必须大于 zmin");
        return false;
    }
    return true;
}

bool VenueWizard::validateStep3(QString& errOut) const
{
    if (m_editTraj->text().trimmed().isEmpty()) {
        errOut = tr("请选择默认轨迹 .json 文件");
        return false;
    }
    if (m_chkAutoSync && m_chkAutoSync->isChecked()) {
        const QString lp = m_editBagLocal->text().trimmed();
        if (lp.isEmpty() || !QFile::exists(lp)) {
            errOut = tr("已勾选自动同步进容器，请点击「浏览…」选择有效的本机 .bag 文件");
            return false;
        }
    }
    if (m_editBagRel->text().trimmed().isEmpty()) {
        errOut = tr("请填写或选择 Bag 标识路径（例如 bags/%1.bag）").arg(m_editId->text().trimmed());
        return false;
    }
    return true;
}

void VenueWizard::onPrevStep()
{
    const int idx = m_stack->currentIndex();
    if (idx > 0) {
        m_stack->setCurrentIndex(idx - 1);
        updateStepUi();
    }
}

void VenueWizard::onNextOrSave()
{
    const int idx = m_stack->currentIndex();
    QString err;
    if (idx == 0) {
        if (!validateStep1(err)) {
            m_lblFooterStatus->setText(tr("错误：%1").arg(err));
            m_lblFooterStatus->setStyleSheet("color: #dc2626;");
            return;
        }
        if (m_editBagRel->text().trimmed().isEmpty())
            m_editBagRel->setText(QStringLiteral("bags/%1.bag").arg(m_editId->text().trimmed()));
        m_stack->setCurrentIndex(1);
        updateStepUi();
        return;
    }
    if (idx == 1) {
        if (!validateStep2(err)) {
            m_lblFooterStatus->setText(tr("错误：%1").arg(err));
            m_lblFooterStatus->setStyleSheet("color: #dc2626;");
            return;
        }
        m_stack->setCurrentIndex(2);
        updateStepUi();
        return;
    }
    if (!validateStep3(err)) {
        m_lblFooterStatus->setText(tr("错误：%1").arg(err));
        m_lblFooterStatus->setStyleSheet("color: #dc2626;");
        return;
    }
    submitVenue(/*overwrite=*/false);
}

QJsonObject VenueWizard::buildVenueJson() const
{
    QJsonObject v;
    v["schema"] = QStringLiteral("venue/v1");
    v["venue_id"] = m_editId->text().trimmed();
    v["name"] = m_editName->text().trimmed();
    v["created"] = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    v["created_by"] = QStringLiteral("maptool-wizard");
    v["map_pgm"] = m_editPgm->text().trimmed();
    v["map_pcd"] = m_editPcd->text().trimmed();
    v["default_traj"] = m_editTraj->text().trimmed();
    v["default_param_set"] = m_editParamSet->text().trimmed().isEmpty()
        ? QStringLiteral("t31_tol_1p0") : m_editParamSet->text().trimmed();

    QJsonObject hf;
    hf["min"] = m_spinZmin->value();
    hf["max"] = m_spinZmax->value();
    v["pcd_height_filter"] = hf;

    QJsonObject org;
    org["x"] = m_spinOriginX->value();
    org["y"] = m_spinOriginY->value();
    QJsonObject ref;
    ref["origin"] = org;
    ref["resolution"] = m_spinRes->value();
    v["map_pose_ref"] = ref;

    QJsonObject bagItem;
    bagItem["bag"] = m_editBagRel->text().trimmed();
    bagItem["traj"] = m_editTraj->text().trimmed();
    bagItem["duration_s"] = m_spinDuration->value();
    if (!m_editBagLocal->text().trimmed().isEmpty())
        bagItem["local_bag"] = m_editBagLocal->text().trimmed();
    bagItem["notes"] = tr("由 MapTool 场地档案向导录入");
    QJsonArray bags;
    bags.append(bagItem);
    v["bags"] = bags;

    if (!m_editBagLocal->text().trimmed().isEmpty())
        v["local_bag"] = m_editBagLocal->text().trimmed();
    if (m_chkAutoSync && m_chkAutoSync->isChecked()) {
        v["sync_to_container"] = true;
    } else {
        const QString cMap = m_comboContainerMapDir->currentText().trimmed();
        const QString cBag = m_comboContainerBag->currentText().trimmed();
        const QString cTraj = m_comboContainerTraj->currentText().trimmed();
        const QString cWorld = m_comboContainerWorld->currentText().trimmed();
        if (!cMap.isEmpty() || !cBag.isEmpty() || !cTraj.isEmpty() || !cWorld.isEmpty()) {
            QJsonObject cont;
            if (!cMap.isEmpty()) cont["map_dir"] = cMap;
            if (!cBag.isEmpty()) cont["bag"] = cBag;
            if (!cTraj.isEmpty()) cont["trajectory"] = cTraj;
            if (!cWorld.isEmpty()) cont["world"] = cWorld;
            v["container"] = cont;
        }
    }
    return v;
}

void VenueWizard::submitVenue(bool overwrite)
{
    m_btnNext->setEnabled(false);
    m_lblFooterStatus->setText(tr("正在保存场地档案…"));
    m_lblFooterStatus->setStyleSheet("color: #2563eb;");

    const QJsonObject payload = buildVenueJson();
    const QString vid = payload.value(QStringLiteral("venue_id")).toString();
    const QUrl url = overwrite
        ? QUrl(m_bridgeUrl + QStringLiteral("/venues/") + vid)
        : QUrl(m_bridgeUrl + QStringLiteral("/venues"));
    QNetworkRequest req{url};
    req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (overwrite)
        m_nam->put(req, QJsonDocument(payload).toJson());
    else
        m_nam->post(req, QJsonDocument(payload).toJson());
}

void VenueWizard::onNetworkFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    const QString path = reply->url().path();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();

    if (path.endsWith(QStringLiteral("/container/assets"))) {
        m_btnScanContainer->setEnabled(true);
        if (reply->error() != QNetworkReply::NoError) {
            m_lblContainerHint->setText(tr("扫描失败：%1").arg(reply->errorString()));
            return;
        }
        const QJsonObject o = QJsonDocument::fromJson(body).object();
        auto fill = [](QComboBox* cb, const QJsonArray& arr) {
            const QString prev = cb->currentText();
            cb->clear();
            cb->addItem(QString());
            for (const auto& v : arr) cb->addItem(v.toString());
            if (!prev.isEmpty()) cb->setCurrentText(prev);
        };
        fill(m_comboContainerMapDir, o.value("map_dirs").toArray());
        fill(m_comboContainerBag, o.value("bags").toArray());
        fill(m_comboContainerTraj, o.value("trajectories").toArray());
        fill(m_comboContainerWorld, o.value("worlds").toArray());
        m_lblContainerHint->setText(
            tr("✓ 已扫描：%1 个场景目录、%2 个 bag")
                .arg(o.value("map_dirs").toArray().size())
                .arg(o.value("bags").toArray().size()));
        m_lblContainerHint->setStyleSheet("color: #15803d; font-size: 12px;");
        return;
    }

    m_btnNext->setEnabled(true);
    if (status == 409) {
        const auto ans = QMessageBox::question(
            this, tr("场地 ID 已存在"),
            tr("场地「%1」已存在，是否覆盖更新该场地档案？").arg(m_editId->text().trimmed()));
        if (ans == QMessageBox::Yes) {
            submitVenue(/*overwrite=*/true);
        } else {
            m_lblFooterStatus->setText(tr("已取消覆盖，请修改场地 ID"));
            m_lblFooterStatus->setStyleSheet("color: #b45309;");
        }
        return;
    }
    if (reply->error() != QNetworkReply::NoError || (status != 200 && status != 201)) {
        const QJsonObject errObj = QJsonDocument::fromJson(body).object();
        const QString detail = errObj.value(QStringLiteral("detail")).toString();
        m_lblFooterStatus->setText(
            tr("保存失败（HTTP %1）：%2")
                .arg(status)
                .arg(detail.isEmpty() ? reply->errorString() : detail));
        m_lblFooterStatus->setStyleSheet("color: #dc2626;");
        return;
    }

    m_savedVenueId = m_editId->text().trimmed();
    emit venueSaved(m_savedVenueId);
    QMessageBox::information(
        this, tr("保存成功"),
        tr("场地档案「%1」已保存至 sim-bridge，回归任务面板将自动刷新。").arg(m_savedVenueId));
    accept();
}
