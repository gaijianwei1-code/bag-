#include "RosbagPlayDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QMessageBox>
#include <QRegularExpression>
#include <QListWidgetItem>

RosbagPlayDialog::RosbagPlayDialog(const QString &bagFilePath, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("选择 rosbag 播放参数");
    resize(400, 500);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 速率选择布局
    QHBoxLayout *rateLayout = new QHBoxLayout;
    QLabel *rateLabel = new QLabel("播放倍速:");
    rateCombo = new QComboBox;
    rateCombo->addItem("0.2×", 0.2);
    rateCombo->addItem("0.5×", 0.5);
    rateCombo->addItem("1×",   1.0);
    rateCombo->addItem("2×",   2.0);
    rateCombo->addItem("3×",   3.0);
    rateCombo->addItem("5×",   5.0);
    rateCombo->addItem("10×",  10.0);
    rateCombo->setCurrentIndex(2); // 默认 1×

    rateLayout->addWidget(rateLabel);
    rateLayout->addWidget(rateCombo);
    mainLayout->addLayout(rateLayout);

    // 主题列表
    topicList = new QListWidget;
    topicList->setSelectionMode(QAbstractItemView::NoSelection);
    mainLayout->addWidget(new QLabel("请选择要播放的 topic:"));
    mainLayout->addWidget(topicList, 1);

    // 确认 / 取消 按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RosbagPlayDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &RosbagPlayDialog::reject);

    // 使用 rosbag info 提取 topic 列表
    QProcess p(this);
    p.start("bash", QStringList() << "-c"
                                  << QString("source ~/.bashrc && rosbag info \"%1\"")
                                         .arg(bagFilePath));
    if (!p.waitForFinished(10000)) {
        QMessageBox::warning(this, "错误", "读取 rosbag info 超时");
        return;
    }

    QString output = p.readAllStandardOutput();
    QRegularExpression re("^\\s+(\\/[^\\s]+)\\s+\\d+\\s+msgs", QRegularExpression::MultilineOption);
    auto it = re.globalMatch(output);
    while (it.hasNext()) {
        auto match = it.next();
        QString topic = match.captured(1);
        QListWidgetItem *item = new QListWidgetItem(topic, topicList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }

}

RosbagPlayDialog::~RosbagPlayDialog()
{
    // 所有控件 Qt 自动清理，无需手动 delete
}

QStringList RosbagPlayDialog::selectedTopics() const
{
    QStringList sel;
    for (int i = 0; i < topicList->count(); ++i) {
        auto *item = topicList->item(i);
        if (item->checkState() == Qt::Checked) {
            sel << item->text();
        }
    }
    return sel;
}

double RosbagPlayDialog::playRate() const
{
    return rateCombo->currentData().toDouble();
}
