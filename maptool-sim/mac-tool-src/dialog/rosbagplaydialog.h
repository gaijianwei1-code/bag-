#ifndef ROSBAGPLAYDIALOG_H
#define ROSBAGPLAYDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QComboBox>
#include <QDialogButtonBox>

class RosbagPlayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RosbagPlayDialog(const QString &bagFilePath, QWidget *parent = nullptr);
    ~RosbagPlayDialog();

    QStringList selectedTopics() const;
    double playRate() const;

private:
    QListWidget *topicList;
    QComboBox *rateCombo;
};

#endif // ROSBAGPLAYDIALOG_H
