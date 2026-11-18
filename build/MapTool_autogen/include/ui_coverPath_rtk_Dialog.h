/********************************************************************************
** Form generated from reading UI file 'coverPath_rtk_Dialog.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_COVERPATH_RTK_DIALOG_H
#define UI_COVERPATH_RTK_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>

QT_BEGIN_NAMESPACE

class Ui_CoverPath_Rtk_Dialog
{
public:
    QLabel *labelPathType;
    QLabel *labelPathWidth;
    QLabel *labelMinTurnRadius;
    QComboBox *comboBoxPathType;
    QDoubleSpinBox *minTurnRaiusSpinBox;
    QSpinBox *pathWidthSpinBox;
    QDialogButtonBox *buttonOK;
    QRadioButton *radioButton_round;
    QRadioButton *radioButton_forthAndBack;
    QLabel *label_mapAccuracy;
    QSpinBox *SpinBox_mapAccuracy;

    void setupUi(QDialog *CoverPath_Rtk_Dialog)
    {
        if (CoverPath_Rtk_Dialog->objectName().isEmpty())
            CoverPath_Rtk_Dialog->setObjectName("CoverPath_Rtk_Dialog");
        CoverPath_Rtk_Dialog->resize(410, 250);
        labelPathType = new QLabel(CoverPath_Rtk_Dialog);
        labelPathType->setObjectName("labelPathType");
        labelPathType->setGeometry(QRect(30, 140, 100, 40));
        labelPathType->setAlignment(Qt::AlignCenter);
        labelPathWidth = new QLabel(CoverPath_Rtk_Dialog);
        labelPathWidth->setObjectName("labelPathWidth");
        labelPathWidth->setGeometry(QRect(30, 60, 100, 40));
        labelPathWidth->setAlignment(Qt::AlignCenter);
        labelMinTurnRadius = new QLabel(CoverPath_Rtk_Dialog);
        labelMinTurnRadius->setObjectName("labelMinTurnRadius");
        labelMinTurnRadius->setGeometry(QRect(30, 100, 100, 40));
        labelMinTurnRadius->setAlignment(Qt::AlignCenter);
        comboBoxPathType = new QComboBox(CoverPath_Rtk_Dialog);
        comboBoxPathType->addItem(QString());
        comboBoxPathType->setObjectName("comboBoxPathType");
        comboBoxPathType->setGeometry(QRect(130, 145, 100, 30));
        minTurnRaiusSpinBox = new QDoubleSpinBox(CoverPath_Rtk_Dialog);
        minTurnRaiusSpinBox->setObjectName("minTurnRaiusSpinBox");
        minTurnRaiusSpinBox->setGeometry(QRect(130, 105, 100, 30));
        minTurnRaiusSpinBox->setFocusPolicy(Qt::StrongFocus);
        minTurnRaiusSpinBox->setAlignment(Qt::AlignCenter);
        minTurnRaiusSpinBox->setDecimals(2);
        minTurnRaiusSpinBox->setMinimum(0.000000000000000);
        minTurnRaiusSpinBox->setMaximum(10000.000000000000000);
        minTurnRaiusSpinBox->setSingleStep(0.100000000000000);
        minTurnRaiusSpinBox->setValue(1.000000000000000);
        pathWidthSpinBox = new QSpinBox(CoverPath_Rtk_Dialog);
        pathWidthSpinBox->setObjectName("pathWidthSpinBox");
        pathWidthSpinBox->setGeometry(QRect(130, 65, 100, 30));
        pathWidthSpinBox->setAlignment(Qt::AlignCenter);
        pathWidthSpinBox->setMinimum(1);
        pathWidthSpinBox->setMaximum(9999);
        pathWidthSpinBox->setSingleStep(5);
        pathWidthSpinBox->setValue(140);
        buttonOK = new QDialogButtonBox(CoverPath_Rtk_Dialog);
        buttonOK->setObjectName("buttonOK");
        buttonOK->setGeometry(QRect(250, 130, 130, 80));
        buttonOK->setOrientation(Qt::Vertical);
        buttonOK->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        radioButton_round = new QRadioButton(CoverPath_Rtk_Dialog);
        radioButton_round->setObjectName("radioButton_round");
        radioButton_round->setGeometry(QRect(90, 20, 95, 30));
        radioButton_round->setChecked(true);
        radioButton_forthAndBack = new QRadioButton(CoverPath_Rtk_Dialog);
        radioButton_forthAndBack->setObjectName("radioButton_forthAndBack");
        radioButton_forthAndBack->setGeometry(QRect(220, 20, 95, 30));
        radioButton_forthAndBack->setCheckable(true);
        label_mapAccuracy = new QLabel(CoverPath_Rtk_Dialog);
        label_mapAccuracy->setObjectName("label_mapAccuracy");
        label_mapAccuracy->setGeometry(QRect(30, 180, 100, 40));
        label_mapAccuracy->setAlignment(Qt::AlignCenter);
        SpinBox_mapAccuracy = new QSpinBox(CoverPath_Rtk_Dialog);
        SpinBox_mapAccuracy->setObjectName("SpinBox_mapAccuracy");
        SpinBox_mapAccuracy->setGeometry(QRect(130, 185, 100, 30));
        SpinBox_mapAccuracy->setAlignment(Qt::AlignCenter);
        SpinBox_mapAccuracy->setMinimum(5);
        SpinBox_mapAccuracy->setMaximum(9999);
        SpinBox_mapAccuracy->setSingleStep(5);
        SpinBox_mapAccuracy->setValue(30);

        retranslateUi(CoverPath_Rtk_Dialog);

        QMetaObject::connectSlotsByName(CoverPath_Rtk_Dialog);
    } // setupUi

    void retranslateUi(QDialog *CoverPath_Rtk_Dialog)
    {
        CoverPath_Rtk_Dialog->setWindowTitle(QCoreApplication::translate("CoverPath_Rtk_Dialog", "Dialog", nullptr));
        labelPathType->setText(QCoreApplication::translate("CoverPath_Rtk_Dialog", "<html><head/><body><p><span style=\" font-size:11pt;\">\350\210\252\345\220\221\350\247\204\345\210\222=</span></p></body></html>", nullptr));
        labelPathWidth->setText(QCoreApplication::translate("CoverPath_Rtk_Dialog", "<html><head/><body><p><span style=\" font-size:11pt;\">\350\275\250\350\277\271\345\256\275\345\272\246=</span></p></body></html>", nullptr));
        labelMinTurnRadius->setText(QCoreApplication::translate("CoverPath_Rtk_Dialog", "<html><head/><body><p><span style=\" font-size:11pt;\">\350\275\254\345\274\257\345\215\212\345\276\204=</span></p></body></html>", nullptr));
        comboBoxPathType->setItemText(0, QCoreApplication::translate("CoverPath_Rtk_Dialog", "\351\235\240\345\217\263", nullptr));

        minTurnRaiusSpinBox->setSuffix(QCoreApplication::translate("CoverPath_Rtk_Dialog", "m", nullptr));
        pathWidthSpinBox->setSuffix(QCoreApplication::translate("CoverPath_Rtk_Dialog", "cm", nullptr));
        radioButton_round->setText(QCoreApplication::translate("CoverPath_Rtk_Dialog", "\347\216\257\345\275\242", nullptr));
        radioButton_forthAndBack->setText(QCoreApplication::translate("CoverPath_Rtk_Dialog", "\345\274\223\345\275\242", nullptr));
        label_mapAccuracy->setText(QCoreApplication::translate("CoverPath_Rtk_Dialog", "<html><head/><body><p><span style=\" font-size:11pt;\">\345\234\260\345\233\276\347\262\276\345\272\246=</span></p></body></html>", nullptr));
        SpinBox_mapAccuracy->setSuffix(QCoreApplication::translate("CoverPath_Rtk_Dialog", "px/w", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CoverPath_Rtk_Dialog: public Ui_CoverPath_Rtk_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_COVERPATH_RTK_DIALOG_H
