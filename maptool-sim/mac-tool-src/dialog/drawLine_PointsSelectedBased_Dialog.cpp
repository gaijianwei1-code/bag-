#include "drawLine_PointsSelectedBased_Dialog.h"
#include "ui_drawLine_PointsSelectedBased_Dialog.h"

DrawLine_PointsSelectedBased_Dialog::DrawLine_PointsSelectedBased_Dialog(QWidget *parent, qreal reso) :
    QDialog(parent),
    ui(new Ui::DrawLine_PointsSelectedBased_Dialog),
    resolution(reso)
{
    ui->setupUi(this);
//    ui->spinBox_lineWidth->setSingleStep(100.0 * resolution);
    qDebug() << "250112#resolution:" << resolution << ' ' << 100.0 * resolution;
}

DrawLine_PointsSelectedBased_Dialog::~DrawLine_PointsSelectedBased_Dialog()
{
    delete ui;
}

void DrawLine_PointsSelectedBased_Dialog::on_spinBox_offsetDistance_valueChanged(int arg1)
{
    ui->label_offsetDistance_val->setText(QString("(%1px)").arg(arg1 / 100.0 / resolution, 0, 'f', 2));
}


void DrawLine_PointsSelectedBased_Dialog::on_spinBox_extendLength_valueChanged(int arg1)
{
    ui->label_extendLength_val->setText(QString("(%1px)").arg((arg1 / 100.0 / resolution), 0, 'f', 2));
}


void DrawLine_PointsSelectedBased_Dialog::on_spinBox_lineWidth_valueChanged(int arg1)
{
    ui->label_lineWidth_val->setText(QString("(%1px)").arg(arg1 / 100.0 / resolution, 0, 'f', 2));
}


void DrawLine_PointsSelectedBased_Dialog::on_buttonBox_accepted()
{
    offsetDirection = ui->comboBox_offsetDirection->currentIndex(); //0-右侧, 1-左侧
    offsetDistance = ui->spinBox_offsetDistance->value();
    extendLength = ui->spinBox_extendLength->value();
    curveClosure = ui->checkBox_curveClosure->isChecked();
    lineWidth = ui->spinBox_lineWidth->value();
    emit drawLine_PointsSelectedBased_signal(offsetDirection, offsetDistance, extendLength, curveClosure, lineWidth);
}

