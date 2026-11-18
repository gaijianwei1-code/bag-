/********************************************************************************
** Form generated from reading UI file 'copypointsdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_COPYPOINTSDIALOG_H
#define UI_COPYPOINTSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_copyPointsDialog
{
public:
    QGridLayout *gridLayout_3;
    QGridLayout *gridLayout_2;
    QPushButton *pushButton_first;
    QPushButton *pushButton_second;
    QPushButton *pushButton_apply_copy;
    QGridLayout *gridLayout;
    QLabel *label_direction;
    QLineEdit *lineEdit_val_direction;
    QLabel *label_angle;
    QLabel *label_distance;
    QLineEdit *lineEdit_val_distance;
    QLabel *label_cm;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QDialog *copyPointsDialog)
    {
        if (copyPointsDialog->objectName().isEmpty())
            copyPointsDialog->setObjectName("copyPointsDialog");
        copyPointsDialog->resize(229, 134);
        gridLayout_3 = new QGridLayout(copyPointsDialog);
        gridLayout_3->setObjectName("gridLayout_3");
        gridLayout_2 = new QGridLayout();
        gridLayout_2->setObjectName("gridLayout_2");
        pushButton_first = new QPushButton(copyPointsDialog);
        pushButton_first->setObjectName("pushButton_first");

        gridLayout_2->addWidget(pushButton_first, 0, 0, 1, 1);

        pushButton_second = new QPushButton(copyPointsDialog);
        pushButton_second->setObjectName("pushButton_second");

        gridLayout_2->addWidget(pushButton_second, 0, 1, 1, 1);

        pushButton_apply_copy = new QPushButton(copyPointsDialog);
        pushButton_apply_copy->setObjectName("pushButton_apply_copy");

        gridLayout_2->addWidget(pushButton_apply_copy, 1, 0, 1, 1);


        gridLayout_3->addLayout(gridLayout_2, 1, 1, 1, 1);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        label_direction = new QLabel(copyPointsDialog);
        label_direction->setObjectName("label_direction");

        gridLayout->addWidget(label_direction, 0, 0, 1, 1);

        lineEdit_val_direction = new QLineEdit(copyPointsDialog);
        lineEdit_val_direction->setObjectName("lineEdit_val_direction");
        lineEdit_val_direction->setMinimumSize(QSize(100, 0));

        gridLayout->addWidget(lineEdit_val_direction, 0, 1, 1, 1);

        label_angle = new QLabel(copyPointsDialog);
        label_angle->setObjectName("label_angle");

        gridLayout->addWidget(label_angle, 0, 2, 1, 1);

        label_distance = new QLabel(copyPointsDialog);
        label_distance->setObjectName("label_distance");

        gridLayout->addWidget(label_distance, 1, 0, 1, 1);

        lineEdit_val_distance = new QLineEdit(copyPointsDialog);
        lineEdit_val_distance->setObjectName("lineEdit_val_distance");
        lineEdit_val_distance->setMinimumSize(QSize(100, 0));

        gridLayout->addWidget(lineEdit_val_distance, 1, 1, 1, 1);

        label_cm = new QLabel(copyPointsDialog);
        label_cm->setObjectName("label_cm");

        gridLayout->addWidget(label_cm, 1, 2, 1, 1);


        gridLayout_3->addLayout(gridLayout, 0, 0, 1, 3);

        horizontalSpacer = new QSpacerItem(164, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_3->addItem(horizontalSpacer, 1, 0, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(163, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_2, 1, 2, 1, 1);


        retranslateUi(copyPointsDialog);

        QMetaObject::connectSlotsByName(copyPointsDialog);
    } // setupUi

    void retranslateUi(QDialog *copyPointsDialog)
    {
        copyPointsDialog->setWindowTitle(QCoreApplication::translate("copyPointsDialog", "\345\244\215\345\210\266\347\274\226\350\276\221", nullptr));
        pushButton_first->setText(QCoreApplication::translate("copyPointsDialog", "\350\256\276\345\256\232\345\237\272\345\207\206\347\202\271", nullptr));
        pushButton_second->setText(QCoreApplication::translate("copyPointsDialog", "\350\256\276\345\256\232\345\201\217\347\247\273\347\202\271", nullptr));
        pushButton_apply_copy->setText(QCoreApplication::translate("copyPointsDialog", "\345\244\215\345\210\266\347\202\271\351\200\211\345\214\272", nullptr));
        label_direction->setText(QCoreApplication::translate("copyPointsDialog", "\345\201\217\347\247\273\346\226\271\345\220\221\357\274\232", nullptr));
        label_angle->setText(QCoreApplication::translate("copyPointsDialog", "\350\247\222\345\272\246(\302\260)", nullptr));
        label_distance->setText(QCoreApplication::translate("copyPointsDialog", "\345\201\217\347\247\273\350\267\235\347\246\273\357\274\232", nullptr));
        label_cm->setText(QCoreApplication::translate("copyPointsDialog", "(cm)", nullptr));
    } // retranslateUi

};

namespace Ui {
    class copyPointsDialog: public Ui_copyPointsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_COPYPOINTSDIALOG_H
