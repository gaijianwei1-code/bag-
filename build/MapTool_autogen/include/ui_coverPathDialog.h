/********************************************************************************
** Form generated from reading UI file 'coverPathDialog.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_COVERPATHDIALOG_H
#define UI_COVERPATHDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>

QT_BEGIN_NAMESPACE

class Ui_CoverPathDialog
{
public:
    QDialogButtonBox *buttonOK;
    QPushButton *pushButton_coverPath_pre;
    QLabel *labelPathWidth;
    QSpinBox *pathWidthSpinBox;
    QLabel *labelMinTurnRadius;
    QDoubleSpinBox *minTurnRaiusSpinBox;
    QLabel *labelPathType;
    QComboBox *comboBoxPathType;
    QLabel *labelStartPointF;
    QLabel *labelStartPointF_data;
    QPushButton *pushButton_selectPoint;
    QLabel *labelInitDirection;
    QDoubleSpinBox *SpinBox_initDirection;

    void setupUi(QDialog *CoverPathDialog)
    {
        if (CoverPathDialog->objectName().isEmpty())
            CoverPathDialog->setObjectName("CoverPathDialog");
        CoverPathDialog->resize(410, 240);
        buttonOK = new QDialogButtonBox(CoverPathDialog);
        buttonOK->setObjectName("buttonOK");
        buttonOK->setGeometry(QRect(250, 150, 130, 80));
        buttonOK->setOrientation(Qt::Vertical);
        buttonOK->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        pushButton_coverPath_pre = new QPushButton(CoverPathDialog);
        pushButton_coverPath_pre->setObjectName("pushButton_coverPath_pre");
        pushButton_coverPath_pre->setGeometry(QRect(250, 105, 130, 30));
        labelPathWidth = new QLabel(CoverPathDialog);
        labelPathWidth->setObjectName("labelPathWidth");
        labelPathWidth->setGeometry(QRect(30, 60, 100, 40));
        labelPathWidth->setAlignment(Qt::AlignCenter);
        pathWidthSpinBox = new QSpinBox(CoverPathDialog);
        pathWidthSpinBox->setObjectName("pathWidthSpinBox");
        pathWidthSpinBox->setGeometry(QRect(130, 65, 100, 30));
        pathWidthSpinBox->setAlignment(Qt::AlignCenter);
        pathWidthSpinBox->setMinimum(1);
        pathWidthSpinBox->setMaximum(9999);
        pathWidthSpinBox->setSingleStep(5);
        pathWidthSpinBox->setValue(140);
        labelMinTurnRadius = new QLabel(CoverPathDialog);
        labelMinTurnRadius->setObjectName("labelMinTurnRadius");
        labelMinTurnRadius->setGeometry(QRect(30, 100, 100, 40));
        labelMinTurnRadius->setAlignment(Qt::AlignCenter);
        minTurnRaiusSpinBox = new QDoubleSpinBox(CoverPathDialog);
        minTurnRaiusSpinBox->setObjectName("minTurnRaiusSpinBox");
        minTurnRaiusSpinBox->setGeometry(QRect(130, 105, 100, 30));
        minTurnRaiusSpinBox->setFocusPolicy(Qt::StrongFocus);
        minTurnRaiusSpinBox->setAlignment(Qt::AlignCenter);
        minTurnRaiusSpinBox->setDecimals(2);
        minTurnRaiusSpinBox->setMinimum(0.000000000000000);
        minTurnRaiusSpinBox->setMaximum(10000.000000000000000);
        minTurnRaiusSpinBox->setSingleStep(0.100000000000000);
        minTurnRaiusSpinBox->setValue(1.000000000000000);
        labelPathType = new QLabel(CoverPathDialog);
        labelPathType->setObjectName("labelPathType");
        labelPathType->setGeometry(QRect(30, 140, 100, 40));
        labelPathType->setAlignment(Qt::AlignCenter);
        comboBoxPathType = new QComboBox(CoverPathDialog);
        comboBoxPathType->addItem(QString());
        comboBoxPathType->setObjectName("comboBoxPathType");
        comboBoxPathType->setGeometry(QRect(130, 145, 100, 30));
        labelStartPointF = new QLabel(CoverPathDialog);
        labelStartPointF->setObjectName("labelStartPointF");
        labelStartPointF->setGeometry(QRect(30, 20, 100, 40));
        labelStartPointF->setAlignment(Qt::AlignCenter);
        labelStartPointF_data = new QLabel(CoverPathDialog);
        labelStartPointF_data->setObjectName("labelStartPointF_data");
        labelStartPointF_data->setGeometry(QRect(115, 20, 180, 40));
        labelStartPointF_data->setAlignment(Qt::AlignCenter);
        pushButton_selectPoint = new QPushButton(CoverPathDialog);
        pushButton_selectPoint->setObjectName("pushButton_selectPoint");
        pushButton_selectPoint->setGeometry(QRect(300, 25, 80, 30));
        labelInitDirection = new QLabel(CoverPathDialog);
        labelInitDirection->setObjectName("labelInitDirection");
        labelInitDirection->setGeometry(QRect(30, 180, 100, 40));
        labelInitDirection->setAlignment(Qt::AlignCenter);
        SpinBox_initDirection = new QDoubleSpinBox(CoverPathDialog);
        SpinBox_initDirection->setObjectName("SpinBox_initDirection");
        SpinBox_initDirection->setEnabled(true);
        SpinBox_initDirection->setGeometry(QRect(130, 185, 100, 30));
        SpinBox_initDirection->setFocusPolicy(Qt::StrongFocus);
        SpinBox_initDirection->setAlignment(Qt::AlignCenter);
        SpinBox_initDirection->setDecimals(2);
        SpinBox_initDirection->setMinimum(0.000000000000000);
        SpinBox_initDirection->setMaximum(360.000000000000000);
        SpinBox_initDirection->setSingleStep(1.000000000000000);
        SpinBox_initDirection->setValue(45.000000000000000);

        retranslateUi(CoverPathDialog);

        QMetaObject::connectSlotsByName(CoverPathDialog);
    } // setupUi

    void retranslateUi(QDialog *CoverPathDialog)
    {
        CoverPathDialog->setWindowTitle(QCoreApplication::translate("CoverPathDialog", "\350\246\206\347\233\226\345\274\217\350\275\250\350\277\271\345\261\236\346\200\247\350\256\276\345\256\232", nullptr));
        pushButton_coverPath_pre->setText(QCoreApplication::translate("CoverPathDialog", "\351\242\204\350\256\241\350\246\206\347\233\226\345\214\272\345\237\237", nullptr));
        labelPathWidth->setText(QCoreApplication::translate("CoverPathDialog", "<html><head/><body><p><span style=\" font-size:11pt;\">\350\275\250\350\277\271\345\256\275\345\272\246=</span></p></body></html>", nullptr));
        pathWidthSpinBox->setSuffix(QCoreApplication::translate("CoverPathDialog", "cm", nullptr));
        labelMinTurnRadius->setText(QCoreApplication::translate("CoverPathDialog", "<html><head/><body><p><span style=\" font-size:11pt;\">\350\275\254\345\274\257\345\215\212\345\276\204=</span></p></body></html>", nullptr));
        minTurnRaiusSpinBox->setSuffix(QCoreApplication::translate("CoverPathDialog", "m", nullptr));
        labelPathType->setText(QCoreApplication::translate("CoverPathDialog", "<html><head/><body><p><span style=\" font-size:11pt;\">\350\210\252\345\220\221\350\247\204\345\210\222=</span></p></body></html>", nullptr));
        comboBoxPathType->setItemText(0, QCoreApplication::translate("CoverPathDialog", "\351\235\240\345\217\263", nullptr));

        labelStartPointF->setText(QCoreApplication::translate("CoverPathDialog", "<html><head/><body><p><span style=\" font-size:11pt;\">\350\265\267\345\247\213\344\275\215\347\275\256=</span></p></body></html>", nullptr));
        labelStartPointF_data->setText(QCoreApplication::translate("CoverPathDialog", "<html><head/><body><p><span style=\" font-size:10pt;\">\357\274\210x\357\274\214y\357\274\211</span></p></body></html>", nullptr));
        pushButton_selectPoint->setText(QCoreApplication::translate("CoverPathDialog", "\351\200\211\347\202\271", nullptr));
        labelInitDirection->setText(QCoreApplication::translate("CoverPathDialog", "<html><head/><body><p><span style=\" font-size:11pt;\">\345\210\235\345\247\213\346\226\271\345\220\221=</span></p></body></html>", nullptr));
        SpinBox_initDirection->setSuffix(QCoreApplication::translate("CoverPathDialog", "\302\260", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CoverPathDialog: public Ui_CoverPathDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_COVERPATHDIALOG_H
