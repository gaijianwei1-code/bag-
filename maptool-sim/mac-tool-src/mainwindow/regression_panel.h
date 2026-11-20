#ifndef REGRESSION_PANEL_H
#define REGRESSION_PANEL_H

// T4.1 回归任务面板：选分支 → 选 bag → 改参数 → 提交 run → 看直播
// 数据来自 sim-bridge 的 REST API（/branches, /venues, /runs）

#include <QDialog>
#include <QString>
#include <QList>
#include <QMap>

class QComboBox;
class QTableWidget;
class QPushButton;
class QLabel;
class QLineEdit;
class QSpinBox;
class QNetworkAccessManager;
class QNetworkReply;

namespace Ui { class RegressionPanel; }

struct BranchInfo {
    QString branch;
    QString commit;
    bool isCurrent = false;
};

struct ParamRow {
    QString file;
    QString key;
    QString fromVal;
    QString toVal;
};

class RegressionPanel : public QDialog
{
    Q_OBJECT
public:
    explicit RegressionPanel(const QString& bridgeUrl, const QString& token,
                             QWidget* parent = nullptr);
    ~RegressionPanel() override;

    QString selectedBranch() const;
    QString selectedBag() const;
    QList<ParamRow> paramChanges() const;
    int durationSeconds() const;
    QString runId() const { return m_lastRunId; }

signals:
    void runSubmitted(const QString& runId);
    void statusMessage(const QString& msg);

private slots:
    void fetchBranches();
    void fetchVenues();
    void submitRun();
    void onNetworkFinished(QNetworkReply* reply);

private:
    void setupUi();
    void updateSubmitState();
    void showError(const QString& msg);
    void addParamRow(const QString& file, const QString& key,
                     const QString& from, const QString& to);

    QString m_bridgeUrl;
    QString m_token;
    QString m_lastRunId;
    QList<BranchInfo> m_branches;
    QMap<QString, bool> m_venueRunnable;   // venue_id → 是否可跑（缺 container 则为 false）

    QComboBox* m_comboBranch = nullptr;
    QComboBox* m_comboBag = nullptr;
    QTableWidget* m_tableParams = nullptr;
    QSpinBox* m_spinDuration = nullptr;
    QPushButton* m_btnSubmit = nullptr;
    QPushButton* m_btnRefresh = nullptr;
    QLabel* m_lblStatus = nullptr;
    QNetworkAccessManager* m_nam = nullptr;
};

#endif // REGRESSION_PANEL_H
