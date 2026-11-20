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
#include <QColor>

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
    // 显示文本可能带 " ★" 标记，发给 bridge 的必须是纯分支名
    const QString data = m_comboBranch->currentData().toString();
    return data.isEmpty() ? m_comboBranch->currentText() : data;
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
    // 拦不可跑的场地：直接提示，不发请求（发了也是 0.1 秒后 failed，
    // 用户再去连直播只会"没反应"，体验极差）
    if (!m_comboBag->currentData(Qt::UserRole + 2).toBool()) {
        showError(tr("这个场地缺少 container 配置（bag/map_dir），无法真正跑 run。\n"
                     "请改选「标准回归样本」场地。"));
        return;
    }

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
        // Qt 对 4xx/5xx 统一报 "server replied: Not Found/Conflict"，
        // 真正的信息在响应体 {"detail": ...} 里 —— 优先展示它，否则排查会被误导。
        const QByteArray body = reply->readAll();
        QString detail;
        const QJsonDocument d = QJsonDocument::fromJson(body);
        if (d.isObject()) {
            const QJsonValue dv = d.object().value("detail");
            if (dv.isString()) detail = dv.toString();
            else if (dv.isArray()) {
                // 422 校验错误是数组，把 msg 拼起来
                QStringList parts;
                for (const auto& e : dv.toArray())
                    parts << e.toObject().value("msg").toString();
                detail = parts.join("; ");
            }
        }
        const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (err.contains("Connection refused") || err.contains("HostNotFound")) {
            showError(tr("无法连接 bridge（%1）。\n"
                         "请先运行：bash sim-bridge/start_bridge.sh").arg(m_bridgeUrl));
        } else if (!detail.isEmpty()) {
            showError(tr("提交失败（HTTP %1）：%2").arg(http).arg(detail));
        } else if (http == 409) {
            showError(tr("已有 run 在跑（首版单并发）。请等当前 run 结束或重启 bridge 后再试。"));
        } else {
            showError(tr("%1（HTTP %2）").arg(err).arg(http));
        }
        m_btnSubmit->setEnabled(true);
        reply->deleteLater();
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
        m_venueRunnable.clear();
        const QJsonArray arr = doc.array();
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            const QString vid = o["venue_id"].toString();
            // 跳过不可跑的场地（缺 container.bag 等），否则用户选了它
            // 提交后 run 立刻 failed，连直播只会"没反应"。
            const bool runnable = o.contains("runnable")
                                  ? o["runnable"].toBool() : true;
            m_venueRunnable[vid] = runnable;
            // 获取该场地的 bags
            QNetworkRequest req{QUrl(m_bridgeUrl + "/venues/" + vid + "/bags")};
            req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
            m_nam->get(req);
        }
    }
    else if (url.contains("/venues/") && url.endsWith("/bags")) {
        const QJsonArray arr = doc.array();
        // 路径形如 /venues/{vid}/bags —— 必须用 section("/bags", 0, 0) 只取第一段，
        // 否则 end 默认 -1 会连 "/bags" 一起返回，venue_id 变成 "xxx/bags" → POST /runs 404
        const QString vid = url.section("/venues/", 1).section("/bags", 0, 0);
        const bool runnable = m_venueRunnable.value(vid, true);
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            const QString bag = o["bag"].toString();
            // 不可跑的场地仍然列出，但标注清楚，避免用户选到才失败
            const QString label = runnable ? bag
                                           : bag + tr("（场地缺 container，跑不了）");
            m_comboBag->addItem(label, bag);
            // 存 venue_id 到 UserRole+1
            int idx = m_comboBag->count() - 1;
            m_comboBag->setItemData(idx, vid, Qt::UserRole + 1);
            m_comboBag->setItemData(idx, runnable, Qt::UserRole + 2);
            if (!runnable) {
                // 灰显，视觉上区分
                m_comboBag->setItemData(idx, QColor(150, 150, 150), Qt::ForegroundRole);
            }
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
