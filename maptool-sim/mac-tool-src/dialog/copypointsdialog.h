#ifndef COPYPOINTSDIALOG_H
#define COPYPOINTSDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <cmath>

namespace Ui {
class copyPointsDialog;
}

//class MainWindow;  // 前向声明 MainWindow 类

class copyPointsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit copyPointsDialog(QWidget *parent = nullptr, qreal reso = 0.05);
    ~copyPointsDialog();

    void setLableDirection(double);
    void setLableDistance(double);
    void setApplyButtonText(const QString&);

signals:
    void selectBasePoint();
    void selectOffsetPoint();
    void apply_pointsOffset_copy(QPointF);

public slots:
    void setBasePoint(QPointF);
    void setOffsetPoint(QPointF);

private slots:
    void on_pushButton_first_clicked();
    void on_pushButton_second_clicked();

    void on_pushButton_apply_copy_clicked();

private:
    Ui::copyPointsDialog *ui;
    bool has_basePoint;
    QPointF basePoint;
    QPointF offsetPoint;
    QPointF offset;
    qreal resolution;

    // 声明 MainWindow 为友元类，MainWindow 可以访问 copyPointsDialog 的私有成员
//    friend class MainWindow;
};

#endif // COPYPOINTSDIALOG_H
