#ifndef REGRESSION_PANEL_H
#define REGRESSION_PANEL_H

// T4.1 回归任务面板：选分支 → 选 bag → 改参数 → 提交 run → 看直播
// 数据来自 sim-bridge 的 REST API（/branches, /venues, /runs）

#include <QDialog>
#include <QString>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <functional>

class QComboBox;
class QTableWidget;
class QPushButton;
class QCheckBox;
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
    // T4.2 L2 分段重跑：注入从主窗口读取当前选中轨迹段 (fromIdx, toIdx, totalPoints) 的回调
    void setSegmentPicker(std::function<bool(int&, int&, int&)> picker);
    void syncSegmentFromMap(bool autoEnable = false);
    // 画布改轨迹直接复跑：注入从主窗口导出当前编辑轨迹 JSON 的回调
    void setTrajectoryExporter(std::function<QJsonObject()> exporter);

signals:
    void runSubmitted(const QString& runId);
    void statusMessage(const QString& msg);
    void newVenueRequested();

public slots:
    void refreshAll();

private slots:
    void fetchBranches();
    void fetchVenues();
    void fetchRuns();
    void loadSelectedRunParams();
    void submitRun();
    void cancelActiveRun();
    void openSelectedRunReport();
    void saveSelectedRunReport();
    void onNetworkFinished(QNetworkReply* reply);

private:
    void setupUi();
    void updateSubmitState();
    void updateSegmentUi();
    void showError(const QString& msg);
    void addParamRow(const QString& file, const QString& key,
                     const QString& from, const QString& to);
    QString currentHistoryRunId() const;
    void requestReport(int action);   // 1=浏览器打开 2=另存
    void finishReportRequest(const QByteArray& body);

    QString m_bridgeUrl;
    QString m_token;
    QString m_lastRunId;
    QList<BranchInfo> m_branches;
    QMap<QString, bool> m_venueRunnable;   // venue_id → 是否可跑（缺 container 则为 false）
    QMap<QString, int> m_venueDuration;    // venue_id → 默认时长（秒）

    QComboBox* m_comboBranch = nullptr;
    QComboBox* m_comboBag = nullptr;
    QComboBox* m_comboHistoryRun = nullptr;
    QCheckBox* m_chkRunnableOnly = nullptr;
    QCheckBox* m_chkUseCanvasTraj = nullptr;
    QCheckBox* m_chkSegment = nullptr;
    QSpinBox* m_spinSegFrom = nullptr;
    QSpinBox* m_spinSegTo = nullptr;
    QSpinBox* m_spinSegPad = nullptr;
    QPushButton* m_btnPickSegment = nullptr;
    QLabel* m_lblSegHint = nullptr;
    int m_segTotalPoints = 0;
    std::function<bool(int&, int&, int&)> m_segmentPicker;
    std::function<QJsonObject()> m_trajExporter;
    QTableWidget* m_tableParams = nullptr;
    QSpinBox* m_spinDuration = nullptr;
    QPushButton* m_btnLoadParams = nullptr;
    QPushButton* m_btnReport = nullptr;       // T5.1 浏览器打开对比报告
    QPushButton* m_btnSaveReport = nullptr;   // T5.1 另存报告 HTML
    int m_reportAction = 0;                   // 0=无 1=打开 2=另存
    QString m_reportRunId;
    QPushButton* m_btnSubmit = nullptr;
    QPushButton* m_btnCancel = nullptr;
    QPushButton* m_btnRefresh = nullptr;
    QLabel* m_lblStatus = nullptr;
    QNetworkAccessManager* m_nam = nullptr;
};

#endif // REGRESSION_PANEL_H
