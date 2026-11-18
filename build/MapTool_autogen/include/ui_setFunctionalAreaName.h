/********************************************************************************
** Form generated from reading UI file 'setFunctionalAreaName.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETFUNCTIONALAREANAME_H
#define UI_SETFUNCTIONALAREANAME_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SetFunctionalAreaName
{
public:
    QLabel *label;
    QLineEdit *lineEdit;
    QPushButton *pushButton;

    void setupUi(QWidget *SetFunctionalAreaName)
    {
        if (SetFunctionalAreaName->objectName().isEmpty())
            SetFunctionalAreaName->setObjectName("SetFunctionalAreaName");
        SetFunctionalAreaName->resize(400, 300);
        label = new QLabel(SetFunctionalAreaName);
        label->setObjectName("label");
        label->setGeometry(QRect(40, 40, 350, 60));
        lineEdit = new QLineEdit(SetFunctionalAreaName);
        lineEdit->setObjectName("lineEdit");
        lineEdit->setGeometry(QRect(40, 100, 300, 60));
        pushButton = new QPushButton(SetFunctionalAreaName);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(200, 200, 150, 50));

        retranslateUi(SetFunctionalAreaName);

        QMetaObject::connectSlotsByName(SetFunctionalAreaName);
    } // setupUi

    void retranslateUi(QWidget *SetFunctionalAreaName)
    {
        SetFunctionalAreaName->setWindowTitle(QCoreApplication::translate("SetFunctionalAreaName", "Form", nullptr));
        label->setText(QCoreApplication::translate("SetFunctionalAreaName", "\350\256\276\347\275\256\344\270\212\344\270\200\344\270\252\344\277\235\345\255\230\345\212\237\350\203\275\345\214\272\347\232\204\345\220\215\347\247\260\357\274\232", nullptr));
        pushButton->setText(QCoreApplication::translate("SetFunctionalAreaName", "\347\241\256\350\256\244", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SetFunctionalAreaName: public Ui_SetFunctionalAreaName {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETFUNCTIONALAREANAME_H
