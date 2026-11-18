/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>
#include <graphicsview.h>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *doOpen;
    QAction *doClose;
    QAction *doSave;
    QAction *doSave_As;
    QAction *doExit;
    QAction *doAbout;
    QAction *doRestore;
    QAction *doZoom;
    QAction *doGrayscale;
    QAction *doWarm;
    QAction *doCool;
    QAction *doMovie_frame;
    QAction *doClassic_frame;
    QAction *doFlower_frame;
    QAction *doBrightness;
    QAction *doHorizontal;
    QAction *doVertical;
    QAction *doSimple;
    QAction *doGauss;
    QAction *doMeida_Filter;
    QAction *doLaplace;
    QAction *doBinaryzation;
    QAction *doContour_extraction;
    QAction *doDilate;
    QAction *doExpansion;
    QAction *doOpening;
    QAction *doClosing;
    QAction *doRotate;
    QAction *do_adjust;
    QAction *do_adjust_left;
    QAction *do_adjust_right;
    QAction *doRotate_left;
    QAction *doRotate_right;
    QAction *do_AutoPS_wec;
    QAction *doOpenJson;
    QAction *doSaveJson;
    QAction *doCleanJson;
    QAction *actiontest;
    QAction *do_draw_path;
    QAction *do_fill_path;
    QAction *doOpenYaml;
    QAction *doDrawLine;
    QAction *doDrawTraj_png;
    QAction *do_draw_polygons_area;
    QAction *do_draw_ploygons;
    QAction *do_adjust_polygons_area;
    QAction *do_draw_full_covered_path;
    QAction *do_save_polygons_area;
    QAction *do_load_polygons_area;
    QAction *do_clean_polygons_area;
    QAction *doDrawWhitePolygon;
    QAction *doDrawPathContinue;
    QAction *do_rtk_full_cover_path;
    QAction *doOpenJsonExclusively;
    QAction *do_selectMultiPoint_polygen;
    QAction *do_addAttribute;
    QAction *do_clean_pointSelect_area;
    QAction *doCleanAllMPath;
    QAction *do_draw_next_mpath;
    QAction *doOpenJson_multiMPath;
    QAction *do_draw_functional_area;
    QAction *do_clean_current_functional_area;
    QAction *do_clean_all_functional_area;
    QAction *do_draw_next_functional_area;
    QAction *do_save_functional_area;
    QAction *do_load_functional_area;
    QAction *doSaveJson_unfilled;
    QAction *do_selectMultiPoint_pointSelect;
    QAction *do_clean_pointSelect_pointSelect;
    QAction *do_addAttribute_pointSelect;
    QAction *do_copyPoints_pointSelect;
    QAction *do_deletePoints_pointSelect;
    QAction *do_movePoints_pointSelect;
    QAction *do_drawLine_pointsSelectedBased;
    QAction *do_modifyPGMSize;
    QAction *do_draw_parkingLot_area;
    QAction *do_save_parkingLot_area;
    QAction *do_saveYaml_forRTK;
    QAction *do_load_parkingLot_area;
    QAction *do_roslaunch_lslidar_mapping;
    QAction *do_rosbag_play;
    QAction *do_save_mapping_file;
    QAction *do_kill_mapping_and_rosbag_Node;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    GraphicsView *pgmGraphicsView;
    QMenuBar *menubar;
    QMenu *menu;
    QMenu *menu_2;
    QMenu *menu_3;
    QMenu *menu_4;
    QMenu *menu_5;
    QMenu *menurtk;
    QMenu *menu_6;
    QMenu *menu_7;
    QMenu *menu_8;
    QStatusBar *statusbar;
    QToolBar *toolBar_File;
    QToolBar *toolBar_Edit;
    QToolBar *toolBar_PathEdit;
    QToolBar *toolBar_attrEdit;
    QToolBar *toolBar_fullCover;
    QToolBar *toolBar_About;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1200, 900);
        doOpen = new QAction(MainWindow);
        doOpen->setObjectName("doOpen");
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/doOpen.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doOpen->setIcon(icon);
        doClose = new QAction(MainWindow);
        doClose->setObjectName("doClose");
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/doClose.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doClose->setIcon(icon1);
        doSave = new QAction(MainWindow);
        doSave->setObjectName("doSave");
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/doSave.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doSave->setIcon(icon2);
        doSave_As = new QAction(MainWindow);
        doSave_As->setObjectName("doSave_As");
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/doSave_As.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doSave_As->setIcon(icon3);
        doExit = new QAction(MainWindow);
        doExit->setObjectName("doExit");
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/doExit.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doExit->setIcon(icon4);
        doAbout = new QAction(MainWindow);
        doAbout->setObjectName("doAbout");
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/doAbout.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doAbout->setIcon(icon5);
        doRestore = new QAction(MainWindow);
        doRestore->setObjectName("doRestore");
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/doRestore.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doRestore->setIcon(icon6);
        doZoom = new QAction(MainWindow);
        doZoom->setObjectName("doZoom");
        doGrayscale = new QAction(MainWindow);
        doGrayscale->setObjectName("doGrayscale");
        doWarm = new QAction(MainWindow);
        doWarm->setObjectName("doWarm");
        doCool = new QAction(MainWindow);
        doCool->setObjectName("doCool");
        doMovie_frame = new QAction(MainWindow);
        doMovie_frame->setObjectName("doMovie_frame");
        doClassic_frame = new QAction(MainWindow);
        doClassic_frame->setObjectName("doClassic_frame");
        doFlower_frame = new QAction(MainWindow);
        doFlower_frame->setObjectName("doFlower_frame");
        doBrightness = new QAction(MainWindow);
        doBrightness->setObjectName("doBrightness");
        doHorizontal = new QAction(MainWindow);
        doHorizontal->setObjectName("doHorizontal");
        doVertical = new QAction(MainWindow);
        doVertical->setObjectName("doVertical");
        doSimple = new QAction(MainWindow);
        doSimple->setObjectName("doSimple");
        doGauss = new QAction(MainWindow);
        doGauss->setObjectName("doGauss");
        doMeida_Filter = new QAction(MainWindow);
        doMeida_Filter->setObjectName("doMeida_Filter");
        doLaplace = new QAction(MainWindow);
        doLaplace->setObjectName("doLaplace");
        doBinaryzation = new QAction(MainWindow);
        doBinaryzation->setObjectName("doBinaryzation");
        doContour_extraction = new QAction(MainWindow);
        doContour_extraction->setObjectName("doContour_extraction");
        doDilate = new QAction(MainWindow);
        doDilate->setObjectName("doDilate");
        doExpansion = new QAction(MainWindow);
        doExpansion->setObjectName("doExpansion");
        doOpening = new QAction(MainWindow);
        doOpening->setObjectName("doOpening");
        doClosing = new QAction(MainWindow);
        doClosing->setObjectName("doClosing");
        doRotate = new QAction(MainWindow);
        doRotate->setObjectName("doRotate");
        do_adjust = new QAction(MainWindow);
        do_adjust->setObjectName("do_adjust");
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/do_adjust.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        do_adjust->setIcon(icon7);
        do_adjust_left = new QAction(MainWindow);
        do_adjust_left->setObjectName("do_adjust_left");
        do_adjust_right = new QAction(MainWindow);
        do_adjust_right->setObjectName("do_adjust_right");
        doRotate_left = new QAction(MainWindow);
        doRotate_left->setObjectName("doRotate_left");
        doRotate_right = new QAction(MainWindow);
        doRotate_right->setObjectName("doRotate_right");
        do_AutoPS_wec = new QAction(MainWindow);
        do_AutoPS_wec->setObjectName("do_AutoPS_wec");
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/do_AutoPS_wec.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        do_AutoPS_wec->setIcon(icon8);
        doOpenJson = new QAction(MainWindow);
        doOpenJson->setObjectName("doOpenJson");
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/doOpenJson.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doOpenJson->setIcon(icon9);
        doSaveJson = new QAction(MainWindow);
        doSaveJson->setObjectName("doSaveJson");
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/doSaveJson.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doSaveJson->setIcon(icon10);
        doCleanJson = new QAction(MainWindow);
        doCleanJson->setObjectName("doCleanJson");
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/doCleanJson.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doCleanJson->setIcon(icon11);
        actiontest = new QAction(MainWindow);
        actiontest->setObjectName("actiontest");
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/actiontest.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        actiontest->setIcon(icon12);
        do_draw_path = new QAction(MainWindow);
        do_draw_path->setObjectName("do_draw_path");
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/do_draw_path.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        do_draw_path->setIcon(icon13);
        do_fill_path = new QAction(MainWindow);
        do_fill_path->setObjectName("do_fill_path");
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/do_fill_path.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        do_fill_path->setIcon(icon14);
        doOpenYaml = new QAction(MainWindow);
        doOpenYaml->setObjectName("doOpenYaml");
        QIcon icon15;
        icon15.addFile(QString::fromUtf8(":/doOpenYaml.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doOpenYaml->setIcon(icon15);
        doDrawLine = new QAction(MainWindow);
        doDrawLine->setObjectName("doDrawLine");
        doDrawTraj_png = new QAction(MainWindow);
        doDrawTraj_png->setObjectName("doDrawTraj_png");
        QIcon icon16;
        icon16.addFile(QString::fromUtf8(":/doDrawTraj_png.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doDrawTraj_png->setIcon(icon16);
        do_draw_polygons_area = new QAction(MainWindow);
        do_draw_polygons_area->setObjectName("do_draw_polygons_area");
        do_draw_ploygons = new QAction(MainWindow);
        do_draw_ploygons->setObjectName("do_draw_ploygons");
        do_adjust_polygons_area = new QAction(MainWindow);
        do_adjust_polygons_area->setObjectName("do_adjust_polygons_area");
        do_draw_full_covered_path = new QAction(MainWindow);
        do_draw_full_covered_path->setObjectName("do_draw_full_covered_path");
        do_save_polygons_area = new QAction(MainWindow);
        do_save_polygons_area->setObjectName("do_save_polygons_area");
        do_load_polygons_area = new QAction(MainWindow);
        do_load_polygons_area->setObjectName("do_load_polygons_area");
        do_clean_polygons_area = new QAction(MainWindow);
        do_clean_polygons_area->setObjectName("do_clean_polygons_area");
        doDrawWhitePolygon = new QAction(MainWindow);
        doDrawWhitePolygon->setObjectName("doDrawWhitePolygon");
        doDrawPathContinue = new QAction(MainWindow);
        doDrawPathContinue->setObjectName("doDrawPathContinue");
        QIcon icon17;
        icon17.addFile(QString::fromUtf8(":/doDrawPathContinue.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        doDrawPathContinue->setIcon(icon17);
        do_rtk_full_cover_path = new QAction(MainWindow);
        do_rtk_full_cover_path->setObjectName("do_rtk_full_cover_path");
        doOpenJsonExclusively = new QAction(MainWindow);
        doOpenJsonExclusively->setObjectName("doOpenJsonExclusively");
        do_selectMultiPoint_polygen = new QAction(MainWindow);
        do_selectMultiPoint_polygen->setObjectName("do_selectMultiPoint_polygen");
        do_addAttribute = new QAction(MainWindow);
        do_addAttribute->setObjectName("do_addAttribute");
        do_clean_pointSelect_area = new QAction(MainWindow);
        do_clean_pointSelect_area->setObjectName("do_clean_pointSelect_area");
        doCleanAllMPath = new QAction(MainWindow);
        doCleanAllMPath->setObjectName("doCleanAllMPath");
        do_draw_next_mpath = new QAction(MainWindow);
        do_draw_next_mpath->setObjectName("do_draw_next_mpath");
        doOpenJson_multiMPath = new QAction(MainWindow);
        doOpenJson_multiMPath->setObjectName("doOpenJson_multiMPath");
        do_draw_functional_area = new QAction(MainWindow);
        do_draw_functional_area->setObjectName("do_draw_functional_area");
        do_clean_current_functional_area = new QAction(MainWindow);
        do_clean_current_functional_area->setObjectName("do_clean_current_functional_area");
        do_clean_all_functional_area = new QAction(MainWindow);
        do_clean_all_functional_area->setObjectName("do_clean_all_functional_area");
        do_draw_next_functional_area = new QAction(MainWindow);
        do_draw_next_functional_area->setObjectName("do_draw_next_functional_area");
        do_save_functional_area = new QAction(MainWindow);
        do_save_functional_area->setObjectName("do_save_functional_area");
        do_load_functional_area = new QAction(MainWindow);
        do_load_functional_area->setObjectName("do_load_functional_area");
        do_load_functional_area->setEnabled(false);
        doSaveJson_unfilled = new QAction(MainWindow);
        doSaveJson_unfilled->setObjectName("doSaveJson_unfilled");
        do_selectMultiPoint_pointSelect = new QAction(MainWindow);
        do_selectMultiPoint_pointSelect->setObjectName("do_selectMultiPoint_pointSelect");
        do_clean_pointSelect_pointSelect = new QAction(MainWindow);
        do_clean_pointSelect_pointSelect->setObjectName("do_clean_pointSelect_pointSelect");
        do_addAttribute_pointSelect = new QAction(MainWindow);
        do_addAttribute_pointSelect->setObjectName("do_addAttribute_pointSelect");
        do_copyPoints_pointSelect = new QAction(MainWindow);
        do_copyPoints_pointSelect->setObjectName("do_copyPoints_pointSelect");
        do_deletePoints_pointSelect = new QAction(MainWindow);
        do_deletePoints_pointSelect->setObjectName("do_deletePoints_pointSelect");
        do_movePoints_pointSelect = new QAction(MainWindow);
        do_movePoints_pointSelect->setObjectName("do_movePoints_pointSelect");
        do_drawLine_pointsSelectedBased = new QAction(MainWindow);
        do_drawLine_pointsSelectedBased->setObjectName("do_drawLine_pointsSelectedBased");
        do_modifyPGMSize = new QAction(MainWindow);
        do_modifyPGMSize->setObjectName("do_modifyPGMSize");
        do_draw_parkingLot_area = new QAction(MainWindow);
        do_draw_parkingLot_area->setObjectName("do_draw_parkingLot_area");
        do_save_parkingLot_area = new QAction(MainWindow);
        do_save_parkingLot_area->setObjectName("do_save_parkingLot_area");
        do_saveYaml_forRTK = new QAction(MainWindow);
        do_saveYaml_forRTK->setObjectName("do_saveYaml_forRTK");
        do_load_parkingLot_area = new QAction(MainWindow);
        do_load_parkingLot_area->setObjectName("do_load_parkingLot_area");
        do_roslaunch_lslidar_mapping = new QAction(MainWindow);
        do_roslaunch_lslidar_mapping->setObjectName("do_roslaunch_lslidar_mapping");
        do_rosbag_play = new QAction(MainWindow);
        do_rosbag_play->setObjectName("do_rosbag_play");
        do_save_mapping_file = new QAction(MainWindow);
        do_save_mapping_file->setObjectName("do_save_mapping_file");
        do_kill_mapping_and_rosbag_Node = new QAction(MainWindow);
        do_kill_mapping_and_rosbag_Node->setObjectName("do_kill_mapping_and_rosbag_Node");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName("gridLayout");
        pgmGraphicsView = new GraphicsView(centralwidget);
        pgmGraphicsView->setObjectName("pgmGraphicsView");
        pgmGraphicsView->setAutoFillBackground(true);

        gridLayout->addWidget(pgmGraphicsView, 0, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1200, 17));
        menu = new QMenu(menubar);
        menu->setObjectName("menu");
        menu_2 = new QMenu(menubar);
        menu_2->setObjectName("menu_2");
        menu_3 = new QMenu(menubar);
        menu_3->setObjectName("menu_3");
        menu_4 = new QMenu(menubar);
        menu_4->setObjectName("menu_4");
        menu_5 = new QMenu(menubar);
        menu_5->setObjectName("menu_5");
        menurtk = new QMenu(menubar);
        menurtk->setObjectName("menurtk");
        menu_6 = new QMenu(menubar);
        menu_6->setObjectName("menu_6");
        menu_7 = new QMenu(menubar);
        menu_7->setObjectName("menu_7");
        menu_8 = new QMenu(menubar);
        menu_8->setObjectName("menu_8");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);
        toolBar_File = new QToolBar(MainWindow);
        toolBar_File->setObjectName("toolBar_File");
        MainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar_File);
        toolBar_Edit = new QToolBar(MainWindow);
        toolBar_Edit->setObjectName("toolBar_Edit");
        MainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar_Edit);
        toolBar_PathEdit = new QToolBar(MainWindow);
        toolBar_PathEdit->setObjectName("toolBar_PathEdit");
        MainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar_PathEdit);
        toolBar_attrEdit = new QToolBar(MainWindow);
        toolBar_attrEdit->setObjectName("toolBar_attrEdit");
        MainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar_attrEdit);
        toolBar_fullCover = new QToolBar(MainWindow);
        toolBar_fullCover->setObjectName("toolBar_fullCover");
        MainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar_fullCover);
        toolBar_About = new QToolBar(MainWindow);
        toolBar_About->setObjectName("toolBar_About");
        MainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar_About);

        menubar->addAction(menu->menuAction());
        menubar->addAction(menu_2->menuAction());
        menubar->addAction(menu_4->menuAction());
        menubar->addAction(menu_5->menuAction());
        menubar->addAction(menurtk->menuAction());
        menubar->addAction(menu_6->menuAction());
        menubar->addAction(menu_7->menuAction());
        menubar->addAction(menu_8->menuAction());
        menubar->addAction(menu_3->menuAction());
        menu->addAction(doOpen);
        menu->addAction(doSave);
        menu->addAction(doSave_As);
        menu->addAction(doClose);
        menu->addSeparator();
        menu->addAction(doExit);
        menu_2->addAction(doRestore);
        menu_2->addAction(do_adjust);
        menu_2->addSeparator();
        menu_2->addAction(do_AutoPS_wec);
        menu_2->addAction(doDrawLine);
        menu_2->addAction(doDrawWhitePolygon);
        menu_3->addAction(doAbout);
        menu_3->addAction(actiontest);
        menu_4->addAction(doOpenYaml);
        menu_4->addSeparator();
        menu_4->addAction(doOpenJson);
        menu_4->addAction(doOpenJson_multiMPath);
        menu_4->addAction(do_draw_path);
        menu_4->addAction(doDrawPathContinue);
        menu_4->addAction(do_fill_path);
        menu_4->addAction(do_draw_next_mpath);
        menu_4->addAction(doCleanJson);
        menu_4->addAction(doCleanAllMPath);
        menu_4->addSeparator();
        menu_4->addAction(doSaveJson);
        menu_4->addAction(doSaveJson_unfilled);
        menu_4->addAction(doDrawTraj_png);
        menu_4->addSeparator();
        menu_4->addAction(do_selectMultiPoint_pointSelect);
        menu_4->addAction(do_clean_pointSelect_pointSelect);
        menu_4->addAction(do_copyPoints_pointSelect);
        menu_4->addAction(do_deletePoints_pointSelect);
        menu_4->addAction(do_movePoints_pointSelect);
        menu_5->addAction(do_draw_polygons_area);
        menu_5->addAction(do_draw_ploygons);
        menu_5->addAction(do_adjust_polygons_area);
        menu_5->addSeparator();
        menu_5->addAction(do_save_polygons_area);
        menu_5->addAction(do_clean_polygons_area);
        menu_5->addAction(do_load_polygons_area);
        menu_5->addSeparator();
        menu_5->addAction(do_draw_full_covered_path);
        menurtk->addAction(doOpenJsonExclusively);
        menurtk->addAction(do_modifyPGMSize);
        menurtk->addSeparator();
        menurtk->addAction(do_selectMultiPoint_pointSelect);
        menurtk->addAction(do_clean_pointSelect_pointSelect);
        menurtk->addAction(do_drawLine_pointsSelectedBased);
        menurtk->addSeparator();
        menurtk->addAction(do_rtk_full_cover_path);
        menurtk->addSeparator();
        menurtk->addAction(doSave);
        menurtk->addAction(do_saveYaml_forRTK);
        menurtk->addAction(doSaveJson);
        menu_6->addAction(do_selectMultiPoint_polygen);
        menu_6->addAction(do_clean_pointSelect_area);
        menu_6->addAction(do_addAttribute);
        menu_6->addSeparator();
        menu_6->addAction(do_selectMultiPoint_pointSelect);
        menu_6->addAction(do_clean_pointSelect_pointSelect);
        menu_6->addAction(do_addAttribute_pointSelect);
        menu_7->addAction(do_draw_functional_area);
        menu_7->addAction(do_clean_current_functional_area);
        menu_7->addAction(do_clean_all_functional_area);
        menu_7->addAction(do_draw_next_functional_area);
        menu_7->addSeparator();
        menu_7->addAction(do_draw_parkingLot_area);
        menu_7->addAction(do_save_parkingLot_area);
        menu_7->addAction(do_load_parkingLot_area);
        menu_7->addSeparator();
        menu_7->addAction(do_save_functional_area);
        menu_7->addAction(do_load_functional_area);
        menu_8->addAction(do_roslaunch_lslidar_mapping);
        menu_8->addAction(do_rosbag_play);
        menu_8->addAction(do_save_mapping_file);
        menu_8->addAction(do_kill_mapping_and_rosbag_Node);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        doOpen->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200(&O)", nullptr));
#if QT_CONFIG(tooltip)
        doOpen->setToolTip(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200\344\270\200\345\274\240\345\233\276\347\211\207", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        doOpen->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+O", nullptr));
#endif // QT_CONFIG(shortcut)
        doClose->setText(QCoreApplication::translate("MainWindow", "\345\205\263\351\227\255\345\233\276\347\211\207(&W)", nullptr));
#if QT_CONFIG(shortcut)
        doClose->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+W", nullptr));
#endif // QT_CONFIG(shortcut)
        doSave->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\345\233\276\347\211\207(&S)", nullptr));
#if QT_CONFIG(tooltip)
        doSave->setToolTip(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\345\233\276\347\211\207(S)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        doSave->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        doSave_As->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\207\272\345\233\276\347\211\207(F12)", nullptr));
#if QT_CONFIG(tooltip)
        doSave_As->setToolTip(QCoreApplication::translate("MainWindow", "\345\257\274\345\207\272\345\233\276\347\211\207(F12)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        doSave_As->setShortcut(QCoreApplication::translate("MainWindow", "F12", nullptr));
#endif // QT_CONFIG(shortcut)
        doExit->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272(&Q)", nullptr));
#if QT_CONFIG(shortcut)
        doExit->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Q", nullptr));
#endif // QT_CONFIG(shortcut)
        doAbout->setText(QCoreApplication::translate("MainWindow", "\345\205\263\344\272\216(F1)", nullptr));
#if QT_CONFIG(shortcut)
        doAbout->setShortcut(QCoreApplication::translate("MainWindow", "F1", nullptr));
#endif // QT_CONFIG(shortcut)
        doRestore->setText(QCoreApplication::translate("MainWindow", "\346\201\242\345\244\215(&Z)", nullptr));
#if QT_CONFIG(shortcut)
        doRestore->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Z", nullptr));
#endif // QT_CONFIG(shortcut)
        doZoom->setText(QCoreApplication::translate("MainWindow", "\347\274\251\346\224\276(&+)", nullptr));
#if QT_CONFIG(shortcut)
        doZoom->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+-", nullptr));
#endif // QT_CONFIG(shortcut)
        doGrayscale->setText(QCoreApplication::translate("MainWindow", "\347\201\260\345\272\246", nullptr));
        doWarm->setText(QCoreApplication::translate("MainWindow", "Warm", nullptr));
        doCool->setText(QCoreApplication::translate("MainWindow", "Cool", nullptr));
        doMovie_frame->setText(QCoreApplication::translate("MainWindow", "\347\224\265\345\275\261\347\233\270\346\241\206", nullptr));
        doClassic_frame->setText(QCoreApplication::translate("MainWindow", "\347\273\217\345\205\270\347\233\270\346\241\206", nullptr));
        doFlower_frame->setText(QCoreApplication::translate("MainWindow", "\345\260\217\350\212\261\347\233\270\346\241\206", nullptr));
        doBrightness->setText(QCoreApplication::translate("MainWindow", "\344\272\256\345\272\246", nullptr));
        doHorizontal->setText(QCoreApplication::translate("MainWindow", "\346\260\264\345\271\263\347\277\273\350\275\254", nullptr));
        doVertical->setText(QCoreApplication::translate("MainWindow", "\345\236\202\347\233\264\347\277\273\350\275\254", nullptr));
        doSimple->setText(QCoreApplication::translate("MainWindow", "\347\256\200\345\215\225\346\250\241\347\263\212", nullptr));
        doGauss->setText(QCoreApplication::translate("MainWindow", "\351\253\230\346\226\257\346\250\241\347\263\212", nullptr));
        doMeida_Filter->setText(QCoreApplication::translate("MainWindow", "\344\270\255\345\200\274\346\273\244\346\263\242", nullptr));
        doLaplace->setText(QCoreApplication::translate("MainWindow", "\346\213\211\346\231\256\346\213\211\346\226\257\351\224\220\345\214\226", nullptr));
        doBinaryzation->setText(QCoreApplication::translate("MainWindow", "\344\272\214\345\200\274\345\214\226", nullptr));
        doContour_extraction->setText(QCoreApplication::translate("MainWindow", "\350\275\256\345\273\223\346\217\220\345\217\226", nullptr));
        doDilate->setText(QCoreApplication::translate("MainWindow", "\350\205\220\350\232\200", nullptr));
        doExpansion->setText(QCoreApplication::translate("MainWindow", "\350\206\250\350\203\200", nullptr));
        doOpening->setText(QCoreApplication::translate("MainWindow", "\345\274\200\350\277\220\347\256\227", nullptr));
        doClosing->setText(QCoreApplication::translate("MainWindow", "\351\227\255\350\277\220\347\256\227", nullptr));
        doRotate->setText(QCoreApplication::translate("MainWindow", "\346\227\213\350\275\254", nullptr));
        do_adjust->setText(QCoreApplication::translate("MainWindow", "\351\200\202\345\272\224\345\261\217\345\271\225(&D)", nullptr));
#if QT_CONFIG(shortcut)
        do_adjust->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+D", nullptr));
#endif // QT_CONFIG(shortcut)
        do_adjust_left->setText(QCoreApplication::translate("MainWindow", "\345\267\246\345\233\276\351\200\202\345\272\224(&L)", nullptr));
#if QT_CONFIG(shortcut)
        do_adjust_left->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Left", nullptr));
#endif // QT_CONFIG(shortcut)
        do_adjust_right->setText(QCoreApplication::translate("MainWindow", "\345\217\263\345\233\276\351\200\202\345\272\224(&R)", nullptr));
#if QT_CONFIG(shortcut)
        do_adjust_right->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Right", nullptr));
#endif // QT_CONFIG(shortcut)
        doRotate_left->setText(QCoreApplication::translate("MainWindow", "\351\200\206\346\227\266\351\222\210\346\227\213\350\275\254(&L)", nullptr));
#if QT_CONFIG(tooltip)
        doRotate_left->setToolTip(QCoreApplication::translate("MainWindow", "\351\200\206\346\227\266\351\222\210\346\227\213\350\275\254", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        doRotate_left->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+L", nullptr));
#endif // QT_CONFIG(shortcut)
        doRotate_right->setText(QCoreApplication::translate("MainWindow", "\351\241\272\346\227\266\351\222\210\346\227\213\350\275\254(&R)", nullptr));
#if QT_CONFIG(tooltip)
        doRotate_right->setToolTip(QCoreApplication::translate("MainWindow", "\351\241\272\346\227\266\351\222\210\346\227\213\350\275\254", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        doRotate_right->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+R", nullptr));
#endif // QT_CONFIG(shortcut)
        do_AutoPS_wec->setText(QCoreApplication::translate("MainWindow", "\350\207\252\345\212\250\344\277\256\345\233\276", nullptr));
        doOpenJson->setText(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275Json\346\226\207\344\273\266(\345\215\225\346\235\241\350\275\250\350\277\271)", nullptr));
#if QT_CONFIG(tooltip)
        doOpenJson->setToolTip(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275Json\346\226\207\344\273\266(\345\215\225\346\235\241\350\275\250\350\277\271)", nullptr));
#endif // QT_CONFIG(tooltip)
        doSaveJson->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230Json\346\226\207\344\273\266(\345\241\253\345\205\205\350\275\250\350\277\271)", nullptr));
#if QT_CONFIG(tooltip)
        doSaveJson->setToolTip(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230Json\346\226\207\344\273\266(\345\241\253\345\205\205\350\275\250\350\277\271)", nullptr));
#endif // QT_CONFIG(tooltip)
        doCleanJson->setText(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\345\275\223\345\211\215\350\275\250\350\277\271", nullptr));
#if QT_CONFIG(tooltip)
        doCleanJson->setToolTip(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\345\275\223\345\211\215\350\275\250\350\277\271", nullptr));
#endif // QT_CONFIG(tooltip)
        actiontest->setText(QCoreApplication::translate("MainWindow", "test(\345\274\200\345\217\221\344\275\277\347\224\250)", nullptr));
#if QT_CONFIG(tooltip)
        actiontest->setToolTip(QCoreApplication::translate("MainWindow", "test", nullptr));
#endif // QT_CONFIG(tooltip)
        do_draw_path->setText(QCoreApplication::translate("MainWindow", "\350\207\252\347\224\261\347\273\230\345\210\266\350\275\250\350\277\271", nullptr));
#if QT_CONFIG(tooltip)
        do_draw_path->setToolTip(QCoreApplication::translate("MainWindow", "\350\207\252\347\224\261\347\273\230\345\210\266\350\275\250\350\277\271", nullptr));
#endif // QT_CONFIG(tooltip)
        do_fill_path->setText(QCoreApplication::translate("MainWindow", "\345\241\253\345\205\205\345\205\250\351\203\250\350\275\250\350\277\271", nullptr));
#if QT_CONFIG(tooltip)
        do_fill_path->setToolTip(QCoreApplication::translate("MainWindow", "\345\241\253\345\205\205\345\205\250\351\203\250\350\275\250\350\277\271", nullptr));
#endif // QT_CONFIG(tooltip)
        doOpenYaml->setText(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275yaml\346\226\207\344\273\266", nullptr));
#if QT_CONFIG(tooltip)
        doOpenYaml->setToolTip(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275yaml\346\226\207\344\273\266", nullptr));
#endif // QT_CONFIG(tooltip)
        doDrawLine->setText(QCoreApplication::translate("MainWindow", "\347\273\230\345\210\266\350\231\232\346\213\237\345\242\231", nullptr));
#if QT_CONFIG(tooltip)
        doDrawLine->setToolTip(QCoreApplication::translate("MainWindow", "\347\273\230\345\210\266\350\231\232\346\213\237\345\242\231", nullptr));
#endif // QT_CONFIG(tooltip)
        doDrawTraj_png->setText(QCoreApplication::translate("MainWindow", "\347\273\230\345\210\266\350\275\250\350\277\271\345\210\260png", nullptr));
#if QT_CONFIG(tooltip)
        doDrawTraj_png->setToolTip(QCoreApplication::translate("MainWindow", "\347\273\230\345\210\266\350\275\250\350\277\271\345\210\260png", nullptr));
#endif // QT_CONFIG(tooltip)
        do_draw_polygons_area->setText(QCoreApplication::translate("MainWindow", "\347\273\230\345\210\266\345\244\232\350\276\271\345\275\242\345\214\272\345\237\237", nullptr));
#if QT_CONFIG(tooltip)
        do_draw_polygons_area->setToolTip(QCoreApplication::translate("MainWindow", "\347\273\230\345\210\266\345\244\232\350\276\271\345\275\242\345\214\272\345\237\237", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        do_draw_polygons_area->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+4", nullptr));
#endif // QT_CONFIG(shortcut)
        do_draw_ploygons->setText(QCoreApplication::translate("MainWindow", "\345\241\253\345\205\205\345\214\272\345\237\237", nullptr));
#if QT_CONFIG(tooltip)
        do_draw_ploygons->setToolTip(QCoreApplication::translate("MainWindow", "\345\241\253\345\205\205\345\214\272\345\237\237", nullptr));
#endif // QT_CONFIG(tooltip)
        do_adjust_polygons_area->setText(QCoreApplication::translate("MainWindow", "\346\230\276\347\244\272\345\244\232\350\276\271\345\275\242\345\214\272\345\237\237", nullptr));
#if QT_CONFIG(tooltip)
        do_adjust_polygons_area->setToolTip(QCoreApplication::translate("MainWindow", "\346\230\276\347\244\272\345\244\232\350\276\271\345\275\242\345\214\272\345\237\237", nullptr));
#endif // QT_CONFIG(tooltip)
        do_draw_full_covered_path->setText(QCoreApplication::translate("MainWindow", "\347\273\230\345\210\266\350\246\206\347\233\226\345\274\217\350\275\250\350\277\271", nullptr));
#if QT_CONFIG(tooltip)
        do_draw_full_covered_path->setToolTip(QCoreApplication::translate("MainWindow", "\347\273\230\345\210\266\350\246\206\347\233\226\345\274\217\350\275\250\350\277\271", nullptr));
#endif // QT_CONFIG(tooltip)
        do_save_polygons_area->setText(QCoreApplication::translate("MainWindow", "\350\256\260\345\275\225\345\214\272\345\237\237", nullptr));
#if QT_CONFIG(tooltip)
        do_save_polygons_area->setToolTip(QCoreApplication::translate("MainWindow", "\350\256\260\345\275\225\345\214\272\345\237\237", nullptr));
#endif // QT_CONFIG(tooltip)
        do_load_polygons_area->setText(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275\345\214\272\345\237\237", nullptr));
#if QT_CONFIG(tooltip)
        do_load_polygons_area->setToolTip(QCoreApplication::translate("MainWindow", "\350\256\260\350\275\275\345\214\272\345\237\237", nullptr));
#endif // QT_CONFIG(tooltip)
        do_clean_polygons_area->setText(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\345\214\272\345\237\237", nullptr));
#if QT_CONFIG(tooltip)
        do_clean_polygons_area->setToolTip(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\345\214\272\345\237\237", nullptr));
#endif // QT_CONFIG(tooltip)
        doDrawWhitePolygon->setText(QCoreApplication::translate("MainWindow", "\345\244\232\350\276\271\345\275\242\346\251\241\347\232\256\346\223\246", nullptr));
#if QT_CONFIG(tooltip)
        doDrawWhitePolygon->setToolTip(QCoreApplication::translate("MainWindow", "\345\244\232\350\276\271\345\275\242\346\251\241\347\232\256\346\223\246", nullptr));
#endif // QT_CONFIG(tooltip)
        doDrawPathContinue->setText(QCoreApplication::translate("MainWindow", "\350\277\275\345\212\240\350\275\250\350\277\271", nullptr));
#if QT_CONFIG(tooltip)
        doDrawPathContinue->setToolTip(QCoreApplication::translate("MainWindow", "\350\277\275\345\212\240\350\275\250\350\277\271", nullptr));
#endif // QT_CONFIG(tooltip)
        do_rtk_full_cover_path->setText(QCoreApplication::translate("MainWindow", "rtk\350\246\206\347\233\226\345\274\217\350\275\250\350\277\271", nullptr));
#if QT_CONFIG(tooltip)
        do_rtk_full_cover_path->setToolTip(QCoreApplication::translate("MainWindow", "rtk\350\246\206\347\233\226\345\274\217\350\275\250\350\277\271", nullptr));
#endif // QT_CONFIG(tooltip)
        doOpenJsonExclusively->setText(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275rtk-Json\346\226\207\344\273\266", nullptr));
#if QT_CONFIG(tooltip)
        doOpenJsonExclusively->setToolTip(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275rtk-Json\346\226\207\344\273\266", nullptr));
#endif // QT_CONFIG(tooltip)
        do_selectMultiPoint_polygen->setText(QCoreApplication::translate("MainWindow", "\346\241\206\351\200\211\347\202\271", nullptr));
#if QT_CONFIG(tooltip)
        do_selectMultiPoint_polygen->setToolTip(QCoreApplication::translate("MainWindow", "\346\241\206\351\200\211\347\202\271", nullptr));
#endif // QT_CONFIG(tooltip)
        do_addAttribute->setText(QCoreApplication::translate("MainWindow", "\346\214\211\346\241\206\351\200\211\345\214\272\346\267\273\345\212\240\345\261\236\346\200\247", nullptr));
#if QT_CONFIG(tooltip)
        do_addAttribute->setToolTip(QCoreApplication::translate("MainWindow", "\346\214\211\346\241\206\351\200\211\345\214\272\346\267\273\345\212\240\345\261\236\346\200\247", nullptr));
#endif // QT_CONFIG(tooltip)
        do_clean_pointSelect_area->setText(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\346\241\206\351\200\211", nullptr));
#if QT_CONFIG(tooltip)
        do_clean_pointSelect_area->setToolTip(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\346\241\206\351\200\211", nullptr));
#endif // QT_CONFIG(tooltip)
        doCleanAllMPath->setText(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\346\211\200\346\234\211\350\275\250\350\277\271", nullptr));
#if QT_CONFIG(tooltip)
        doCleanAllMPath->setToolTip(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\346\211\200\346\234\211\350\275\250\350\277\271", nullptr));
#endif // QT_CONFIG(tooltip)
        do_draw_next_mpath->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\345\271\266\350\277\233\350\241\214\344\270\213\344\270\200\346\235\241\350\275\250\350\277\271\347\273\230\345\210\266", nullptr));
#if QT_CONFIG(tooltip)
        do_draw_next_mpath->setToolTip(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\345\271\266\350\277\233\350\241\214\344\270\213\344\270\200\346\235\241\350\275\250\350\277\271\347\273\230\345\210\266", nullptr));
#endif // QT_CONFIG(tooltip)
        doOpenJson_multiMPath->setText(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275Json\346\226\207\344\273\266(\345\244\232\346\235\241\350\275\250\350\277\271)", nullptr));
#if QT_CONFIG(tooltip)
        doOpenJson_multiMPath->setToolTip(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275Json\346\226\207\344\273\266(\345\244\232\346\235\241\350\275\250\350\277\271)", nullptr));
#endif // QT_CONFIG(tooltip)
        do_draw_functional_area->setText(QCoreApplication::translate("MainWindow", "\347\273\230\345\210\266\345\212\237\350\203\275\345\214\272", nullptr));
#if QT_CONFIG(tooltip)
        do_draw_functional_area->setToolTip(QCoreApplication::translate("MainWindow", "\347\273\230\345\210\266\345\212\237\350\203\275\345\214\272", nullptr));
#endif // QT_CONFIG(tooltip)
        do_clean_current_functional_area->setText(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\345\275\223\345\211\215\345\212\237\350\203\275\345\214\272", nullptr));
#if QT_CONFIG(tooltip)
        do_clean_current_functional_area->setToolTip(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\345\275\223\345\211\215\345\212\237\350\203\275\345\214\272", nullptr));
#endif // QT_CONFIG(tooltip)
        do_clean_all_functional_area->setText(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\346\211\200\346\234\211\345\212\237\350\203\275\345\214\272", nullptr));
#if QT_CONFIG(tooltip)
        do_clean_all_functional_area->setToolTip(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\346\211\200\346\234\211\345\212\237\350\203\275\345\214\272", nullptr));
#endif // QT_CONFIG(tooltip)
        do_draw_next_functional_area->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\345\271\266\350\277\233\350\241\214\344\270\213\344\270\200\344\270\252\345\212\237\350\203\275\345\214\272\347\273\230\345\210\266", nullptr));
#if QT_CONFIG(tooltip)
        do_draw_next_functional_area->setToolTip(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\345\271\266\350\277\233\350\241\214\344\270\213\344\270\200\344\270\252\345\212\237\350\203\275\345\214\272\347\273\230\345\210\266", nullptr));
#endif // QT_CONFIG(tooltip)
        do_save_functional_area->setText(QCoreApplication::translate("MainWindow", "\350\256\260\345\275\225\345\212\237\350\203\275\345\214\272", nullptr));
#if QT_CONFIG(tooltip)
        do_save_functional_area->setToolTip(QCoreApplication::translate("MainWindow", "\350\256\260\345\275\225\345\212\237\350\203\275\345\214\272", nullptr));
#endif // QT_CONFIG(tooltip)
        do_load_functional_area->setText(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275\345\212\237\350\203\275\345\214\272", nullptr));
#if QT_CONFIG(tooltip)
        do_load_functional_area->setToolTip(QCoreApplication::translate("MainWindow", "\345\212\240\350\275\275\345\212\237\350\203\275\345\214\272", nullptr));
#endif // QT_CONFIG(tooltip)
        doSaveJson_unfilled->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230Json\346\226\207\344\273\266(\344\270\215\345\241\253\345\205\205\350\275\250\350\277\271)", nullptr));
#if QT_CONFIG(tooltip)
        doSaveJson_unfilled->setToolTip(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230Json\346\226\207\344\273\266(\344\270\215\345\241\253\345\205\205\350\275\250\350\277\271)", nullptr));
#endif // QT_CONFIG(tooltip)
        do_selectMultiPoint_pointSelect->setText(QCoreApplication::translate("MainWindow", "\347\202\271\351\200\211\347\202\271(\351\200\211\345\256\232\345\274\200\345\247\213/\347\273\223\346\235\237\347\202\271)", nullptr));
#if QT_CONFIG(tooltip)
        do_selectMultiPoint_pointSelect->setToolTip(QCoreApplication::translate("MainWindow", "\347\202\271\351\200\211\347\202\271(\351\200\211\345\256\232\345\274\200\345\247\213/\347\273\223\346\235\237\347\202\271)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        do_selectMultiPoint_pointSelect->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+1", nullptr));
#endif // QT_CONFIG(shortcut)
        do_clean_pointSelect_pointSelect->setText(QCoreApplication::translate("MainWindow", "\345\217\226\346\266\210\351\200\211\347\202\271", nullptr));
#if QT_CONFIG(tooltip)
        do_clean_pointSelect_pointSelect->setToolTip(QCoreApplication::translate("MainWindow", "\345\217\226\346\266\210\351\200\211\347\202\271", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        do_clean_pointSelect_pointSelect->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+2", nullptr));
#endif // QT_CONFIG(shortcut)
        do_addAttribute_pointSelect->setText(QCoreApplication::translate("MainWindow", "\346\214\211\347\202\271\351\200\211\345\214\272\346\267\273\345\212\240\345\261\236\346\200\247", nullptr));
#if QT_CONFIG(tooltip)
        do_addAttribute_pointSelect->setToolTip(QCoreApplication::translate("MainWindow", "\346\214\211\347\202\271\351\200\211\345\214\272\346\267\273\345\212\240\345\261\236\346\200\247", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        do_addAttribute_pointSelect->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+3", nullptr));
#endif // QT_CONFIG(shortcut)
        do_copyPoints_pointSelect->setText(QCoreApplication::translate("MainWindow", "\345\244\215\345\210\266\347\202\271\351\200\211\345\214\272", nullptr));
#if QT_CONFIG(tooltip)
        do_copyPoints_pointSelect->setToolTip(QCoreApplication::translate("MainWindow", "\345\244\215\345\210\266\347\202\271\351\200\211\345\214\272", nullptr));
#endif // QT_CONFIG(tooltip)
        do_deletePoints_pointSelect->setText(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244\347\202\271\351\200\211\345\214\272", nullptr));
#if QT_CONFIG(tooltip)
        do_deletePoints_pointSelect->setToolTip(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244\347\202\271\351\200\211\345\214\272", nullptr));
#endif // QT_CONFIG(tooltip)
        do_movePoints_pointSelect->setText(QCoreApplication::translate("MainWindow", "\347\247\273\345\212\250\347\202\271\351\200\211\345\214\272", nullptr));
#if QT_CONFIG(tooltip)
        do_movePoints_pointSelect->setToolTip(QCoreApplication::translate("MainWindow", "\347\247\273\345\212\250\347\202\271\351\200\211\345\214\272", nullptr));
#endif // QT_CONFIG(tooltip)
        do_drawLine_pointsSelectedBased->setText(QCoreApplication::translate("MainWindow", "\346\262\277\350\275\250\350\277\271\347\273\230\345\210\266\350\231\232\346\213\237\345\242\231", nullptr));
#if QT_CONFIG(tooltip)
        do_drawLine_pointsSelectedBased->setToolTip(QCoreApplication::translate("MainWindow", "\346\262\277\350\275\250\350\277\271\347\273\230\345\210\266\350\231\232\346\213\237\345\242\231", nullptr));
#endif // QT_CONFIG(tooltip)
        do_modifyPGMSize->setText(QCoreApplication::translate("MainWindow", "\344\277\256\346\224\271\345\234\260\345\233\276\345\260\272\345\257\270", nullptr));
#if QT_CONFIG(tooltip)
        do_modifyPGMSize->setToolTip(QCoreApplication::translate("MainWindow", "\344\277\256\346\224\271\345\234\260\345\233\276\345\260\272\345\257\270", nullptr));
#endif // QT_CONFIG(tooltip)
        do_draw_parkingLot_area->setText(QCoreApplication::translate("MainWindow", "\345\201\234\350\275\246\345\214\272\347\273\230\345\210\266", nullptr));
#if QT_CONFIG(tooltip)
        do_draw_parkingLot_area->setToolTip(QCoreApplication::translate("MainWindow", "\345\201\234\350\275\246\345\214\272\347\273\230\345\210\266", nullptr));
#endif // QT_CONFIG(tooltip)
        do_save_parkingLot_area->setText(QCoreApplication::translate("MainWindow", "\345\201\234\350\275\246\345\214\272\344\277\235\345\255\230", nullptr));
#if QT_CONFIG(tooltip)
        do_save_parkingLot_area->setToolTip(QCoreApplication::translate("MainWindow", "\345\201\234\350\275\246\345\214\272\344\277\235\345\255\230", nullptr));
#endif // QT_CONFIG(tooltip)
        do_saveYaml_forRTK->setText(QCoreApplication::translate("MainWindow", "\344\277\256\346\255\243yaml\346\226\207\344\273\266(\351\234\200\350\246\201rtk\345\273\272\345\233\276\346\227\266yaml\345\216\237\346\226\207\344\273\266)(\350\213\245rtk\345\234\260\345\233\276\345\220\253\346\234\211\347\273\230\345\210\266\345\206\205\345\256\271\357\274\214\345\210\231\345\277\205\351\241\273\344\277\256\346\255\243)", nullptr));
#if QT_CONFIG(tooltip)
        do_saveYaml_forRTK->setToolTip(QCoreApplication::translate("MainWindow", "\344\277\256\346\255\243yaml\346\226\207\344\273\266", nullptr));
#endif // QT_CONFIG(tooltip)
        do_load_parkingLot_area->setText(QCoreApplication::translate("MainWindow", "\345\201\234\350\275\246\345\214\272\345\212\240\350\275\275", nullptr));
#if QT_CONFIG(tooltip)
        do_load_parkingLot_area->setToolTip(QCoreApplication::translate("MainWindow", "\345\201\234\350\275\246\345\214\272\345\212\240\350\275\275", nullptr));
#endif // QT_CONFIG(tooltip)
        do_roslaunch_lslidar_mapping->setText(QCoreApplication::translate("MainWindow", "\345\220\257\345\212\250\345\273\272\345\233\276\350\212\202\347\202\271", nullptr));
#if QT_CONFIG(tooltip)
        do_roslaunch_lslidar_mapping->setToolTip(QCoreApplication::translate("MainWindow", "\345\220\257\345\212\250\345\273\272\345\233\276\350\212\202\347\202\271", nullptr));
#endif // QT_CONFIG(tooltip)
        do_rosbag_play->setText(QCoreApplication::translate("MainWindow", "\346\222\255\346\224\276\346\277\200\345\205\211\351\233\267\350\276\276\346\225\260\346\215\256", nullptr));
#if QT_CONFIG(tooltip)
        do_rosbag_play->setToolTip(QCoreApplication::translate("MainWindow", "\346\222\255\346\224\276\346\277\200\345\205\211\351\233\267\350\276\276\346\225\260\346\215\256", nullptr));
#endif // QT_CONFIG(tooltip)
        do_save_mapping_file->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\345\234\260\345\233\276\346\226\207\344\273\266", nullptr));
#if QT_CONFIG(tooltip)
        do_save_mapping_file->setToolTip(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\345\234\260\345\233\276\346\226\207\344\273\266", nullptr));
#endif // QT_CONFIG(tooltip)
        do_kill_mapping_and_rosbag_Node->setText(QCoreApplication::translate("MainWindow", "\347\273\223\346\235\237\345\273\272\345\233\276\344\270\216rosbag\350\212\202\347\202\271", nullptr));
#if QT_CONFIG(tooltip)
        do_kill_mapping_and_rosbag_Node->setToolTip(QCoreApplication::translate("MainWindow", "\347\273\223\346\235\237\345\273\272\345\233\276\344\270\216rosbag\350\212\202\347\202\271", nullptr));
#endif // QT_CONFIG(tooltip)
        menu->setTitle(QCoreApplication::translate("MainWindow", "\345\274\200\345\247\213", nullptr));
        menu_2->setTitle(QCoreApplication::translate("MainWindow", "\345\233\276\347\211\207\347\274\226\350\276\221", nullptr));
        menu_3->setTitle(QCoreApplication::translate("MainWindow", "\345\270\256\345\212\251", nullptr));
        menu_4->setTitle(QCoreApplication::translate("MainWindow", "\350\275\250\350\277\271\347\274\226\350\276\221", nullptr));
        menu_5->setTitle(QCoreApplication::translate("MainWindow", "\350\246\206\347\233\226\345\274\217\350\275\250\350\277\271", nullptr));
        menurtk->setTitle(QCoreApplication::translate("MainWindow", "rtk\350\275\250\350\277\271\347\274\226\350\276\221", nullptr));
        menu_6->setTitle(QCoreApplication::translate("MainWindow", "\350\275\250\350\277\271\346\240\207\346\263\250", nullptr));
        menu_7->setTitle(QCoreApplication::translate("MainWindow", "\345\212\237\350\203\275\345\214\272\347\273\230\345\210\266", nullptr));
        menu_8->setTitle(QCoreApplication::translate("MainWindow", "\347\246\273\347\272\277\345\273\272\345\233\276", nullptr));
        toolBar_File->setWindowTitle(QCoreApplication::translate("MainWindow", "toolBar", nullptr));
        toolBar_Edit->setWindowTitle(QCoreApplication::translate("MainWindow", "toolBar", nullptr));
        toolBar_PathEdit->setWindowTitle(QCoreApplication::translate("MainWindow", "toolBar", nullptr));
        toolBar_attrEdit->setWindowTitle(QCoreApplication::translate("MainWindow", "toolBar", nullptr));
        toolBar_fullCover->setWindowTitle(QCoreApplication::translate("MainWindow", "toolBar", nullptr));
        toolBar_About->setWindowTitle(QCoreApplication::translate("MainWindow", "toolBar", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
