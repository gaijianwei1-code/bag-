#ifndef VENUE_WIZARD_H
#define VENUE_WIZARD_H

// T1.2 场地档案向导：三步点选录入新场地（选地图 → 选点云 → 关联 bag/轨迹 + 即时校验）
// 保存时通过 sim-bridge 的 POST /venues（重名可确认 PUT 覆盖），免手写 YAML。

#include <QDialog>
#include <QString>
#include <QJsonObject>

class QStackedWidget;
class QLabel;
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QPushButton;
class QNetworkAccessManager;
class QNetworkReply;

class VenueWizard : public QDialog
{
    Q_OBJECT
public:
    explicit VenueWizard(const QString& bridgeUrl, const QString& token,
                         QWidget* parent = nullptr);
    ~VenueWizard() override = default;

    QString savedVenueId() const { return m_savedVenueId; }

signals:
    void venueSaved(const QString& venueId);

private slots:
    void browsePgm();
    void browsePcd();
    void browseTraj();
    void scanContainerAssets();
    void onPrevStep();
    void onNextOrSave();
    void onNetworkFinished(QNetworkReply* reply);

private:
    void setupUi();
    void updateStepUi();
    bool validateStep1(QString& errOut) const;
    bool validateStep2(QString& errOut) const;
    bool validateStep3(QString& errOut) const;
    void parseYamlNearPgm(const QString& pgmPath);
    void checkPcdAndOverlap();
    void checkTrajFile();
    QJsonObject buildVenueJson() const;
    void submitVenue(bool overwrite);

    QString m_bridgeUrl;
    QString m_token;
    QString m_savedVenueId;
    QString m_lastDir;
    QNetworkAccessManager* m_nam = nullptr;

    // 步进头 + 页面栈
    QLabel* m_lblStepBar = nullptr;
    QStackedWidget* m_stack = nullptr;
    QPushButton* m_btnPrev = nullptr;
    QPushButton* m_btnNext = nullptr;
    QLabel* m_lblFooterStatus = nullptr;

    // 第 1 步：基本信息 + 地图 (.pgm / .yaml)
    QLineEdit* m_editId = nullptr;
    QLineEdit* m_editName = nullptr;
    QLineEdit* m_editPgm = nullptr;
    QDoubleSpinBox* m_spinRes = nullptr;
    QDoubleSpinBox* m_spinOriginX = nullptr;
    QDoubleSpinBox* m_spinOriginY = nullptr;
    QLabel* m_lblMapCheck = nullptr;
    int m_pgmWidth = 0;
    int m_pgmHeight = 0;

    // 第 2 步：点云 (.pcd) + 高度过滤
    QLineEdit* m_editPcd = nullptr;
    QDoubleSpinBox* m_spinZmin = nullptr;
    QDoubleSpinBox* m_spinZmax = nullptr;
    QLabel* m_lblPcdCheck = nullptr;
    bool m_pcdOk = false;

    // 第 3 步：Bag / 轨迹 + 容器路径
    QLineEdit* m_editTraj = nullptr;
    QLineEdit* m_editBagRel = nullptr;
    QSpinBox* m_spinDuration = nullptr;
    QLineEdit* m_editParamSet = nullptr;
    QLabel* m_lblTrajCheck = nullptr;
    QComboBox* m_comboContainerMapDir = nullptr;
    QComboBox* m_comboContainerBag = nullptr;
    QComboBox* m_comboContainerTraj = nullptr;
    QComboBox* m_comboContainerWorld = nullptr;
    QPushButton* m_btnScanContainer = nullptr;
    QLabel* m_lblContainerHint = nullptr;
};

#endif // VENUE_WIZARD_H
