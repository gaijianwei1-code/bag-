#ifndef DRAWLINE_POINTSSELECTEDBASED_DIALOG_H
#define DRAWLINE_POINTSSELECTEDBASED_DIALOG_H

#include <QDialog>
#include <QDebug>

namespace Ui {
class DrawLine_PointsSelectedBased_Dialog;
}

class DrawLine_PointsSelectedBased_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit DrawLine_PointsSelectedBased_Dialog(QWidget *parent = nullptr, qreal reso = 0.05);
    ~DrawLine_PointsSelectedBased_Dialog();

private slots:
    void on_spinBox_offsetDistance_valueChanged(int arg1);

    void on_spinBox_extendLength_valueChanged(int arg1);

    void on_spinBox_lineWidth_valueChanged(int arg1);

    void on_buttonBox_accepted();

signals:
    void drawLine_PointsSelectedBased_signal(int, int, int, bool, int);

private:
    Ui::DrawLine_PointsSelectedBased_Dialog *ui;
    qreal resolution;
    int offsetDirection;
    int offsetDistance;
    int extendLength;
    bool curveClosure;
    int lineWidth;
};

#endif // DRAWLINE_POINTSSELECTEDBASED_DIALOG_H
/*
    <widget class="QLabel" name="label_offsetDirection">
    <widget class="QComboBox" name="comboBox_offsetDirection">

    <widget class="QLabel" name="label_offsetDistance">
    <widget class="QSpinBox" name="spinBox_offsetDistance">
    <widget class="QLabel" name="label_offsetDistance_val">

    <widget class="QLabel" name="label_extendLength">
    <widget class="QSpinBox" name="spinBox_extendLength">
    <widget class="QLabel" name="label_extendLength_val">

    <widget class="QCheckBox" name="checkBox_curveClosure">

    <widget class="QLabel" name="label_lineWidth">
    <widget class="QSpinBox" name="spinBox_lineWidth">
    <widget class="QLabel" name="label_lineWidth_val">

    <widget class="QDialogButtonBox" name="buttonBox">
*/
