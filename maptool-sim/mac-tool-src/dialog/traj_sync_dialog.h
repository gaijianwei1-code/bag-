#ifndef TRAJ_SYNC_DIALOG_H
#define TRAJ_SYNC_DIALOG_H

#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <functional>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class TrajSyncPreviewWidget;

namespace blz_sync {

enum class AlignMode {
    AutoStartFixedLen = 0,   // 自动匹配最近起点 + 等长替换（固定点数）
    AutoStartEnd      = 1,   // 自动匹配最近起终点（完整分叉段，可增减点数）
    ManualStartFixed  = 2,   // 手动指定目标起始下标（固定点数）
    SameAsSource      = 3    // 与源相同下标 [srcStart..srcEnd]
};

struct SyncPlan {
    bool valid = false;
    QString error;
    int dstStart = -1;
    int dstEnd = -1;
    int srcCount = 0;
    int dstCount = 0;
    double startMatchDist = 0.0;
    double endMatchDist = 0.0;
    double joinPrevDist = 0.0;
    double joinNextDist = 0.0;
};

bool loadTrajectoryJson(const QString& path, QJsonObject& rootOut, QString& errOut);
bool saveTrajectoryJson(const QString& path, const QJsonObject& root,
                        bool makeBackup, QString& errOut);

int findNearestPointIndex(const QJsonArray& pts, double x, double y,
                          int minIdx, double* distOut = nullptr);

SyncPlan computeSyncPlan(const QJsonArray& srcPts, int srcStart, int srcEnd,
                         const QJsonArray& dstPts, AlignMode mode,
                         int manualDstStart = 0);

bool applySyncPlan(const QJsonArray& srcPts, int srcStart, int srcEnd,
                   const QJsonObject& dstRoot, const SyncPlan& plan,
                   bool syncPos, bool syncAttr,
                   QJsonObject& outRoot, QString& errOut);

} // namespace blz_sync

class TrajSyncDialog : public QDialog
{
    Q_OBJECT

public:
    using SegmentPicker = std::function<bool(int& fromIdx, int& toIdx, int& totalPoints)>;
    using CanvasExporter = std::function<QJsonObject()>;

    explicit TrajSyncDialog(QWidget* parent = nullptr);
    ~TrajSyncDialog() override = default;

    void setDefaultDir(const QString& dir);
    void setSegmentPicker(SegmentPicker picker);
    void setCanvasExporter(CanvasExporter exporter);
    void setSourceFileIfEmpty(const QString& jsonPath);

private slots:
    void browseSourceFile();
    void loadSourceFromCanvas();
    void pickRangeFromCanvas();
    void addTargetFiles();
    void removeSelectedTargets();
    void clearTargets();
    void applyDefaultModeToAll();
    void recomputeAllPlans();
    void onTargetSelectionChanged();
    void executeSync();

private:
    struct TargetEntry {
        QString filePath;
        QJsonObject root;
        QJsonArray points;
        blz_sync::AlignMode mode = blz_sync::AlignMode::AutoStartFixedLen;
        int manualStart = 0;
        blz_sync::SyncPlan plan;
        QString resultNote;
        bool synced = false;
    };

    void setupUi();
    void loadSourceObject(const QJsonObject& root, const QString& label);
    void updateSourceRangeInfo();
    void refreshTargetTable();
    void updateRowPlan(int row);
    void updatePreviewForRow(int row);
    QString currentBrowseDir() const;

    QString m_defaultDir;
    SegmentPicker m_segmentPicker;
    CanvasExporter m_canvasExporter;

    QJsonObject m_srcRoot;
    QJsonArray m_srcPoints;
    bool m_srcFromCanvas = false;
    QVector<TargetEntry> m_targets;
    bool m_updatingTable = false;

    // 源控件
    QLineEdit* m_editSrcPath = nullptr;
    QPushButton* m_btnSrcBrowse = nullptr;
    QPushButton* m_btnSrcCanvas = nullptr;
    QLabel* m_lblSrcTotal = nullptr;
    QSpinBox* m_spinSrcStart = nullptr;
    QSpinBox* m_spinSrcEnd = nullptr;
    QPushButton* m_btnPickCanvasRange = nullptr;
    QLabel* m_lblSrcInfo = nullptr;

    // 同步选项
    QCheckBox* m_chkSyncPos = nullptr;
    QCheckBox* m_chkSyncAttr = nullptr;
    QCheckBox* m_chkBackup = nullptr;
    QComboBox* m_comboDefaultMode = nullptr;

    // 目标列表与预览
    QTableWidget* m_table = nullptr;
    TrajSyncPreviewWidget* m_preview = nullptr;
    QLabel* m_lblPreviewInfo = nullptr;
    QLabel* m_lblStatus = nullptr;
    QPushButton* m_btnExecute = nullptr;
};

#endif // TRAJ_SYNC_DIALOG_H
