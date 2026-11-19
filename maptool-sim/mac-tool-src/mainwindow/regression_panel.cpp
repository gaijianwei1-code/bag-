// T4.1 回归任务面板实现
#include "regression_panel.h"

#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

RegressionPanel::RegressionPanel(const QString& bridgeUrl, const QString& token,
                                 QWidget* parent)
    : QDialog(parent), m_bridgeUrl(bridgeUrl), m_token(token)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &RegressionPanel::onNetworkFinished);
    setupUi();
    fetchBranches();
    fetchVenues();
}

RegressionPanel::~RegressionPanel() = default;

void RegressionPanel::setupUi()
{
    setWindowTitle(tr("回归任务面板 — 选分支·改参数·复跑"));
    setMinimumSize(560, 480);

    auto* mainLayout = new QVBoxLayout(this);

    // 1. 分支 + bag 选择
    auto* grpSelect = new QGroupBox(tr("选择"));
    auto* formSel = new QFormLayout(grpSelect);
    m_comboBranch = new QComboBox;
    m_comboBag = new QComboBox;
    m_spinDuration = new QSpinBox;
    m_spinDuration->setRange(10, 600);
    m_spinDuration->setValue(40);
    m_spinDuration->setSuffix(tr(" 秒"));
    formSel->addRow(tr("分支"), m_comboBranch);
    formSel->addRow(tr("Bag"), m_comboBag);
    formSel->addRow(tr("时长"), m_spinDuration);
    mainLayout->addWidget(grpSelect);

    // 2. 参数表格
    auto* grpParams = new QGroupBox(tr("参数覆盖（可选）"));
    auto* vParams = new QVBoxLayout(grpParams);
    m_tableParams = new QTableWidget(0, 4);
    m_tableParams->setHorizontalHeaderLabels(
        {tr("文件"), tr("参数名"), tr("原值"), tr("新值")});
    m_tableParams->horizontalHeader()->setStretchLastSection(true);
    m_tableParams->setMinimumHeight(160);
    vParams->addWidget(m_tableParams);

    auto* hParamBtns = new QHBoxLayout;
    auto* btnAddRow = new QPushButton(tr("+ 添加参数"));
    auto* btnDelRow = new QPushButton(tr("- 删除选中"));
    hParamBtns->addWidget(btnAddRow);
    hParamBtns->addWidget(btnDelRow);
    hParamBtns->addStretch();
    vParams->addLayout(hParamBtns);
    mainLayout->addWidget(grpParams);

    // 3. 提交 + 状态
    auto* hBottom = new QHBoxLayout;
    m_btnSubmit = new QPushButton(tr("提交复跑"));
    m_btnSubmit->setStyleSheet("QPushButton { background: #2563eb; color: white;"
                               " font-weight: bold; padding: 8px 24px; }");
    m_btnRefresh = new QPushButton(tr("刷新"));
    hBottom->addWidget(m_btnRefresh);
    hBottom->addStretch();
    hBottom->addWidget(m_btnSubmit);
    mainLayout->addLayout(hBottom);

    m_lblStatus = new QLabel(tr("就绪"));
    m_lblStatus->setWordWrap(true);
    mainLayout->addWidget(m_lblStatus);

    // 信号
    connect(btnAddRow, &QPushButton::clicked, this, [this]() {
        addParamRow("schedule_goal.yaml", "", "0", "0");
    });
    connect(btnDelRow, &QPushButton::clicked, this, [this]() {
        int row = m_tableParams->currentRow();
        if (row >= 0) m_tableParams->removeRow(row);
    });
    connect(m_btnRefresh, &QPushButton::clicked, this, [this]() {
        fetchBranches(); fetchVenues();
    });
    connect(m_btnSubmit, &QPushButton::clicked, this, &RegressionPanel::submitRun);
    connect(m_comboBranch, &QComboBox::currentTextChanged,
            this, [this](const QString&) { updateSubmitState(); });
    connect(m_comboBag, &QComboBox::currentTextChanged,
            this, [this](const QString&) { updateSubmitState(); });
}

void RegressionPanel::addParamRow(const QString& file, const QString& key,
                                   const QString& from, const QString& to)
{
    int row = m_tableParams->rowCount();
    m_tableParams->insertRow(row);
    m_tableParams->setItem(row, 0, new QTableWidgetItem(file));
    m_tableParams->setItem(row, 1, new QTableWidgetItem(key));
    m_tableParams->setItem(row, 2, new QTableWidgetItem(from));
    m_tableParams->setItem(row, 3, new QTableWidgetItem(to));
}

QString RegressionPanel::selectedBranch() const
{
    return m_comboBranch->currentText();
}

QString RegressionPanel::selectedBag() const
{
    return m_comboBag->currentData().toString();
}

QList<ParamRow> RegressionPanel::paramChanges() const
{
    QList<ParamRow> rows;
    for (int i = 0; i < m_tableParams->rowCount(); ++i) {
        ParamRow r;
        r.file = m_tableParams->item(i, 0) ? m_tableParams->item(i, 0)->text() : "";
        r.key = m_tableParams->item(i, 1) ? m_tableParams->item(i, 1)->text() : "";
        r.fromVal = m_tableParams->item(i, 2) ? m_tableParams->item(i, 2)->text() : "";
        r.toVal = m_tableParams->item(i, 3) ? m_tableParams->item(i, 3)->text() : "";
        if (!r.key.isEmpty())
            rows.append(r);
    }
    return rows;
}

int RegressionPanel::durationSeconds() const
{
    return m_spinDuration->value();
}

void RegressionPanel::updateSubmitState()
{
    bool ok = !selectedBranch().isEmpty() && !selectedBag().isEmpty();
    m_btnSubmit->setEnabled(ok);
}

void RegressionPanel::showError(const QString& msg)
{
    m_lblStatus->setText(tr("错误：%1").arg(msg));
    m_lblStatus->setStyleSheet("color: red;");
}

// ---------- 网络请求 ----------
void RegressionPanel::fetchBranches()
{
    QNetworkRequest req{QUrl(m_bridgeUrl + "/branches")};
    req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_nam->get(req);
}

void RegressionPanel::fetchVenues()
{
    QNetworkRequest req{QUrl(m_bridgeUrl + "/venues")};
    req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_nam->get(req);
}

void RegressionPanel::submitRun()
{
    QJsonObject body;
    // venue_id 从 bag 的 userData 里取（fetchVenues 时存入）
    body["venue_id"] = m_comboBag->currentData(Qt::UserRole + 1).toString();
    body["bag"] = m_comboBag->currentData().toString();
    body["branch"] = selectedBranch();
    body["traj_json"] = QJsonObject();
    body["duration_s"] = m_spinDuration->value();

    // 参数覆盖
    QJsonObject overrides;
    for (const auto& r : paramChanges()) {
        overrides[r.key] = r.toVal.toDouble();
    }
    if (!overrides.isEmpty())
        body["param_overrides"] = overrides;

    QNetworkRequest req{QUrl(m_bridgeUrl + "/runs")};
    req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_nam->post(req, QJsonDocument(body).toJson());

    m_lblStatus->setText(tr("提交中…"));
    m_lblStatus->setStyleSheet("color: blue;");
    m_btnSubmit->setEnabled(false);
}

void RegressionPanel::onNetworkFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        const QString err = reply->errorString();
        if (err.contains("Connection refused") || err.contains("HostNotFound")) {
            showError(tr("无法连接 bridge（%1）。\n"
                         "请先运行：bash sim-bridge/start_bridge.sh").arg(m_bridgeUrl));
        } else {
            showError(err);
        }
        m_btnSubmit->setEnabled(true);
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    const QString url = reply->request().url().path();

    if (url.endsWith("/branches")) {
        m_comboBranch->clear();
        const QJsonArray arr = doc.array();
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            const QString name = o["branch"].toString();
            const QString commit = o["commit"].toString();
            const bool current = o["is_current"].toBool();
            m_comboBranch->addItem(current ? name + " ★" : name, name);
            if (current) m_comboBranch->setCurrentIndex(m_comboBranch->count() - 1);
        }
        updateSubmitState();
    }
    else if (url.endsWith("/venues")) {
        m_comboBag->clear();
        const QJsonArray arr = doc.array();
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            const QString vid = o["venue_id"].toString();
            // 获取该场地的 bags
            QNetworkRequest req{QUrl(m_bridgeUrl + "/venues/" + vid + "/bags")};
            req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
            m_nam->get(req);
        }
    }
    else if (url.contains("/venues/") && url.endsWith("/bags")) {
        const QJsonArray arr = doc.array();
        const QString vid = url.section("/venues/", 1).section("/bags", 0);
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            const QString bag = o["bag"].toString();
            m_comboBag->addItem(bag, bag);
            // 存 venue_id 到 UserRole+1
            int idx = m_comboBag->count() - 1;
            m_comboBag->setItemData(idx, vid, Qt::UserRole + 1);
        }
        updateSubmitState();
    }
    else if (url.endsWith("/runs")) {
        const QJsonObject o = doc.object();
        m_lastRunId = o["run_id"].toString();
        m_lblStatus->setText(tr("已提交 run_id=%1，切换到直播跟车连接").arg(m_lastRunId));
        m_lblStatus->setStyleSheet("color: green;");
        emit runSubmitted(m_lastRunId);
        m_btnSubmit->setEnabled(true);
    }
}
