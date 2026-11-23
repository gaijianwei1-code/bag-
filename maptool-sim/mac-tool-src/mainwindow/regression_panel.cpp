// T4.1 回归任务面板实现
#include "regression_panel.h"

#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QCheckBox>
#include <QMenu>
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
    fetchRuns();
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
    m_chkRunnableOnly = new QCheckBox(tr("仅显示可运行场地"));
    m_chkRunnableOnly->setChecked(true);
    m_spinDuration = new QSpinBox;
    m_spinDuration->setRange(10, 600);
    m_spinDuration->setValue(76);
    m_spinDuration->setSuffix(tr(" 秒"));
    formSel->addRow(tr("分支"), m_comboBranch);
    formSel->addRow(tr("Bag"), m_comboBag);
    formSel->addRow(QString(), m_chkRunnableOnly);
    formSel->addRow(tr("时长"), m_spinDuration);
    mainLayout->addWidget(grpSelect);

    // 2. 参数表格 + T4.3 历史参数回填
    auto* grpParams = new QGroupBox(tr("参数覆盖与回填（T4.3 副本）"));
    auto* vParams = new QVBoxLayout(grpParams);

    auto* hHistory = new QHBoxLayout;
    hHistory->addWidget(new QLabel(tr("历史 run：")));
    m_comboHistoryRun = new QComboBox;
    m_btnLoadParams = new QPushButton(tr("载回参数"));
    m_btnLoadParams->setEnabled(false);
    hHistory->addWidget(m_comboHistoryRun, 1);
    hHistory->addWidget(m_btnLoadParams);
    vParams->addLayout(hHistory);

    m_tableParams = new QTableWidget(0, 4);
    m_tableParams->setHorizontalHeaderLabels(
        {tr("文件"), tr("参数名"), tr("原值"), tr("新值")});
    m_tableParams->horizontalHeader()->setStretchLastSection(true);
    m_tableParams->setMinimumHeight(160);
    vParams->addWidget(m_tableParams);

    auto* hParamBtns = new QHBoxLayout;
    auto* btnAddRow = new QPushButton(tr("+ 添加参数"));
    auto* btnPreset = new QPushButton(tr("+ 常用参数预设"));
    auto* presetMenu = new QMenu(btnPreset);
    presetMenu->addAction(tr("next_goal_tolerance_normal (6 -> 1.0)"), this, [this]() {
        addParamRow("schedule_goal.yaml", "next_goal_tolerance_normal", "6", "1.0");
    });
    presetMenu->addAction(tr("next_goal_tolerance_increase (6 -> 1.0)"), this, [this]() {
        addParamRow("schedule_goal.yaml", "next_goal_tolerance_increase", "6", "1.0");
    });
    presetMenu->addAction(tr("PursuitLocalPlanner.lookahead_dist (1.5 -> 2.0)"), this, [this]() {
        addParamRow("sweep_base/local_planner_params.yaml", "PursuitLocalPlanner.lookahead_dist", "1.5", "2.0");
    });
    presetMenu->addAction(tr("PursuitLocalPlanner.fast_linear_vel (0.5 -> 0.6)"), this, [this]() {
        addParamRow("sweep_base/local_planner_params.yaml", "PursuitLocalPlanner.fast_linear_vel", "0.5", "0.6");
    });
    btnPreset->setMenu(presetMenu);
    auto* btnDelRow = new QPushButton(tr("- 删除选中"));
    hParamBtns->addWidget(btnAddRow);
    hParamBtns->addWidget(btnPreset);
    hParamBtns->addWidget(btnDelRow);
    hParamBtns->addStretch();
    vParams->addLayout(hParamBtns);
    mainLayout->addWidget(grpParams);

    // 3. 提交 + 停止 + 状态
    auto* hBottom = new QHBoxLayout;
    m_btnSubmit = new QPushButton(tr("提交复跑"));
    m_btnSubmit->setStyleSheet("QPushButton { background: #2563eb; color: white;"
                               " font-weight: bold; padding: 8px 24px; }");
    m_btnCancel = new QPushButton(tr("停止当前 run"));
    m_btnRefresh = new QPushButton(tr("刷新"));
    hBottom->addWidget(m_btnRefresh);
    hBottom->addWidget(m_btnCancel);
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
        fetchBranches(); fetchVenues(); fetchRuns();
    });
    connect(m_btnLoadParams, &QPushButton::clicked,
            this, &RegressionPanel::loadSelectedRunParams);
    connect(m_chkRunnableOnly, &QCheckBox::toggled, this, [this](bool) {
        fetchVenues();
    });
    connect(m_btnCancel, &QPushButton::clicked, this, &RegressionPanel::cancelActiveRun);
    connect(m_btnSubmit, &QPushButton::clicked, this, &RegressionPanel::submitRun);
    connect(m_comboBranch, &QComboBox::currentTextChanged,
            this, [this](const QString&) { updateSubmitState(); });
    connect(m_comboBag, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        if (idx >= 0) {
            const int dur = m_comboBag->itemData(idx, Qt::UserRole + 3).toInt();
            if (dur >= 10 && dur <= 600) m_spinDuration->setValue(dur);
        }
        updateSubmitState();
    });
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

void RegressionPanel::fetchRuns()
{
    QNetworkRequest req{QUrl(m_bridgeUrl + "/runs")};
    req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_nam->get(req);
}

void RegressionPanel::loadSelectedRunParams()
{
    const QString rid = m_comboHistoryRun ? m_comboHistoryRun->currentData().toString() : QString();
    if (rid.isEmpty()) {
        showError(tr("请先选择一条有效的历史 run"));
        return;
    }
    QNetworkRequest req{QUrl(m_bridgeUrl + "/runs/" + rid)};
    req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_nam->get(req);
    m_lblStatus->setText(tr("正在载入 %1 的参数…").arg(rid));
    m_lblStatus->setStyleSheet("color: blue;");
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

    // 参数覆盖（T4.3 传完整 param_changes，同时保留 param_overrides 兼容）
    auto parseJsonVal = [](const QString& s) -> QJsonValue {
        const QString t = s.trimmed();
        if (t.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) return QJsonValue(true);
        if (t.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0) return QJsonValue(false);
        bool ok = false;
        double d = t.toDouble(&ok);
        if (ok) return QJsonValue(d);
        return QJsonValue(t);
    };
    QJsonObject overrides;
    QJsonArray changesArr;
    for (const auto& r : paramChanges()) {
        const QJsonValue toVal = parseJsonVal(r.toVal);
        overrides[r.key] = toVal;
        QJsonObject ch;
        ch["file"] = r.file.trimmed();
        ch["key"] = r.key.trimmed();
        ch["from"] = r.fromVal.trimmed();
        ch["to"] = toVal;
        changesArr.append(ch);
    }
    if (!overrides.isEmpty())
        body["param_overrides"] = overrides;
    if (!changesArr.isEmpty())
        body["param_changes"] = changesArr;

    QNetworkRequest req{QUrl(m_bridgeUrl + "/runs")};
    req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_nam->post(req, QJsonDocument(body).toJson());

    m_lblStatus->setText(tr("提交中…"));
    m_lblStatus->setStyleSheet("color: blue;");
    m_btnSubmit->setEnabled(false);
}

void RegressionPanel::cancelActiveRun()
{
    QNetworkRequest req{QUrl(m_bridgeUrl + "/runs/cancel_active")};
    req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_nam->post(req, QByteArray("{}"));
    m_lblStatus->setText(tr("正在中止当前 run…"));
    m_lblStatus->setStyleSheet("color: #b45309;");
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
        m_venueDuration.clear();
        const QJsonArray arr = doc.array();
        const bool onlyRunnable = m_chkRunnableOnly && m_chkRunnableOnly->isChecked();
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            const QString vid = o["venue_id"].toString();
            const bool runnable = o.contains("runnable")
                                  ? o["runnable"].toBool() : true;
            m_venueRunnable[vid] = runnable;
            m_venueDuration[vid] = o.value("duration_s").toInt(60);
            if (onlyRunnable && !runnable)
                continue;
            QNetworkRequest req{QUrl(m_bridgeUrl + "/venues/" + vid + "/bags")};
            req.setRawHeader("X-Bridge-Token", m_token.toUtf8());
            m_nam->get(req);
        }
        updateSubmitState();
    }
    else if (url.contains("/venues/") && url.endsWith("/bags")) {
        const QJsonArray arr = doc.array();
        // 路径形如 /venues/{vid}/bags —— 必须用 section("/bags", 0, 0) 只取第一段
        const QString vid = url.section("/venues/", 1).section("/bags", 0, 0);
        const bool runnable = m_venueRunnable.value(vid, true);
        const int venueDur = m_venueDuration.value(vid, 60);
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            const QString bag = o["bag"].toString();
            const int dur = o.contains("duration_s") ? o["duration_s"].toInt(venueDur) : venueDur;
            const QString label = runnable ? bag
                                           : bag + tr("（场地缺 container，跑不了）");
            m_comboBag->addItem(label, bag);
            int idx = m_comboBag->count() - 1;
            m_comboBag->setItemData(idx, vid, Qt::UserRole + 1);
            m_comboBag->setItemData(idx, runnable, Qt::UserRole + 2);
            m_comboBag->setItemData(idx, dur, Qt::UserRole + 3);
            if (idx == 0 && dur >= 10 && dur <= 600)
                m_spinDuration->setValue(dur);
            if (!runnable) {
                m_comboBag->setItemData(idx, QColor(150, 150, 150), Qt::ForegroundRole);
            }
        }
        updateSubmitState();
    }
    else if (url.endsWith("/runs/cancel_active")) {
        m_lblStatus->setText(tr("已停止当前 run，可重新提交"));
        m_lblStatus->setStyleSheet("color: green;");
        m_btnSubmit->setEnabled(true);
        updateSubmitState();
        fetchRuns();
    }
    else if (url.endsWith("/runs") && reply->operation() == QNetworkAccessManager::GetOperation) {
        m_comboHistoryRun->clear();
        const QJsonArray arr = doc.array();
        for (const auto& v : arr) {
            const QJsonObject o = v.toObject();
            const QString rid = o["run_id"].toString();
            const QString st = o["state"].toString();
            const int cnt = o["param_changes_count"].toInt();
            m_comboHistoryRun->addItem(
                QStringLiteral("%1 [%2 | %3改参]").arg(rid, st).arg(cnt), rid);
        }
        const bool hasRuns = (m_comboHistoryRun->count() > 0);
        if (!hasRuns)
            m_comboHistoryRun->addItem(tr("（暂无历史 run）"), QString());
        if (m_btnLoadParams) m_btnLoadParams->setEnabled(hasRuns);
    }
    else if (url.endsWith("/runs") && reply->operation() == QNetworkAccessManager::PostOperation) {
        const QJsonObject o = doc.object();
        m_lastRunId = o["run_id"].toString();
        m_lblStatus->setText(tr("已提交 run_id=%1，正在自动连接直播…").arg(m_lastRunId));
        m_lblStatus->setStyleSheet("color: green;");
        emit runSubmitted(m_lastRunId);
        m_btnSubmit->setEnabled(true);
        fetchRuns();
    }
    else if (url.contains("/runs/")
             && !url.endsWith("/timeline")
             && !url.endsWith("/cancel")) {
        const QJsonObject o = doc.object();
        const QString rid = o["run_id"].toString();
        const QJsonObject reqObj = o["req"].toObject();

        // 同步分支与时长（方便直接基于历史 run 微调复跑）
        const QString br = reqObj["branch"].toString();
        if (!br.isEmpty()) {
            int bi = m_comboBranch->findData(br);
            if (bi >= 0) m_comboBranch->setCurrentIndex(bi);
        }
        const int dur = reqObj["duration_s"].toInt();
        if (dur >= 10 && dur <= 600) m_spinDuration->setValue(dur);

        auto valToStr = [](const QJsonValue& v) -> QString {
            if (v.isBool()) return v.toBool() ? QStringLiteral("true") : QStringLiteral("false");
            if (v.isDouble()) return QString::number(v.toDouble());
            if (v.isString()) return v.toString();
            return QString();
        };

        m_tableParams->setRowCount(0);
        int count = 0;
        QJsonArray changes = o["param_changes"].toArray();
        if (changes.isEmpty())
            changes = reqObj["param_changes"].toArray();
        if (!changes.isEmpty()) {
            for (const auto& item : changes) {
                const QJsonObject c = item.toObject();
                const QString key = c["key"].toString();
                if (key.isEmpty()) continue;
                addParamRow(c["file"].toString(), key,
                            valToStr(c["from"]), valToStr(c["to"]));
                ++count;
            }
        } else {
            const QJsonObject ov = reqObj["param_overrides"].toObject();
            for (auto it = ov.begin(); it != ov.end(); ++it) {
                addParamRow(QString(), it.key(), QStringLiteral("-"), valToStr(it.value()));
                ++count;
            }
        }
        m_lblStatus->setText(tr("已从 %1 载回 %2 项参数").arg(rid).arg(count));
        m_lblStatus->setStyleSheet("color: green;");
    }
    reply->deleteLater();
}
