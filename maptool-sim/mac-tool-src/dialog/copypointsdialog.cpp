#include "copypointsdialog.h"
#include "ui_copypointsdialog.h"

copyPointsDialog::copyPointsDialog(QWidget *parent, qreal reso) :
    QDialog(parent),
    resolution(reso),
    ui(new Ui::copyPointsDialog)
{
    has_basePoint = false;
    ui->setupUi(this);
}

copyPointsDialog::~copyPointsDialog()
{
    delete ui;
}

void copyPointsDialog::on_pushButton_first_clicked()
{
    emit selectBasePoint();
}

void copyPointsDialog::on_pushButton_second_clicked()
{
    emit selectOffsetPoint();
}

void copyPointsDialog::setLableDirection(double x)
{
    ui->lineEdit_val_direction->setText(QString::number(x));
}

void copyPointsDialog::setLableDistance(double y)
{
    ui->lineEdit_val_distance->setText(QString::number(y));
}

void copyPointsDialog::setApplyButtonText(const QString& text)
{
    ui->pushButton_apply_copy->setText(text);
}

void copyPointsDialog::setBasePoint(QPointF baseP)
{
    basePoint = baseP;
    has_basePoint = true;
    setLableDirection(0.0);
    setLableDistance(0.0);
}

void copyPointsDialog::setOffsetPoint(QPointF offsetP)
{
    if ( !has_basePoint )
    {
        QMessageBox::critical(this, tr("失败"), tr("请先选择基准点！"));
        return;
    }
    offsetPoint = offsetP;
    offset = offsetPoint - basePoint;

    qreal off_dir, off_dis;
    off_dir = atan2(offset.ry(), offset.rx()) * 180.0 / M_PI;
    off_dis = sqrt(offset.x() * offset.x() + offset.y() * offset.y()) * 100.0 * resolution;
    setLableDirection(off_dir);
    setLableDistance(off_dis);
}

void copyPointsDialog::on_pushButton_apply_copy_clicked()
{
    bool getNumOk;
    qreal _dir, _dis;
    _dir = ui->lineEdit_val_direction->text().toDouble(&getNumOk);
    if (!getNumOk)
    {
        QMessageBox::critical(this, tr("失败"), tr("请确认移动方向的值为实数类型！"));
        return;
    }
    _dis = ui->lineEdit_val_distance->text().toDouble(&getNumOk);
    if (!getNumOk)
    {
        QMessageBox::critical(this, tr("失败"), tr("请确认移动距离的值为实数类型！"));
        return;
    }
    _dir *= M_PI / 180.0;
    _dis /= 100.0 * resolution;
    offset.rx() = _dis * cos(_dir);
    offset.ry() = _dis * sin(_dir);
    emit apply_pointsOffset_copy(offset);
//    this->close();
}

