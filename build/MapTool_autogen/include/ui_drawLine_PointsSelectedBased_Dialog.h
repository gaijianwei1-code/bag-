/********************************************************************************
** Form generated from reading UI file 'drawLine_PointsSelectedBased_Dialog.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DRAWLINE_POINTSSELECTEDBASED_DIALOG_H
#define UI_DRAWLINE_POINTSSELECTEDBASED_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>

QT_BEGIN_NAMESPACE

class Ui_DrawLine_PointsSelectedBased_Dialog
{
public:
    QGridLayout *gridLayout_2;
    QGridLayout *gridLayout;
    QSpacerItem *horizontalSpacer;
    QSpinBox *spinBox_extendLength;
    QComboBox *comboBox_offsetDirection;
    QLabel *label_offsetDirection;
    QLabel *label_offsetDistance_val;
    QLabel *label_extendLength_val;
    QLabel *label_offsetDistance;
    QSpacerItem *horizontalSpacer_2;
    QCheckBox *checkBox_curveClosure;
    QSpinBox *spinBox_offsetDistance;
    QLabel *label_lineWidth;
    QLabel *label_extendLength;
    QDialogButtonBox *buttonBox;
    QSpinBox *spinBox_lineWidth;
    QLabel *label_lineWidth_val;

    void setupUi(QDialog *DrawLine_PointsSelectedBased_Dialog)
    {
        if (DrawLine_PointsSelectedBased_Dialog->objectName().isEmpty())
            DrawLine_PointsSelectedBased_Dialog->setObjectName("DrawLine_PointsSelectedBased_Dialog");
        DrawLine_PointsSelectedBased_Dialog->resize(331, 196);
        gridLayout_2 = new QGridLayout(DrawLine_PointsSelectedBased_Dialog);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer, 3, 0, 1, 1);

        spinBox_extendLength = new QSpinBox(DrawLine_PointsSelectedBased_Dialog);
        spinBox_extendLength->setObjectName("spinBox_extendLength");
        spinBox_extendLength->setMaximum(100000);
        spinBox_extendLength->setSingleStep(10);

        gridLayout->addWidget(spinBox_extendLength, 2, 1, 1, 3);

        comboBox_offsetDirection = new QComboBox(DrawLine_PointsSelectedBased_Dialog);
        comboBox_offsetDirection->addItem(QString());
        comboBox_offsetDirection->addItem(QString());
        comboBox_offsetDirection->setObjectName("comboBox_offsetDirection");

        gridLayout->addWidget(comboBox_offsetDirection, 0, 1, 1, 3);

        label_offsetDirection = new QLabel(DrawLine_PointsSelectedBased_Dialog);
        label_offsetDirection->setObjectName("label_offsetDirection");
        QFont font;
        font.setPointSize(11);
        label_offsetDirection->setFont(font);

        gridLayout->addWidget(label_offsetDirection, 0, 0, 1, 1);

        label_offsetDistance_val = new QLabel(DrawLine_PointsSelectedBased_Dialog);
        label_offsetDistance_val->setObjectName("label_offsetDistance_val");

        gridLayout->addWidget(label_offsetDistance_val, 1, 4, 1, 1);

        label_extendLength_val = new QLabel(DrawLine_PointsSelectedBased_Dialog);
        label_extendLength_val->setObjectName("label_extendLength_val");

        gridLayout->addWidget(label_extendLength_val, 2, 4, 1, 1);

        label_offsetDistance = new QLabel(DrawLine_PointsSelectedBased_Dialog);
        label_offsetDistance->setObjectName("label_offsetDistance");
        label_offsetDistance->setFont(font);

        gridLayout->addWidget(label_offsetDistance, 1, 0, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 3, 3, 1, 1);

        checkBox_curveClosure = new QCheckBox(DrawLine_PointsSelectedBased_Dialog);
        checkBox_curveClosure->setObjectName("checkBox_curveClosure");
        checkBox_curveClosure->setFont(font);

        gridLayout->addWidget(checkBox_curveClosure, 3, 1, 1, 2);

        spinBox_offsetDistance = new QSpinBox(DrawLine_PointsSelectedBased_Dialog);
        spinBox_offsetDistance->setObjectName("spinBox_offsetDistance");
        spinBox_offsetDistance->setMaximum(100000);
        spinBox_offsetDistance->setSingleStep(5);

        gridLayout->addWidget(spinBox_offsetDistance, 1, 1, 1, 3);

        label_lineWidth = new QLabel(DrawLine_PointsSelectedBased_Dialog);
        label_lineWidth->setObjectName("label_lineWidth");
        label_lineWidth->setFont(font);

        gridLayout->addWidget(label_lineWidth, 4, 0, 1, 1);

        label_extendLength = new QLabel(DrawLine_PointsSelectedBased_Dialog);
        label_extendLength->setObjectName("label_extendLength");
        label_extendLength->setFont(font);

        gridLayout->addWidget(label_extendLength, 2, 0, 1, 1);

        buttonBox = new QDialogButtonBox(DrawLine_PointsSelectedBased_Dialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 5, 1, 1, 4);

        spinBox_lineWidth = new QSpinBox(DrawLine_PointsSelectedBased_Dialog);
        spinBox_lineWidth->setObjectName("spinBox_lineWidth");
        spinBox_lineWidth->setMaximum(100000);
        spinBox_lineWidth->setSingleStep(5);

        gridLayout->addWidget(spinBox_lineWidth, 4, 1, 1, 3);

        label_lineWidth_val = new QLabel(DrawLine_PointsSelectedBased_Dialog);
        label_lineWidth_val->setObjectName("label_lineWidth_val");

        gridLayout->addWidget(label_lineWidth_val, 4, 4, 1, 1);


        gridLayout_2->addLayout(gridLayout, 0, 0, 1, 1);


        retranslateUi(DrawLine_PointsSelectedBased_Dialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, DrawLine_PointsSelectedBased_Dialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, DrawLine_PointsSelectedBased_Dialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(DrawLine_PointsSelectedBased_Dialog);
    } // setupUi

    void retranslateUi(QDialog *DrawLine_PointsSelectedBased_Dialog)
    {
        DrawLine_PointsSelectedBased_Dialog->setWindowTitle(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "\346\262\277\350\275\250\350\277\271\347\273\230\345\210\266\350\231\232\346\213\237\345\242\231", nullptr));
        spinBox_extendLength->setSuffix(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "(cm)", nullptr));
        comboBox_offsetDirection->setItemText(0, QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "\345\217\263\344\276\247", nullptr));
        comboBox_offsetDirection->setItemText(1, QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "\345\267\246\344\276\247", nullptr));

        label_offsetDirection->setText(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "\345\201\217\347\247\273\346\226\271\345\220\221\357\274\232", nullptr));
        label_offsetDistance_val->setText(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "(0px)", nullptr));
        label_extendLength_val->setText(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "(0px)", nullptr));
        label_offsetDistance->setText(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "\345\201\217\347\247\273\350\267\235\347\246\273\357\274\232", nullptr));
        checkBox_curveClosure->setText(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "\346\233\262\347\272\277\351\227\255\345\220\210", nullptr));
        spinBox_offsetDistance->setSuffix(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "(cm)", nullptr));
        label_lineWidth->setText(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "\350\231\232\346\213\237\345\242\231\345\256\275\345\272\246\357\274\232", nullptr));
        label_extendLength->setText(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "\345\211\215\345\220\216\345\273\266\351\225\277\357\274\232", nullptr));
        spinBox_lineWidth->setSuffix(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "(cm)", nullptr));
        label_lineWidth_val->setText(QCoreApplication::translate("DrawLine_PointsSelectedBased_Dialog", "(0px)", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DrawLine_PointsSelectedBased_Dialog: public Ui_DrawLine_PointsSelectedBased_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DRAWLINE_POINTSSELECTEDBASED_DIALOG_H
