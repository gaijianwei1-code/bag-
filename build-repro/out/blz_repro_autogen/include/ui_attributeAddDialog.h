/********************************************************************************
** Form generated from reading UI file 'attributeAddDialog.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ATTRIBUTEADDDIALOG_H
#define UI_ATTRIBUTEADDDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_AttributeAddDialog
{
public:
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout_8;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_turnAround;
    QLineEdit *lineEdit_turnAround;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_charge;
    QLineEdit *lineEdit_charge;
    QCheckBox *parking_area;
    QCheckBox *ignore_obstacles;
    QCheckBox *curve_paths;
    QGridLayout *gridLayout_2;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_4;
    QVBoxLayout *verticalLayout;
    QRadioButton *road_id_1;
    QRadioButton *road_id_2;
    QRadioButton *road_id_3;
    QRadioButton *road_id_4;
    QRadioButton *road_id_10;
    QRadioButton *road_id_20;
    QRadioButton *road_id_NO;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_5;
    QVBoxLayout *verticalLayout_2;
    QRadioButton *bypass_on;
    QRadioButton *bypass_off;
    QVBoxLayout *verticalLayout_9;
    QLabel *label_road_width;
    QLineEdit *lineEdit_road_width;
    QLabel *label_road_width_cm;
    QSpacerItem *verticalSpacer;
    QVBoxLayout *verticalLayout_3;
    QCheckBox *seismic;
    QCheckBox *trash;
    QCheckBox *back;
    QCheckBox *clean_flag;
    QCheckBox *reverse_traj;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout_6;
    QLabel *label;
    QLineEdit *att_1;
    QLineEdit *att_2;
    QLineEdit *att_3;
    QVBoxLayout *verticalLayout_7;
    QLabel *label_2;
    QLineEdit *val_1;
    QLineEdit *val_2;
    QLineEdit *val_3;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *AttributeAddDialog)
    {
        if (AttributeAddDialog->objectName().isEmpty())
            AttributeAddDialog->setObjectName("AttributeAddDialog");
        AttributeAddDialog->resize(356, 608);
        gridLayout = new QGridLayout(AttributeAddDialog);
        gridLayout->setObjectName("gridLayout");
        verticalLayout_8 = new QVBoxLayout();
        verticalLayout_8->setObjectName("verticalLayout_8");
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label_turnAround = new QLabel(AttributeAddDialog);
        label_turnAround->setObjectName("label_turnAround");
        QFont font;
        font.setPointSize(12);
        label_turnAround->setFont(font);

        horizontalLayout_2->addWidget(label_turnAround);

        lineEdit_turnAround = new QLineEdit(AttributeAddDialog);
        lineEdit_turnAround->setObjectName("lineEdit_turnAround");

        horizontalLayout_2->addWidget(lineEdit_turnAround);


        verticalLayout_8->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_charge = new QLabel(AttributeAddDialog);
        label_charge->setObjectName("label_charge");
        label_charge->setFont(font);

        horizontalLayout_3->addWidget(label_charge);

        lineEdit_charge = new QLineEdit(AttributeAddDialog);
        lineEdit_charge->setObjectName("lineEdit_charge");

        horizontalLayout_3->addWidget(lineEdit_charge);


        verticalLayout_8->addLayout(horizontalLayout_3);

        parking_area = new QCheckBox(AttributeAddDialog);
        parking_area->setObjectName("parking_area");
        parking_area->setFont(font);

        verticalLayout_8->addWidget(parking_area);

        ignore_obstacles = new QCheckBox(AttributeAddDialog);
        ignore_obstacles->setObjectName("ignore_obstacles");
        ignore_obstacles->setFont(font);

        verticalLayout_8->addWidget(ignore_obstacles);

        curve_paths = new QCheckBox(AttributeAddDialog);
        curve_paths->setObjectName("curve_paths");
        curve_paths->setFont(font);

        verticalLayout_8->addWidget(curve_paths);


        gridLayout->addLayout(verticalLayout_8, 1, 1, 1, 1);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setObjectName("gridLayout_2");
        groupBox = new QGroupBox(AttributeAddDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setFont(font);
        verticalLayout_4 = new QVBoxLayout(groupBox);
        verticalLayout_4->setObjectName("verticalLayout_4");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        road_id_1 = new QRadioButton(groupBox);
        road_id_1->setObjectName("road_id_1");
        road_id_1->setChecked(true);

        verticalLayout->addWidget(road_id_1);

        road_id_2 = new QRadioButton(groupBox);
        road_id_2->setObjectName("road_id_2");

        verticalLayout->addWidget(road_id_2);

        road_id_3 = new QRadioButton(groupBox);
        road_id_3->setObjectName("road_id_3");

        verticalLayout->addWidget(road_id_3);

        road_id_4 = new QRadioButton(groupBox);
        road_id_4->setObjectName("road_id_4");

        verticalLayout->addWidget(road_id_4);

        road_id_10 = new QRadioButton(groupBox);
        road_id_10->setObjectName("road_id_10");

        verticalLayout->addWidget(road_id_10);

        road_id_20 = new QRadioButton(groupBox);
        road_id_20->setObjectName("road_id_20");

        verticalLayout->addWidget(road_id_20);

        road_id_NO = new QRadioButton(groupBox);
        road_id_NO->setObjectName("road_id_NO");
        road_id_NO->setChecked(false);

        verticalLayout->addWidget(road_id_NO);


        verticalLayout_4->addLayout(verticalLayout);


        gridLayout_2->addWidget(groupBox, 0, 0, 3, 1);

        groupBox_2 = new QGroupBox(AttributeAddDialog);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setFont(font);
        verticalLayout_5 = new QVBoxLayout(groupBox_2);
        verticalLayout_5->setObjectName("verticalLayout_5");
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName("verticalLayout_2");
        bypass_on = new QRadioButton(groupBox_2);
        bypass_on->setObjectName("bypass_on");
        bypass_on->setChecked(true);

        verticalLayout_2->addWidget(bypass_on);

        bypass_off = new QRadioButton(groupBox_2);
        bypass_off->setObjectName("bypass_off");

        verticalLayout_2->addWidget(bypass_off);


        verticalLayout_5->addLayout(verticalLayout_2);


        gridLayout_2->addWidget(groupBox_2, 0, 1, 1, 1);

        verticalLayout_9 = new QVBoxLayout();
        verticalLayout_9->setObjectName("verticalLayout_9");
        label_road_width = new QLabel(AttributeAddDialog);
        label_road_width->setObjectName("label_road_width");
        label_road_width->setFont(font);

        verticalLayout_9->addWidget(label_road_width);

        lineEdit_road_width = new QLineEdit(AttributeAddDialog);
        lineEdit_road_width->setObjectName("lineEdit_road_width");

        verticalLayout_9->addWidget(lineEdit_road_width);

        label_road_width_cm = new QLabel(AttributeAddDialog);
        label_road_width_cm->setObjectName("label_road_width_cm");

        verticalLayout_9->addWidget(label_road_width_cm);


        gridLayout_2->addLayout(verticalLayout_9, 1, 1, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_2->addItem(verticalSpacer, 2, 1, 1, 1);


        gridLayout->addLayout(gridLayout_2, 0, 0, 1, 2);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName("verticalLayout_3");
        seismic = new QCheckBox(AttributeAddDialog);
        seismic->setObjectName("seismic");
        seismic->setFont(font);

        verticalLayout_3->addWidget(seismic);

        trash = new QCheckBox(AttributeAddDialog);
        trash->setObjectName("trash");
        trash->setFont(font);

        verticalLayout_3->addWidget(trash);

        back = new QCheckBox(AttributeAddDialog);
        back->setObjectName("back");
        back->setFont(font);

        verticalLayout_3->addWidget(back);

        clean_flag = new QCheckBox(AttributeAddDialog);
        clean_flag->setObjectName("clean_flag");
        clean_flag->setFont(font);

        verticalLayout_3->addWidget(clean_flag);

        reverse_traj = new QCheckBox(AttributeAddDialog);
        reverse_traj->setObjectName("reverse_traj");
        reverse_traj->setFont(font);

        verticalLayout_3->addWidget(reverse_traj);


        gridLayout->addLayout(verticalLayout_3, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setObjectName("verticalLayout_6");
        label = new QLabel(AttributeAddDialog);
        label->setObjectName("label");
        QFont font1;
        font1.setPointSize(11);
        label->setFont(font1);

        verticalLayout_6->addWidget(label);

        att_1 = new QLineEdit(AttributeAddDialog);
        att_1->setObjectName("att_1");

        verticalLayout_6->addWidget(att_1);

        att_2 = new QLineEdit(AttributeAddDialog);
        att_2->setObjectName("att_2");

        verticalLayout_6->addWidget(att_2);

        att_3 = new QLineEdit(AttributeAddDialog);
        att_3->setObjectName("att_3");

        verticalLayout_6->addWidget(att_3);


        horizontalLayout->addLayout(verticalLayout_6);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setObjectName("verticalLayout_7");
        label_2 = new QLabel(AttributeAddDialog);
        label_2->setObjectName("label_2");
        label_2->setFont(font1);

        verticalLayout_7->addWidget(label_2);

        val_1 = new QLineEdit(AttributeAddDialog);
        val_1->setObjectName("val_1");

        verticalLayout_7->addWidget(val_1);

        val_2 = new QLineEdit(AttributeAddDialog);
        val_2->setObjectName("val_2");

        verticalLayout_7->addWidget(val_2);

        val_3 = new QLineEdit(AttributeAddDialog);
        val_3->setObjectName("val_3");

        verticalLayout_7->addWidget(val_3);


        horizontalLayout->addLayout(verticalLayout_7);


        gridLayout->addLayout(horizontalLayout, 2, 0, 1, 2);

        buttonBox = new QDialogButtonBox(AttributeAddDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 3, 0, 1, 1);


        retranslateUi(AttributeAddDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, AttributeAddDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, AttributeAddDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(AttributeAddDialog);
    } // setupUi

    void retranslateUi(QDialog *AttributeAddDialog)
    {
        AttributeAddDialog->setWindowTitle(QCoreApplication::translate("AttributeAddDialog", "\345\261\236\346\200\247\350\256\276\345\256\232", nullptr));
        label_turnAround->setText(QCoreApplication::translate("AttributeAddDialog", "\346\216\211\345\244\264\347\202\271", nullptr));
        lineEdit_turnAround->setText(QCoreApplication::translate("AttributeAddDialog", "0", nullptr));
        label_charge->setText(QCoreApplication::translate("AttributeAddDialog", "\345\205\205\347\202\271\347\224\265", nullptr));
        lineEdit_charge->setText(QCoreApplication::translate("AttributeAddDialog", "0", nullptr));
        parking_area->setText(QCoreApplication::translate("AttributeAddDialog", "\345\201\234\350\275\246\345\214\272", nullptr));
        ignore_obstacles->setText(QCoreApplication::translate("AttributeAddDialog", "\345\277\275\347\225\245\351\232\234\347\242\215\347\211\251", nullptr));
        curve_paths->setText(QCoreApplication::translate("AttributeAddDialog", "\345\274\257\351\201\223", nullptr));
        groupBox->setTitle(QCoreApplication::translate("AttributeAddDialog", "road_id\357\274\210\350\264\264\350\276\271\343\200\201\351\207\207\346\240\267\357\274\211", nullptr));
        road_id_1->setText(QCoreApplication::translate("AttributeAddDialog", "\345\217\263\350\264\264\350\276\271\343\200\201\345\220\221\345\267\246\351\207\207\346\240\267(1)", nullptr));
        road_id_2->setText(QCoreApplication::translate("AttributeAddDialog", "\345\267\246\350\264\264\350\276\271\343\200\201\345\220\221\345\217\263\351\207\207\346\240\267(2)", nullptr));
        road_id_3->setText(QCoreApplication::translate("AttributeAddDialog", "(3)", nullptr));
        road_id_4->setText(QCoreApplication::translate("AttributeAddDialog", "(4)", nullptr));
        road_id_10->setText(QCoreApplication::translate("AttributeAddDialog", "\344\270\215\350\264\264\350\276\271\343\200\201\345\220\221\345\267\246\351\207\207\346\240\267(10)", nullptr));
        road_id_20->setText(QCoreApplication::translate("AttributeAddDialog", "\344\270\215\350\264\264\350\276\271\343\200\201\345\220\221\345\217\263\351\207\207\346\240\267(20)", nullptr));
        road_id_NO->setText(QCoreApplication::translate("AttributeAddDialog", "\344\270\215\350\264\264\350\276\271(0)", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("AttributeAddDialog", "\347\273\225\351\232\234", nullptr));
        bypass_on->setText(QCoreApplication::translate("AttributeAddDialog", "\346\211\223\345\274\200\347\273\225\351\232\234", nullptr));
        bypass_off->setText(QCoreApplication::translate("AttributeAddDialog", "\345\205\263\351\227\255\347\273\225\351\232\234", nullptr));
        label_road_width->setText(QCoreApplication::translate("AttributeAddDialog", "\351\207\207\346\240\267\351\201\223\350\267\257\345\256\275\345\272\246", nullptr));
        lineEdit_road_width->setText(QCoreApplication::translate("AttributeAddDialog", "0", nullptr));
        label_road_width_cm->setText(QCoreApplication::translate("AttributeAddDialog", "(cm)", nullptr));
        seismic->setText(QCoreApplication::translate("AttributeAddDialog", "\351\234\207\345\260\230\347\202\271", nullptr));
        trash->setText(QCoreApplication::translate("AttributeAddDialog", "\345\200\222\345\236\203\345\234\276\347\202\271", nullptr));
        back->setText(QCoreApplication::translate("AttributeAddDialog", "\345\200\222\350\275\246\347\202\271", nullptr));
        clean_flag->setText(QCoreApplication::translate("AttributeAddDialog", "\346\270\205\346\211\253", nullptr));
        reverse_traj->setText(QCoreApplication::translate("AttributeAddDialog", "\345\200\222\350\275\246\350\275\250\350\277\271", nullptr));
        label->setText(QCoreApplication::translate("AttributeAddDialog", "\350\207\252\345\256\232\344\271\211\345\261\236\346\200\247\357\274\232", nullptr));
        att_1->setText(QCoreApplication::translate("AttributeAddDialog", "network_id", nullptr));
        label_2->setText(QCoreApplication::translate("AttributeAddDialog", "\345\261\236\346\200\247\345\200\274\357\274\232", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AttributeAddDialog: public Ui_AttributeAddDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ATTRIBUTEADDDIALOG_H
