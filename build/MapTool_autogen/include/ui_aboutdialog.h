/********************************************************************************
** Form generated from reading UI file 'aboutdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ABOUTDIALOG_H
#define UI_ABOUTDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AboutDialog
{
public:
    QPushButton *pushButton_2;
    QLabel *label_Pic;
    QLabel *label_Text;
    QPushButton *pushButton_3;
    QPushButton *pushButton;

    void setupUi(QWidget *AboutDialog)
    {
        if (AboutDialog->objectName().isEmpty())
            AboutDialog->setObjectName("AboutDialog");
        AboutDialog->resize(550, 350);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/SimpleImage.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        AboutDialog->setWindowIcon(icon);
        pushButton_2 = new QPushButton(AboutDialog);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setGeometry(QRect(360, 110, 150, 50));
        pushButton_2->setAutoFillBackground(false);
        pushButton_2->setStyleSheet(QString::fromUtf8("QPushButton\n"
"{\n"
"    border-image: url(./res/ButtonNormal.png);\n"
"}\n"
"QPushButton:hover\n"
"{\n"
"    border-image: url(./res/ButtonHover.png);\n"
"}\n"
"QPushButton:pressed\n"
"{\n"
"    border-image: url(./res/ButtonPressed.png);\n"
"}\n"
"color:rgb(18, 188, 255);\n"
""));
        pushButton_2->setCheckable(false);
        label_Pic = new QLabel(AboutDialog);
        label_Pic->setObjectName("label_Pic");
        label_Pic->setGeometry(QRect(70, 20, 233, 233));
        label_Text = new QLabel(AboutDialog);
        label_Text->setObjectName("label_Text");
        label_Text->setGeometry(QRect(20, 283, 510, 50));
        pushButton_3 = new QPushButton(AboutDialog);
        pushButton_3->setObjectName("pushButton_3");
        pushButton_3->setEnabled(false);
        pushButton_3->setGeometry(QRect(360, 200, 150, 50));
        pushButton_3->setStyleSheet(QString::fromUtf8("QPushButton\n"
"{\n"
"    border-image: url(./res/ButtonNormal.png);\n"
"}\n"
"QPushButton:hover\n"
"{\n"
"    border-image: url(./res/ButtonHover.png);\n"
"}\n"
"QPushButton:pressed\n"
"{\n"
"    border-image: url(./res/ButtonPressed.png);\n"
"}"));
        pushButton = new QPushButton(AboutDialog);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(360, 20, 150, 50));
        pushButton->setStyleSheet(QString::fromUtf8("QPushButton\n"
"{\n"
"    border-image: url(./res/ButtonNormal.png);\n"
"}\n"
"QPushButton:hover\n"
"{\n"
"    border-image: url(./res/ButtonHover.png);\n"
"}\n"
"QPushButton:pressed\n"
"{\n"
"    border-image: url(./res/ButtonPressed.png);\n"
"}\n"
""));
        pushButton->setAutoDefault(true);

        retranslateUi(AboutDialog);

        QMetaObject::connectSlotsByName(AboutDialog);
    } // setupUi

    void retranslateUi(QWidget *AboutDialog)
    {
        AboutDialog->setWindowTitle(QCoreApplication::translate("AboutDialog", "\345\205\263\344\272\216 - Map Tool", nullptr));
        pushButton_2->setText(QCoreApplication::translate("AboutDialog", "\346\212\200\346\234\257\346\224\257\346\214\201", nullptr));
        label_Pic->setText(QCoreApplication::translate("AboutDialog", "\345\233\276\347\211\207", nullptr));
        label_Text->setText(QCoreApplication::translate("AboutDialog", "Simple Image", nullptr));
        pushButton_3->setText(QCoreApplication::translate("AboutDialog", "\347\211\210\346\235\203\345\217\212\346\235\241\346\254\276", nullptr));
        pushButton->setText(QCoreApplication::translate("AboutDialog", "\346\243\200\346\237\245\346\233\264\346\226\260", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AboutDialog: public Ui_AboutDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTDIALOG_H
