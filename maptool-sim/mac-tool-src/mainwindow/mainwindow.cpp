#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../../core/grid_projection.h"   //T1.5: pcd 加载/投影

#include <QDockWidget>
#include <QAction>
#include <QMenuBar>

#include <iomanip>     // std::setprecision —— MSVC 会通过其它头文件顺带引入，
                       // 但 GCC/Clang（macOS）不会，必须显式包含


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //设定关闭此窗口，则关闭所有窗口
    this->setAttribute( Qt::WA_QuitOnClose, true );

    //初始化
    pgmScene = new BQGraphicsScene;  //类 用于在场景中操作大量的2D图形元素
    pgmPixmapItem = nullptr;
//    m_path = newBPathItem();    //其实会在打开图片那里cleanImage()进行初始化;
//    m_path = new BPathItem;
    qDebug()<<"m_path:"<<(void*)m_path;
    initMPath();
    qDebug()<<"m_path:"<<(void*)m_path;
    initMapSelect();
    initPointSelect();
    initAreaSelect();

    pgmScene->setItemIndexMethod(QGraphicsScene::NoIndex);  //QGraphicsScene::NoIndex：不保存图形项索引。定位图形项具有线性复杂性，因为会搜索场景中的所有图形项。但是，添加，移动和删除图形项是在固定时间内完成的。这种方法最适合动态场景，在动态场景中，要连续添加，移动或删除许多图形项。
//    pgmScene->addItem(m_path);
//    m_path->setParentItem(pgmPixmapItem);

    //初始化
    pixel = new QLabel;
    size = new QLabel;
    mousePos = new QLabel;
    info = nullptr;
    ConstImage = "";
    ConstJson = "";
    ConstYaml = "";
//    ConstImage = getUserPath();
//    ConstJson = getUserPath();
    flag = false;   //用于退出确认

    //初始化界面
    pgmScene->setBackgroundBrush(QColor::fromRgb(224,224,224));
    ui->pgmGraphicsView->setScene(pgmScene);
    ui->pgmGraphicsView->setMouseTracking(true);

    //初始化底部statusBar
    ui->statusbar->addPermanentWidget(size);
    ui->statusbar->addPermanentWidget(pixel);
    ui->statusbar->addPermanentWidget(mousePos);

    createToolBar();

    //===== T1.5 前视命中区图层接线 =====
    //菜单入口：开始菜单追加 "打开点云pcd"，帮助菜单追加 "前视快算" 开关 + 控制面板
    fovLayer = new FovLayer(this);
    fovLayer->attachScene(pgmScene);
    {
        QAction* openPcd = new QAction(tr("打开点云pcd"), this);
        openPcd->setObjectName("doOpenPcd");
        connect(openPcd, &QAction::triggered, this, &MainWindow::on_doOpenPcd_triggered);
        ui->menubar->findChild<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
        //加到"开始"菜单（menu）末尾
        QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
        if (!menus.isEmpty()) menus.first()->addAction(openPcd);

        QAction* fovToggle = new QAction(tr("前视快算"), this);
        fovToggle->setObjectName("doFovToggle");
        fovToggle->setCheckable(true);
        connect(fovToggle, &QAction::triggered, this, &MainWindow::on_doFovToggle_triggered);
        if (!menus.isEmpty()) menus.first()->addAction(fovToggle);
        //控制面板 dock（含开关按钮，与 menu action 双向联动）
        QDockWidget* fovDock = fovLayer->createControlDock(this);
        addDockWidget(Qt::RightDockWidgetArea, fovDock);
        fovDock->hide();    //默认隐藏，避免改变现有布局；需要时从"窗口"里唤出
        connect(fovToggle, &QAction::toggled, fovDock, &QDockWidget::setVisible);
        connect(fovLayer, &FovLayer::zmaxChanged, this, [this](double v) {
            if (fovCloud) {
                fovGrid = std::make_unique<blz::GridProjection>(
                    blz::project_to_grid(*fovCloud, 0.4, 0.05, v));
                fovLayer->setGrid(fovGrid.get());
            }
            if (fovLayer->enabled()) fovLayer->recalcAll();
        });
    }
    //===== T2.4 直播跟车接线 =====
    liveLayer = new LiveLayer(this);
    liveLayer->attachScene(pgmScene);
    {
        QAction* liveToggle = new QAction(tr("直播跟车"), this);
        liveToggle->setObjectName("doLiveToggle");
        liveToggle->setCheckable(true);
        QList<QMenu*> menus2 = ui->menubar->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
        if (!menus2.isEmpty()) menus2.first()->addAction(liveToggle);

        QDockWidget* liveDock = liveLayer->createControlDock(this);
        addDockWidget(Qt::RightDockWidgetArea, liveDock);
        liveDock->hide();
        connect(liveToggle, &QAction::toggled, liveDock, &QDockWidget::setVisible);
    }
    //===== T2.4 接线结束 =====

    //===== T4.1 回归任务面板 =====
    {
        QAction* regPanel = new QAction(tr("回归任务面板"), this);
        regPanel->setObjectName("doRegressionPanel");
        QList<QMenu*> menus3 = ui->menubar->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
        if (!menus3.isEmpty()) menus3.first()->addAction(regPanel);
        connect(regPanel, &QAction::triggered, this, [this]() {
            if (!regressionPanel) {
                regressionPanel = new RegressionPanel(
                    QStringLiteral("http://127.0.0.1:9100"),
                    QStringLiteral("dev-token"), this);
                // 提交成功后自动填入直播跟车的 run_id
                connect(regressionPanel, &RegressionPanel::runSubmitted,
                        this, [this](const QString& runId) {
                    if (liveLayer) {
                        // 自动断开旧连接，填入新 run_id
                        liveLayer->stopLive();
                        // 通过 dock 里的 run_id 输入框填入（简单方案）
                        // 实际上可以通过 findChild 找到 QLineEdit
                        if (auto* dock = findChild<QDockWidget*>("liveControlDock")) {
                            if (auto* edit = dock->findChild<QLineEdit*>()) {
                                // 找到 run_id 输入框（第二个 QLineEdit）
                                auto edits = dock->findChildren<QLineEdit*>();
                                if (edits.size() >= 2) edits[1]->setText(runId);
                            }
                        }
                    }
                    ui->statusbar->showMessage(tr("run 已提交：%1").arg(runId), 8000);
                });
            }
            regressionPanel->show();
            regressionPanel->raise();
        });
    }

    //===== T1.5 接线结束 =====

    setActionStatus(false);
    setWindowTitle("Map Tool");

    //BPathItem : m_path
//    connect(pgmScene, SIGNAL(updatePoint(QPointF,QList<QPointF>,bool)), m_path, SLOT(pushPoint(QPointF,QList<QPointF>,bool)));
    connect(pgmScene, &BQGraphicsScene::createFinished, this, [=](){
        setActionStatus(true);
        ui->statusbar->showMessage( "选择点进行调整(左键点击拖动，左键选中后右键)，调整结束选择'轨迹绘制-保存json文件'保存路径" );
    });
    //MapSelectItem :
    connect(pgmScene, &BQGraphicsScene::createFinished_mp, this, [=](){
        setActionStatus(true);
        ui->statusbar->showMessage( "可选择点进行调整或继续绘制多边形区域，或者进行区域填充，调整结束可进行覆盖式轨迹生成" );
    });
    //MapSelectItem : pointSelecrted
    connect(pgmScene, &BQGraphicsScene::createFinished_pp, this, [=](){
        setActionStatus(true);
        ui->statusbar->showMessage( "可选择点进行调整或继续绘制多边形区域，调整结束可对点进行添加属性" );
    });
    //MapSelectItem : areaSelect
    connect(pgmScene, &BQGraphicsScene::createFinished_ap, this, [=](){
        setActionStatus(true);
        ui->statusbar->showMessage( "可选择点进行调整或继续绘制功能区，调整结束可进行下一功能区绘制或者保存区域信息" );
    });

    //CoverPathDialog : coverPathDialog
    connect(pgmScene, &BQGraphicsScene::selected_startPoint_accept,
            this, [=](const QPointF& point)
            {
        setActionStatus(true);
        ui->statusbar->showMessage( QString("覆盖式轨迹起始点选择为(%1, %2)").arg(point.x()).arg(point.y()), 5000 );
        coverPathDialog->activateWindow();
            });
    connect(pgmScene, &BQGraphicsScene::selected_startPoint_reject,
            this, [=](){
        setActionStatus(true);
        ui->statusbar->showMessage( "取消选择", 3000 );
            });

    //
    connect(pgmScene, SIGNAL(drawLine(QList<QPointF>)), this, SLOT(drawVirtualWall(QList<QPointF>)) );

    //curMousePos
    connect(pgmScene, &BQGraphicsScene::curMousePos, this, [=](QPointF p){
        mousePos->setText( "x: " + QString::number(p.x(), 'f', 2) + " y: " + QString::number(p.y(), 'f', 2) );
    });

    //erasePolygon
    connect(pgmScene, &BQGraphicsScene::createFinished_ep, this, &MainWindow::drawWhitePolygon);

    connect(pgmScene, &BQGraphicsScene::created_parkingLot_polygon_setWidth, this,
            [=](){
                setActionStatus(true);
                ui->statusbar->showMessage( "完成", 3000 );
            });

    if(__cplusplus == 201402L)
    qDebug()<<"c++_version: "<<__cplusplus;

//    ui->do_rtk_full_cover_path->setEnabled(false);
//    ui->doOpenJsonExclusively->setEnabled(false);
//    ui->do_draw_full_covered_path->setEnabled(false);

}

//初始化m_path //此函数禁止直接调用
void MainWindow::initMPath()    //此函数禁止直接调用
{
    if ( m_path )
    {
        disconnect(pgmScene, SIGNAL(updatePoint(QPointF,QList<QPointF>,bool)), m_path, SLOT(pushPoint(QPointF,QList<QPointF>,bool)));
        if ( m_path->scene() == pgmScene )
            pgmScene->removeItem( m_path );
        delete m_path;
    }

    m_path = new BPathItem;
    m_path->setPathWidth(m_path_width);

    //BPathItem : m_path
    connect(pgmScene, SIGNAL(updatePoint(QPointF,QList<QPointF>,bool)), m_path, SLOT(pushPoint(QPointF,QList<QPointF>,bool)));

}

//初始化mapSelected
void MainWindow::initMapSelect()
{
    if ( mapSelected )
    {
        disconnect(pgmScene, SIGNAL(map_ploygon_addPoint(QPointF,bool)), mapSelected, SLOT(cur_ploygon_addPoint(QPointF,bool)));
        if ( mapSelected->scene() == pgmScene )
            pgmScene->removeItem(mapSelected);
        delete mapSelected;
    }

    mapSelected = new MapSelectItem;

    //MapSelectItem : mapSelected
    connect(pgmScene, SIGNAL(map_ploygon_addPoint(QPointF,bool)), mapSelected, SLOT(cur_ploygon_addPoint(QPointF,bool)));

}

//初始化pointSelected
void MainWindow::initPointSelect()
{
    if( pointSelected )
    {
        disconnect(pgmScene, SIGNAL(pointSelect_polygon_addPoint(QPointF,bool)), pointSelected, SLOT(cur_ploygon_addPoint(QPointF,bool)));
        if ( pointSelected->scene() == pgmScene )
            pgmScene->removeItem(pointSelected);
        delete pointSelected;
    }

    pointSelected = new MapSelectItem;

    //MapSelectItem : pointSelecrted
    connect(pgmScene, SIGNAL(pointSelect_polygon_addPoint(QPointF,bool)), pointSelected, SLOT(cur_ploygon_addPoint(QPointF,bool)));

}

//初始化areaSelected
void MainWindow::initAreaSelect()
{
    if( areaSelected )
    {
        disconnect(pgmScene, SIGNAL(areaSelect_polygon_addPoint(QPointF,bool)), areaSelected, SLOT(cur_ploygon_addPoint(QPointF,bool)));
        if ( areaSelected->scene() == pgmScene )
            pgmScene->removeItem(areaSelected);
        delete areaSelected;
    }

    areaSelected = new MapSelectItem;

    //MapSelectItem : areaSelecrted
    connect(pgmScene, SIGNAL(areaSelect_polygon_addPoint(QPointF,bool)), areaSelected, SLOT(cur_ploygon_addPoint(QPointF,bool)));

}

void MainWindow::initParkingLotSelect()
{
    if ( parkingLotSelected )
    {
        disconnect(pgmScene, nullptr, parkingLotSelected, nullptr );
        if ( parkingLotSelected->scene() == pgmScene )
            pgmScene->removeItem(parkingLotSelected);
        delete parkingLotSelected;
        parkingLotSelected = nullptr;
    }
    parkingLotSelected = new ParkingLotItem;

    connect(pgmScene, &BQGraphicsScene::created_parkingLot_polygon_setStartPoint, parkingLotSelected, &ParkingLotItem::setStartPoint);
    connect(pgmScene, &BQGraphicsScene::created_parkingLot_polygon_setEndPoint, parkingLotSelected, &ParkingLotItem::setEndPoint);
    connect(pgmScene, &BQGraphicsScene::created_parkingLot_polygon_setWidth, parkingLotSelected, &ParkingLotItem::setWidth);
}

MainWindow::~MainWindow()
{
    delete ui;

    if(pgmScene)
    {
        delete pgmScene;
        pgmScene = nullptr;
    }

    if(size)
    {
        delete size;
        size = nullptr;
    }

    if(pixel)
    {
        delete pixel;
        pixel = nullptr;
    }

    if(mousePos)
    {
        delete mousePos;
        mousePos = nullptr;
    }
}

//退出事件
void MainWindow::closeEvent(QCloseEvent *event){
    //是否更改
    if(flag){
        if(QMessageBox::Yes == QMessageBox::question(this,tr("尚未保存图片"),tr("是否保存？"),QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes)){
            on_doSave_triggered();
        }
    }
    if(QMessageBox::Yes == QMessageBox::question(this,tr("退出"),tr("是否退出？"),QMessageBox::Yes | QMessageBox::No,QMessageBox::No)){
        event->accept();
    }else {
        event->ignore();
    }
}

// 更新图片
void MainWindow::updateImage(const QPixmap &pixmap)
{
    //刷新图片
    pgmPixmapItem->setPixmap(pixmap);
//    pgmScene->setSceneRect(QRectF(pixmap.rect()));
//    on_do_adjust_triggered();
    QRectF itemsRect = pgmScene->itemsBoundingRect();
    // 设置视图的场景矩形大小
    pgmScene->setSceneRect(QRectF(itemsRect.left()-1000,itemsRect.top()-1000,itemsRect.width()+2000,itemsRect.height()+2000));

}

//初始化toolBar
void MainWindow::createToolBar()
{
    ui->toolBar_File->addAction(ui->doOpen);
    ui->toolBar_File->addAction(ui->doOpenJsonExclusively);
    ui->toolBar_File->addAction(ui->doSave);
    ui->toolBar_File->addAction(ui->doSave_As);
    ui->toolBar_File->addSeparator();
    ui->toolBar_File->addAction(ui->doClose);

    ui->toolBar_Edit->addAction(ui->doRestore);
    ui->toolBar_Edit->addAction(ui->do_adjust);
    ui->toolBar_Edit->addSeparator();
    ui->toolBar_Edit->addAction(ui->do_AutoPS_wec);
    ui->toolBar_Edit->addAction(ui->doDrawLine);
    ui->toolBar_Edit->addAction(ui->doDrawWhitePolygon);

    QLabel *label_pathWidth = new QLabel;
    label_pathWidth->setText("轨迹宽度：");

    QLineEdit *lineEdit_pathWidth = new QLineEdit;
    lineEdit_pathWidth->setEchoMode(QLineEdit::Normal);
    lineEdit_pathWidth->setPlaceholderText("140");
    //    lineEdit_pathWidth->setInputMask("0000;_");
    lineEdit_pathWidth->setFixedWidth(40);
    lineEdit_pathWidth->setMaxLength(4);
    connect(lineEdit_pathWidth, &QLineEdit::returnPressed, this, [=](){ setPathWidth(lineEdit_pathWidth->text()); });

    QLabel *label_cm = new QLabel;
    label_cm->setText("cm");

    ui->toolBar_PathEdit->addAction(ui->doOpenYaml);
    ui->toolBar_PathEdit->addSeparator();
    ui->toolBar_PathEdit->addAction(ui->doOpenJson);
    ui->toolBar_PathEdit->addAction(ui->do_draw_path);
    ui->toolBar_PathEdit->addAction(ui->doDrawPathContinue);
    ui->toolBar_PathEdit->addAction(ui->do_fill_path);
    ui->toolBar_PathEdit->addAction(ui->doCleanJson);
    ui->toolBar_PathEdit->addWidget(label_pathWidth);
    ui->toolBar_PathEdit->addWidget(lineEdit_pathWidth);
    ui->toolBar_PathEdit->addWidget(label_cm);
    ui->toolBar_PathEdit->addSeparator();
    ui->toolBar_PathEdit->addAction(ui->doSaveJson);
    ui->toolBar_PathEdit->addAction(ui->doDrawTraj_png);

    ui->toolBar_attrEdit->addAction(ui->do_selectMultiPoint_pointSelect);
    ui->toolBar_attrEdit->addAction(ui->do_clean_pointSelect_pointSelect);
    ui->toolBar_attrEdit->addAction(ui->do_addAttribute_pointSelect);

    ui->toolBar_fullCover->addAction(ui->do_draw_polygons_area);
    ui->toolBar_fullCover->addAction(ui->do_load_polygons_area);
    ui->toolBar_fullCover->addAction(ui->do_clean_polygons_area);
    ui->toolBar_fullCover->addAction(ui->do_draw_ploygons);
    ui->toolBar_fullCover->addSeparator();
    ui->toolBar_fullCover->addAction(ui->do_draw_full_covered_path);


    ui->toolBar_About->addAction(ui->doAbout);
    ui->toolBar_About->addSeparator();
    ui->toolBar_About->addAction(ui->doExit);


}

//未加载图片，将图片处理按钮设为不可用
void MainWindow::setActionStatus(bool status)
{
    ui->doCool->setEnabled(status);
    ui->doWarm->setEnabled(status);
    ui->doGauss->setEnabled(status);
    ui->doDilate->setEnabled(status);
    ui->doRotate->setEnabled(status);
    ui->doSimple->setEnabled(status);
    ui->doClosing->setEnabled(status);
    ui->doLaplace->setEnabled(status);
    ui->doOpening->setEnabled(status);
    ui->doRestore->setEnabled(status);
    ui->doSave->setEnabled(status);
    ui->doSave_As->setEnabled(status);
    ui->do_adjust->setEnabled(status);
    ui->doVertical->setEnabled(status);
    ui->doExpansion->setEnabled(status);
    ui->doGrayscale->setEnabled(status);
    ui->doHorizontal->setEnabled(status);
    ui->doMovie_frame->setEnabled(status);
    ui->doBinaryzation->setEnabled(status);
    ui->doFlower_frame->setEnabled(status);
    ui->doMeida_Filter->setEnabled(status);
    ui->do_adjust_left->setEnabled(status);
    ui->doClassic_frame->setEnabled(status);
    ui->do_adjust_right->setEnabled(status);
    ui->doBrightness->setEnabled(status);
    ui->doContour_extraction->setEnabled(status);
    ui->doZoom->setEnabled(status);
    ui->doClose->setEnabled(status);
    ui->doRotate_left->setEnabled(status);
    ui->doRotate_right->setEnabled(status);

    ui->do_AutoPS_wec->setEnabled(status);
    ui->doDrawLine->setEnabled(status);
    ui->doDrawWhitePolygon->setEnabled(status);

    //轨迹编辑
    ui->doOpenYaml->setEnabled(status);
    ui->doOpenJson->setEnabled(status);
    ui->doOpenJson_multiMPath->setEnabled(status);
    ui->do_draw_path->setEnabled(status);
    ui->doDrawPathContinue->setEnabled(status);
    ui->do_fill_path->setEnabled(status);
    ui->do_draw_next_mpath->setEnabled(status);
    ui->doCleanJson->setEnabled(status);
    ui->doCleanAllMPath->setEnabled(status);
    ui->doSaveJson->setEnabled(status);
    ui->doSaveJson_unfilled->setEnabled(status);
    ui->doDrawTraj_png->setEnabled(status);
    ui->do_selectMultiPoint_pointSelect->setEnabled(status);
    ui->do_clean_pointSelect_pointSelect->setEnabled(status);
    ui->do_copyPoints_pointSelect->setEnabled(status);
    ui->do_deletePoints_pointSelect->setEnabled(status);
    ui->do_movePoints_pointSelect->setEnabled(status);

    //覆盖式轨迹
    ui->do_draw_polygons_area->setEnabled(status);
    ui->do_draw_ploygons->setEnabled(status);
    ui->do_adjust_polygons_area->setEnabled(status);
    ui->do_save_polygons_area->setEnabled(status);
    ui->do_clean_polygons_area->setEnabled(status);
    ui->do_load_polygons_area->setEnabled(status);
    ui->do_draw_full_covered_path->setEnabled(status);

    //rtk轨迹编辑
//    ui->doOpenJsonExclusively->setEnabled(true);
    ui->do_modifyPGMSize->setEnabled(status);
    ui->do_drawLine_pointsSelectedBased->setEnabled(status);
    ui->do_rtk_full_cover_path->setEnabled(status);
    ui->do_saveYaml_forRTK->setEnabled(status);

    //轨迹标注
    ui->do_selectMultiPoint_polygen->setEnabled(status);
    ui->do_clean_pointSelect_area->setEnabled(status);
    ui->do_addAttribute->setEnabled(status);
    ui->do_addAttribute_pointSelect->setEnabled(status);

    //功能区绘制
    ui->do_draw_functional_area->setEnabled(false);
    ui->do_clean_current_functional_area->setEnabled(false);
    ui->do_clean_all_functional_area->setEnabled(false);
    ui->do_draw_next_functional_area->setEnabled(false);
    ui->do_draw_parkingLot_area->setEnabled(status);
    ui->do_save_parkingLot_area->setEnabled(status);
    ui->do_save_functional_area->setEnabled(false);
//    ui->do_load_functional_area->setEnabled(status);

}

//在绘物体的过程中，将所有按钮设为不可用
void MainWindow::setActionStatus_all(bool status)
{
    ui->doOpen->setEnabled(status);
    ui->doSave->setEnabled(status);
    ui->doSave_As->setEnabled(status);
    ui->doClose->setEnabled(status);

    ui->doRestore->setEnabled(status);
    ui->do_adjust->setEnabled(status);
    ui->do_AutoPS_wec->setEnabled(status);
    ui->doDrawLine->setEnabled(status);
    ui->doDrawWhitePolygon->setEnabled(status);

    ui->doOpenYaml->setEnabled(status);
    ui->doOpenJson->setEnabled(status);
    ui->doOpenJson_multiMPath->setEnabled(status);
    ui->do_draw_path->setEnabled(status);
    ui->doDrawPathContinue->setEnabled(status);
    ui->do_fill_path->setEnabled(status);
    ui->do_draw_next_mpath->setEnabled(status);
    ui->doCleanJson->setEnabled(status);
    ui->doCleanAllMPath->setEnabled(status);
    ui->doSaveJson->setEnabled(status);
    ui->doSaveJson_unfilled->setEnabled(status);
    ui->doDrawTraj_png->setEnabled(status);

    ui->do_draw_polygons_area->setEnabled(status);
    ui->do_draw_ploygons->setEnabled(status);
    ui->do_adjust_polygons_area->setEnabled(status);
    ui->do_load_polygons_area->setEnabled(status);
    ui->do_clean_polygons_area->setEnabled(status);
    ui->do_save_polygons_area->setEnabled(status);
    ui->do_draw_full_covered_path->setEnabled(status);

    ui->doOpenJsonExclusively->setEnabled(status);
    ui->do_rtk_full_cover_path->setEnabled(status);

    ui->do_selectMultiPoint_polygen->setEnabled(status);
    ui->do_clean_pointSelect_area->setEnabled(status);
    ui->do_addAttribute->setEnabled(status);

//    ui->do_draw_functional_area->setEnabled(status);
//    ui->do_clean_current_functional_area->setEnabled(status);
//    ui->do_clean_all_functional_area->setEnabled(status);
//    ui->do_draw_next_functional_area->setEnabled(status);
//    ui->do_save_functional_area->setEnabled(status);
    ui->do_draw_functional_area->setEnabled(false);
    ui->do_clean_current_functional_area->setEnabled(false);
    ui->do_clean_all_functional_area->setEnabled(false);
    ui->do_draw_next_functional_area->setEnabled(false);
    ui->do_save_functional_area->setEnabled(false);

    ui->do_draw_parkingLot_area->setEnabled(status);
    ui->do_save_parkingLot_area->setEnabled(status);

    ui->doAbout->setEnabled(status);
}

//  清理图片
void MainWindow::cleanImage()
{
    //清理
    on_do_clean_pointSelect_pointSelect_triggered();
    qDebug()<<"1";
    on_doCleanAllMPath_triggered();
    qDebug()<<"m_path:"<<(void*)m_path;
    //初始化m_path轨迹大小
    m_path_width = 1;
    m_path->setPathWidth(m_path_width);
    //清空选取区域
    initMapSelect();
    initPointSelect();
    on_do_clean_all_functional_area_triggered();
    qDebug()<<"1:items:"<<pgmScene->items();
    pgmScene->clear();  //Removes and deletes all items from the scene, but otherwise leaves the state of the scene unchanged.
    qDebug()<<"2:items:"<<pgmScene->items();
    qDebug()<<"3:test_parkingLotSelected:"<<parkingLotSelected.isNull();
    //重置
    ui->pgmGraphicsView->resetTransform();
    pgmScene->setSceneRect(QRectF());

    //重置信息
    info = nullptr;
    imgSrc.release();
    imgTmp.release();
    pgmPixmapItem = nullptr;
    isBlankMap = false;

    //更新statusBar
    if (size)
    {
        delete size;
        size = new QLabel;
        ui->statusbar->addPermanentWidget(size);
    }
    if(pixel){
        delete pixel;
        pixel = new QLabel;
        ui->statusbar->addPermanentWidget(pixel);
    }
    //还原标题
    this->setWindowTitle(WINDOW_TITLE);

    //还原地图信息
    resolution = 0;
    origin_x = 0;
    origin_y = 0;

    //T1.5:关闭时清掉前视点云缓存（unique_ptr 自带置空）
    //注意：上面 pgmScene->clear() 已 delete 所有 item（含轨迹与前视图层），
    //必须先清空轨迹指针列表，再让图层重建，否则图层会访问悬垂指针
    if (liveLayer) {
        liveLayer->stopLive();          // 场景即将 clear，先断开直播避免写悬垂
    }
    if (fovLayer) {
        fovLayer->setPathList(QList<BPathItem*>());   // 清悬垂轨迹指针（先于重建）
        fovLayer->setGrid(nullptr);
        fovLayer->setMapInfo(0, 0, 0, 0);
        fovRefreshPaths();
    }
    fovGrid.reset();
    fovCloud.reset();

    //控件设为不可用
    setActionStatus(false);
}

//    打开图片
void MainWindow::on_doOpen_triggered()
{
    if(flag){
        if(QMessageBox::Yes==QMessageBox::question(this,tr("尚未保存"),tr("是否返回操作？"),QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes)){
            return;
        }
    }
    /*
     *TODO:第一次打开桌面选择,第二次打开上次目录
     */
    if(ConstImage==""){
        ConstImage = getUserPath();
    }
    QString imagePath = QFileDialog::getOpenFileName(this, tr("Open image"), ConstImage,
                                                     tr("Image PGM (*.pgm);;"
                                                         "All Images (*.bpm *.jpg *.jpeg *.png *.tif *.pcx *.tga *.svg *.ico);;"
                                                        "All Files (*);;"
                                                        "Image BPM (*.bpm);; Image JPG (*.jpg);;"
                                                        "Image JPEG (*.jpeg);;"
                                                        "Image PNG (*.png);;"));
    ConstImage = imagePath;

    //TODO:多种图片格式
    if ( !imagePath.endsWith( ".pgm" ) ) ui->statusbar->showMessage( "非pgm图片，注意图片格式，图片处理可能出错！" );

    //如果这个目录存在
    if (!imagePath.isEmpty())//首先地址
    {
        QFile file(imagePath);
        //指定打开文件模式
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr(WINDOW_ERROR), tr("不能打开这个文件！"));
            return;
        }

        //将之前的图片清除掉
        cleanImage();

        // 更新图片信息
        info = new QFileInfo(imagePath);

        //显示图片
//        QPixmap pgmPixmap(imagePath); //之前的加载方式
        imgSrc = cv::imread(UTF8ToGBK(imagePath), CV_8U);

        file.close();

        if(!imgSrc.data)
        {
            QMessageBox::critical( this, tr(WINDOW_ERROR), tr("No image data \ncheck the <Image_Path>"));
            return;
        }

        imgTmp = imgSrc.clone();

        QPixmap pgmPixmap = CvMat8UToQPixmap(imgSrc);
        pgmPixmapItem = pgmScene->addPixmap(pgmPixmap);//设置图片
        pgmScene->setSceneRect(QRectF(pgmPixmap.rect()));//更新坐标

        //调整使得图片适应边框
        on_do_adjust_triggered();

        //更改软件标题
        this->setWindowTitle(info->fileName() + " - Map Tool");

        //设置操作可用
        setActionStatus(true);

        //statusBar更新信息
        double size_h = info->size()*1.0/1024;
        QString size_type = " KB";
        if(size_h >= 1024){
            size_h /= 1024;
            size_type = " MB";
        }
        size->setText(QString::number(size_h, 'f', 2) + size_type);
        pixel->setText(QString::number(pgmPixmapItem->pixmap().width())
                       + " x " + QString::number(pgmPixmapItem->pixmap().height()));
        flag = false;

        //如果图片不是pgm格式，则不继续加载json&yaml文件
        if ( !imagePath.endsWith(".pgm") ) return;

        QMessageBox msgBox;
        msgBox.setText("pgm图片已打开，是否加载同一路径下的同名yaml文件?   ");
        msgBox.setInformativeText("若不加载yaml文件，则不能设置轨迹宽度与保存轨迹!");
        QPushButton* yesButton = msgBox.addButton(tr("是"), QMessageBox::AcceptRole);
        QPushButton* moreButton = msgBox.addButton(tr("选择文件"), QMessageBox::ActionRole);
//        QPushButton* noButton = msgBox.addButton(tr("否"), QMessageBox::RejectRole);
        msgBox.addButton(tr("否"), QMessageBox::RejectRole);

        msgBox.exec();

        if(msgBox.clickedButton() == yesButton)
        {
            yamlPath = imagePath.left(imagePath.lastIndexOf(".")) + ".yaml";
            openYaml(yamlPath);
        }
        else if ( msgBox.clickedButton() == moreButton )
        {
            yamlPath = QFileDialog::getOpenFileName(this, tr("Open Yaml"), ConstImage.left(ConstImage.lastIndexOf(".")) + ".yaml",
                                                    tr("Yaml FIles (*.yaml);;"));
            openYaml(yamlPath);
        }
        else
        {
            return;
        }
    }
}

//轨迹编辑-加载Json文件(单条轨迹)
void MainWindow::on_doOpenJson_triggered()
{
    if(ConstImage == ""){
        ConstJson = getUserPath();
    }
    else{
        ConstJson = ConstImage.left(ConstImage.lastIndexOf("/"));
    }
    QString jsonPath = QFileDialog::getOpenFileName(this, tr("Open Json"), ConstJson, tr("Json FIles (*.json);;"));
    ConstJson = jsonPath;//230828//250121#为什么注释没想明白，直接单独用一个const路径不好吗
    QFile file(jsonPath);
    //指定打开文件模式
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::critical(this, tr(WINDOW_ERROR), tr("不能打开json文件！\n请检查路径！"));
        return;
    }
    //读取文件的全部内容
    QTextStream stream(&file);
    setStreamUtf8(stream);
    jsonContent = stream.readAll();
    //TODO_json:管理能否操作json相关的Action的变量

    file.close();

    qDebug()<<"json_file read";
    ui->statusbar->showMessage("json file is opened : " + jsonPath, 3000);

    // QJsonParseError类用于在JSON解析期间报告错误。
    QJsonParseError jsonError;
    // 将json解析为UTF-8编码的json文档，并从中创建一个QJsonDocument。
    // 如果解析成功，返回QJsonDocument对象，否则返回null
    QJsonDocument doc = QJsonDocument::fromJson(jsonContent.toUtf8(), &jsonError);
    // 判断是否解析失败
    if (jsonError.error != QJsonParseError::NoError && !doc.isNull()) {
        qDebug() << "Json格式错误！" << jsonError.error;
        QMessageBox::critical(this, tr(WINDOW_ERROR), tr("Json格式错误！\n") + jsonError.errorString());
        return;
    }

    qDebug()<<"json 格式正确";
    m_path->clear();
    pgmScene->addItem(m_path);

    //获取根{}
    QJsonObject rootObj = doc.object();
    //解析数组(points)
    QJsonValue pointsValue = rootObj.value("points");
    //判断是否是Array类型
    if ( pointsValue.type() == QJsonValue::Array )
    {
        //转换为QJsonArray类型
        QJsonArray pointsArray = pointsValue.toArray();

        for ( int i = 0; i < pointsArray.size(); i ++ )
        {
            QJsonValue pointValue = pointsArray.at(i);
            // 判断是否是object类型
            if (pointValue.type() == QJsonValue::Object) {
                // 转换为QJsonObject类型
                QJsonObject pointObj = pointValue.toObject();

                QJsonValue point_pixel_x = pointObj.value( "pixel_x" );
                qreal pixel_x;
                if (point_pixel_x.isUndefined())
                {
                    qDebug() << "250117#test: 不含pixel_x";
                    if (resolution == 0)
                    {
                        QMessageBox::critical(this, tr("轨迹加载失败"), tr("1.points数组中不含pixel_x字段\n2.未加载yaml文件\n") + jsonError.errorString());
                        return;
                    }
                    if (pointObj.value("x") == QJsonValue::Undefined)
                    {
                        QMessageBox::critical(this, tr("轨迹加载失败"), tr("1.points数组中既不含pixel_x字段\n2.也不含x字段\n") + jsonError.errorString());
                        return;
                    }
                    pixel_x = (pointObj.value( "x" ).toDouble() - origin_x) / resolution;
                }
                else
                    pixel_x = point_pixel_x.toDouble();
                QJsonValue point_pixel_y = pointObj.value( "pixel_y" );
                qreal pixel_y;
                if (point_pixel_y.isUndefined())
                {
                    qDebug() << "250117#test: 不含pixel_y";
                    if (resolution == 0)
                    {
                        QMessageBox::critical(this, tr("轨迹加载失败"), tr("1.points数组中不含pixel_y字段\n2.未加载yaml文件\n") + jsonError.errorString());
                        return;
                    }
                    if (pointObj.value("y") == QJsonValue::Undefined)
                    {
                        QMessageBox::critical(this, tr("轨迹加载失败"), tr("1.points数组中既不含pixel_y字段\n2.也不含y字段\n") + jsonError.errorString());
                        return;
                    }
                    int height = pgmPixmapItem->pixmap().height();
                    pixel_y = height - 1 - (pointObj.value("y").toDouble() - origin_y) / resolution;
                }
                else
                    pixel_y = point_pixel_y.toDouble();

                bool has_attr = false;
                int arr[ATTR_NUM] = {0};
                if ( pointObj.value("road_id") != QJsonValue::Undefined && (has_attr = true) )
                    arr[0] = pointObj.value("road_id").toInt();
                if ( pointObj.value("road_width") != QJsonValue::Undefined && (has_attr = true) )
                    arr[1] = pointObj.value("road_width").toInt();
                if ( pointObj.value("bypass") != QJsonValue::Undefined && (has_attr = true) )
                    arr[2] = pointObj.value("bypass").toInt();
                if ( pointObj.value("seismic") != QJsonValue::Undefined && (has_attr = true) )
                    arr[3] = pointObj.value("seismic").toInt();
                if ( pointObj.value("turnAround") != QJsonValue::Undefined && (has_attr = true) )
                    arr[4] = pointObj.value("turnAround").toInt();
                if ( pointObj.value("trash") != QJsonValue::Undefined && (has_attr = true) )
                    arr[5] = pointObj.value("trash").toInt();
                if ( pointObj.value("charge") != QJsonValue::Undefined && (has_attr = true) )
                    arr[6] = pointObj.value("charge").toInt();
                if ( pointObj.value("back") != QJsonValue::Undefined && (has_attr = true) )
                    arr[7] = pointObj.value("back").toInt();
                if ( pointObj.value("clean_flag") != QJsonValue::Undefined && (has_attr = true) )
                    arr[8] = pointObj.value("clean_flag").toInt();
                if ( pointObj.value("parking_area") != QJsonValue::Undefined && (has_attr = true) )
                    arr[9] = pointObj.value("parking_area").toInt();
                if ( pointObj.value("curve_paths") != QJsonValue::Undefined && (has_attr = true) )
                    arr[10] = pointObj.value("curve_paths").toInt();
                if ( pointObj.value("ignore_obstacles") != QJsonValue::Undefined && (has_attr = true) )
                    arr[11] = pointObj.value("ignore_obstacles").toInt();
                if ( pointObj.value("reverse_traj") != QJsonValue::Undefined && (has_attr = true) )
                    arr[12] = pointObj.value("reverse_traj").toInt();

                std::map<QString, QString> self_df;
                QStringList keys = pointObj.keys();
                for ( auto it = keys.begin(); it != keys.end(); it ++ )
                {
                    if ( !KEY.count(*it) && (has_attr = true) )
                        self_df[*it] = QString::number( pointObj.value( *it ).toInt() );
                }

                if ( has_attr )
                    m_path->addNextPoint( QPointF( pixel_x, pixel_y ), arr, self_df );
                else
                    m_path->addNextPoint( QPointF( pixel_x, pixel_y ) );
            }
        }
    }
    /*-------------------different_code-----------------------*/
//    drawPath(m_path);
    if ( resolution == 0 )
    {
        on_doOpenYaml_triggered();
    }
    /*-------------------different_code-----------------------*/
}

//func_构造单个点的 JSON 表达
QJsonObject MainWindow::createPointJson(const QPointF& point_pixel, attribute* attr, int height, qreal yaw)
{
    QJsonObject pointObj;

    qreal pixel_x = point_pixel.x();
    qreal pixel_y = point_pixel.y();

    pointObj.insert("pixel_x", pixel_x);
    pointObj.insert("pixel_y", pixel_y);
    pointObj.insert("x", pixel_x * resolution + origin_x);
    pointObj.insert("y", (height - 1 - pixel_y) * resolution + origin_y);
    pointObj.insert("yaw", yaw);

    if (attr) {
        pointObj.insert("road_id", attr->road_id);
        pointObj.insert("road_width", attr->road_width);
        pointObj.insert("bypass", attr->bypass);
        pointObj.insert("seismic", attr->seismic);
        pointObj.insert("turnAround", attr->turnAround);
        pointObj.insert("trash", attr->trash);
        pointObj.insert("charge", attr->charge);
        pointObj.insert("back", attr->back);
        pointObj.insert("clean_flag", attr->clean_flag);
        pointObj.insert("parking_area", attr->parking_area);
        pointObj.insert("curve_paths", attr->curve_paths);
        pointObj.insert("ignore_obstacles", attr->ignore_obstacles);
        pointObj.insert("reverse_traj", attr->reverse_traj);

        for (const auto& pair : attr->self_define) {
            bool ok;
            int value = pair.second.toInt(&ok);
            pointObj.insert(pair.first, ok ? value : -1);
        }
    }

    return pointObj;
}

//func_保存json文件
void MainWindow::doSaveJson()
{

    if ( resolution == 0 )
    {
        //        on_doOpenYaml_triggered();
        QMessageBox::critical(this,tr(WINDOW_ERROR),tr("未加载yaml文件或者未加载rtk-json文件"));
    }
    if ( resolution == 0 )
    {
        return;
    }

    qDebug()<<"pointsNumber : "<<m_path->pointsNumber();
    //构建points数组
    QJsonArray pointsArray;
    int height = pgmPixmapItem->pixmap().height();

    auto processPath = [&](BPathItem* path) {
        if (!path) {
            qWarning() << "Error: Path is null!";
            return;
        }
        int n = path->pointsNumber();

        for (int i = 0; i < n; ++i) {
            QPointF point_pixel = path->getPoint(i);
            attribute* attr = path->getElement(i)->attr;

            qreal yaw = 0;

            if (attr && attr->reverse_traj == 1) {
                BPointItem* nextElem = path->getElement(i + 1);
                BPointItem* prevElem = path->getElement(i - 1);

                if (!nextElem || !nextElem->attr || nextElem->attr->reverse_traj != 1) {
                    if (prevElem) {
                        QPointF prev = prevElem->getPoint();
                        yaw = atan2(prev.y() - point_pixel.y(), point_pixel.x() - prev.x());
                    } else {
                        yaw = 0; // fallback
                    }
                } else {
                    QPointF next = nextElem->getPoint();
                    yaw = atan2(point_pixel.y() - next.y(), next.x() - point_pixel.x());
                }

                yaw += M_PI;
                if (yaw > M_PI) yaw -= 2 * M_PI;
                if (yaw < -M_PI) yaw += 2 * M_PI;
            } else {
                if (n == 1) {
                    yaw = 0;
                } else if (i == 0) {
                    QPointF next = path->getPoint(i + 1);
                    yaw = atan2(point_pixel.y() - next.y(), next.x() - point_pixel.x());
                } else if (i == n - 1) {
                    QPointF prev = path->getPoint(i - 1);
                    yaw = atan2(prev.y() - point_pixel.y(), point_pixel.x() - prev.x());
                } else {
                    QPointF prev = path->getPoint(i - 1);
                    QPointF next = path->getPoint(i + 1);

                    QPointF vec1 = point_pixel - prev;
                    QPointF vec2 = next - point_pixel;
                    qreal len1 = std::hypot(vec1.x(), vec1.y());
                    if (len1 != 0)
                        vec1 /= len1;
                    qreal len2 = std::hypot(vec2.x(), vec2.y());
                    if (len2 != 0)
                        vec2 /= len2;

                    QPointF avg = (vec1 + vec2) / 2.0;
                    qDebug() << "250509#" << i << ' ' << avg;

                    yaw = atan2(-avg.y(), avg.x());
                }
            }

            QJsonObject pointObj = createPointJson(point_pixel, attr, height, yaw);
            pointsArray.append(pointObj);
        }
    };

    for (const auto& path : m_path_list) {
        processPath(path);
    }

    if (m_path) {
        processPath(m_path);
    }

    //构建json对象
    QJsonObject json;
    json.insert( "points", QJsonValue( pointsArray ) );
    json.insert( "post_process", 0 );
    //将json设置为文档的主要对象
    QJsonDocument doc;
    doc.setObject( json );

    // 重写打开文件，覆盖原有文件，达到删除文件全部内容的效果
    //    QString test = ConstImage.left(ConstImage.lastIndexOf(".")) + "_1.json";
    //    QFile writeFile( test );
    if(ConstJson == "" && ConstImage != "")//250121#讲道理constJson在这里肯定打开了吧！
    {
        ConstJson = ConstImage.left(ConstImage.lastIndexOf("/"));
    }
    else
    {
        ConstJson = getUserPath();
    }
    qDebug()<<"jsonfile1";
    ConstJson = QFileDialog::getSaveFileName(this, tr("Save json"), ConstJson,
                                             tr("file Json (*.json);;"
                                                "All files (*);;"));
    qDebug()<<"jsonfile2";
    QFile writeFile( ConstJson );
    if ( !writeFile.open( QFile::WriteOnly | QFile::Truncate ) ) {
        qDebug() << "can't open : " << ConstJson;
        return;
    }

    // 将修改后的内容写入文件
    QTextStream wirteStream( &writeFile );
    setStreamUtf8(wirteStream);		// 设置写入编码是UTF8
    wirteStream << doc.toJson();		// 写入文件
    writeFile.close();					// 关闭文件
    //
    ui->statusbar->showMessage( "json文件已保存 : " + ConstJson, 3000 );
}

//保存json文件
void MainWindow::on_doSaveJson_triggered()
{
    qDebug()<<"on_doSaveJson_triggered";
    on_do_fill_path_triggered();
    doSaveJson();
}

//保存Json文件(不填充轨迹)
void MainWindow::on_doSaveJson_unfilled_triggered()
{
    doSaveJson();
}

//清除当前轨迹
void MainWindow::on_doCleanJson_triggered()
{
    if (m_path && m_path->scene() == pgmScene)
    {
        pgmScene->removeItem(m_path);
        qDebug()<<"m_path removed";
    }
    m_path->clear();
}

// 关闭图片
void MainWindow::on_doClose_triggered()
{
    if(flag){
        if(QMessageBox::Yes==QMessageBox::question(this,tr("尚未保存"),tr("是否返回操作再关闭？"),QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes)){
            return;
        }
    }
    //直接清理图片W
    cleanImage();
    flag = false;
}

//保存图片
void MainWindow::on_doSave_triggered()
{
    //暂时先使用doSave_As
    on_doSave_As_triggered();
//    //首先将图片与本地图片进行替换
//    pgmPixmapItem->pixmap().save(ConstImage);

//    //更新底部statusBar
//    if (size)
//    {
//        delete size;
//        size = new QLabel;
//        ui->statusbar->addPermanentWidget(size);
//    }
//    if(pixel){
//        delete pixel;
//        pixel = new QLabel;
//        ui->statusbar->addPermanentWidget(pixel);
//    }

//    //重新获取图片信息
//    delete info;
//    info = new QFileInfo(ConstImage);

//    //更新底部statusBar
//    double size_h=info->size()*1.0/1024;
//    QString size_type=" KB";
//    if(size_h>=1024){
//        size_h/=1024;
//        size_type=" MB";
//    }
//    size->setText(QString::number(size_h, 'f', 2)+size_type);
//    pixel->setText(QString::number(pgmPixmapItem->pixmap().width())
//                   + " x " + QString::number(pgmPixmapItem->pixmap().height()));

//    //调整图片适应窗口
//    on_do_adjust_triggered();
//    flag = false;
}

//另存为//这个不对，还得 改改//已改逻辑为导出图片
void MainWindow::on_doSave_As_triggered()
{
    //直接用原始图片作为地址和名字
    QString newPath = QFileDialog::getSaveFileName(this, tr("Save image"), ConstImage,
                                                   tr("Image PGM (*.pgm);;"
                                                      "Image BPM (*.bpm);;"
                                                      "Image JPG (*.jpg);;"
                                                      "Image JPEG (*.jpeg);;"
                                                      "Image PNG (*.png);;"
                                                      "Image SVG (*.svg);;"
                                                      "Image ICO (*.ico);;"
                                                      "Image TIF (*.tif);;"
                                                      "Image PCX (*.pcx);;"
                                                      "Image TGA (*.tga);;"
                                                      "All files (*);;"));
    ConstImage = newPath;
    //检测是否能够保存 逻辑同打开
    if (!newPath.isEmpty()) {
        QFile file(newPath);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, tr(WINDOW_ERROR), tr("不能保存这个文件！"));
            return;
        }
        pgmPixmapItem->pixmap().save(newPath);
        flag = false;
        file.close();
    }

    //更新底部statusBar
    if (size)
    {
        delete size;
        size = new QLabel;
        ui->statusbar->addPermanentWidget(size);
    }
    if(pixel){
        delete pixel;
        pixel = new QLabel;
        ui->statusbar->addPermanentWidget(pixel);
    }

    //重新获取图片信息
    delete info;
    info = new QFileInfo(ConstImage);

    //更新底部statusBar
    double size_h=info->size()*1.0/1024;
    QString size_type=" KB";
    if(size_h>=1024){
        size_h/=1024;
        size_type=" MB";
    }
    size->setText(QString::number(size_h, 'f', 2)+size_type);
    pixel->setText(QString::number(pgmPixmapItem->pixmap().width())
                   + " x " + QString::number(pgmPixmapItem->pixmap().height()));

    //调整图片适应窗口
    on_do_adjust_triggered();
    flag = false;
}

//退出
void MainWindow::on_doExit_triggered()
{
    //检查是否更改
    if(flag){
        if(QMessageBox::Yes==QMessageBox::question(this,tr("尚未保存"),tr("是否返回操作再退出？"),QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes)){
            return;
        }
    }
    cleanImage();
    qApp->quit();
}

//恢复图像
void MainWindow::on_doRestore_triggered()
{
    //获取初始图片
    QPixmap pgmPixmap =CvMat8UToQPixmap(imgSrc);
    imgTmp = imgSrc.clone();
    //更新图片
    updateImage(pgmPixmap);
    //调整图像自适应
//    on_do_adjust_triggered();
    flag = false;
}

//调整图片适应
void MainWindow::on_do_adjust_triggered()
{
    //恢复
    ui->pgmGraphicsView->resetTransform();
    // 计算场景矩形以包含所有项
    QRectF itemsRect = pgmScene->itemsBoundingRect();
    // 设置视图的场景矩形大小
    pgmScene->setSceneRect(QRectF(itemsRect.left()-1000,itemsRect.top()-1000,itemsRect.width()+2000,itemsRect.height()+2000));

    //图
    int height = itemsRect.height();
    int width = itemsRect.width();
    int max_height = ui->pgmGraphicsView->height();
    int max_width = ui->pgmGraphicsView->width();

    //计算
//    int pic_size = qMax(width,height);
//    int max_size = qMin(max_width,max_height) - 5;
//    double val = (max_size*1.0)/pic_size;
    double val = 0.95 * qMin( ( max_height - 5 ) * 1.0 / height , ( max_width - 5 ) * 1.0 / width );

    ui->pgmGraphicsView->scale(val,val);
    ui->pgmGraphicsView->centerOn(itemsRect.center());
}

//关于
void MainWindow::on_doAbout_triggered()
{
    AboutDialog *dialog = new AboutDialog();
    dialog->setAttribute( Qt::WA_QuitOnClose, false );
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

//自动修图
void  MainWindow::on_do_AutoPS_wec_triggered()
{
    cv::Mat imgMedian;
    cv::medianBlur(imgTmp, imgMedian, 3);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::erode(imgMedian, imgMedian, kernel);    //膨胀
    cv::dilate(imgMedian, imgMedian, kernel);   //腐蚀
//    cv::medianBlur(imgMedian, imgMedian, 3);

    imgTmp = imgMedian;
    //获取tmp图片
    QPixmap pgmPixmap =CvMat8UToQPixmap(imgTmp);
    //更新图片
    updateImage(pgmPixmap);
    flag = true;
}

/******************************************************************************
 *                              获得当前用户的桌面目录
 *****************************************************************************/
QString MainWindow::getUserPath()
{
    QString userPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    return userPath;
}

//========== T1.5 前视命中区图层 ==========

//把当前轨迹集合同步给前视图层并触发重算
void MainWindow::fovRefreshPaths()
{
    if (!fovLayer) return;
    QList<BPathItem*> paths = m_path_list;
    if (m_path && !paths.contains(m_path)) paths.push_back(m_path);
    fovLayer->setPathList(paths);
    if (fovLayer->enabled())
        fovLayer->recalcAll();
}

//打开点云 pcd：读取 → 高度过滤投影 → 缓存 → 图层重算
void MainWindow::on_doOpenPcd_triggered()
{
    QString dir = (ConstImage != "") ? ConstImage.left(ConstImage.lastIndexOf("/")) : getUserPath();
    QString pcdPath = QFileDialog::getOpenFileName(this, tr("打开点云 pcd"), dir,
                                                   tr("Point Cloud (*.pcd);;All Files (*)"));
    if (pcdPath.isEmpty()) return;

    //同步读文件（几秒可接受）：statusBar 提示 + processEvents 让 UI 不冻结
    ui->statusbar->showMessage("正在加载点云: " + pcdPath + " ...");
    QApplication::processEvents();

    try {
        auto pts = blz::load_pcd(pcdPath.toStdString());
        ui->statusbar->showMessage(QString("点云读取完成: %1 点，正在投影 ...").arg(pts.size()));
        QApplication::processEvents();

        fovCloud = std::make_unique<std::vector<blz::Vec3>>(std::move(pts));
        fovGrid = std::make_unique<blz::GridProjection>(
            blz::project_to_grid(*fovCloud, 0.4, 0.05, fovLayer->zmax()));

        fovLayer->setGrid(fovGrid.get());
        if (fovLayer->enabled())
            fovLayer->recalcAll();

        ui->statusbar->showMessage(QString("点云已加载: %1 点（参与投影 %2）")
                                       .arg(fovCloud->size())
                                       .arg(fovGrid->point_count), 5000);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr(WINDOW_ERROR), tr("pcd 加载失败:\n") + e.what());
        ui->statusbar->clearMessage();
    }
}

//前视快算开关（menu action checkable）
void MainWindow::on_doFovToggle_triggered()
{
    QAction* a = qobject_cast<QAction*>(sender());
    if (fovLayer && a) fovLayer->setEnabled(a->isChecked());
}

//测试按钮
void MainWindow::on_actiontest_triggered()
{
//    qDebug()<<"PI: "<<M_PI;

//    QString yamlPath = ConstImage.left(ConstImage.lastIndexOf(".")) + ".yaml";
//    YAML::Node yamlContent = YAML::LoadFile(yamlPath.toStdString());
//    if(yamlContent["resolution"])
//    {
//        double a = yamlContent["resolution"].as<double>();
//        qDebug()<<"resolution: "<<a;
//    }else{
//        qDebug()<<"resolution not found";
//    }

//    MapSelectArea *test = new MapSelectArea(pgmScene, this);
    /*
    MapSelectItem *test = new MapSelectItem(QPoint(1000,1000));
    pgmScene->addItem(test);*/

//    test *aaa = new test(QPointF(1000, 1000), 1000, 500);
//    pgmScene->addItem(aaa);

//    AttributeAddDialog *aad = new AttributeAddDialog(this);
//    connect(aad, SIGNAL(sendData(std::string)), this, SLOT(printAttribute(std::string)));
//    aad->setAttribute(Qt::WA_QuitOnClose, false);
//    aad->setAttribute(Qt::WA_DeleteOnClose);
//    aad->show();

//    //重置
//    ui->pgmGraphicsView->resetTransform();
//    updateImage(pgmPixmapItem->pixmap());
    cleanImage();
    pgmScene->setSceneRect(QRectF());
}

void MainWindow::printAttribute(std::string msg)
{
    qDebug()<<QString::fromStdString(msg);
}
//自由绘制轨迹
void MainWindow::on_do_draw_path_triggered()
{

    qDebug()<<'a';
    m_path->clear();
    qDebug()<<'b';
    pgmScene->startCreate();
//    QPointF p(1000, 1000);
//    qDebug()<<"b1";
//    QList<QPointF> m_list;
//    qDebug()<<"b2";
//    m_list.push_back(p);
//    qDebug()<<"b3";
//    pgmScene->emit updatePoint(p, m_list, false);
    qDebug()<<'c';
    setActionStatus(false);
    qDebug()<<"d:"<<m_path->scene()<<(void*)m_path;
    pgmScene->addItem(m_path);
    qDebug()<<"e:"<<m_path->scene();
    ui->statusbar->showMessage( "左键点击加入点，右键点击结束绘制" );
    qDebug()<<'f';

}

//填充全部轨迹
void MainWindow::on_do_fill_path_triggered()
{
    qDebug()<<"11："<<m_path_list.size();
    for ( auto& it : m_path_list )
    {
//        qDebug()<<"12";
        if ( it->pointsNumber() < 2 ) continue;
        for ( int i = 0 ; i < it->pointsNumber() - 1 ; i ++ )
        {
//            qDebug()<<"13";
            QPointF cur = it->getPoint( i ),
                next = it->getPoint( i + 1 );
//            qDebug()<<"14";
            if ( ( ( cur.x() - next.x() ) * ( cur.x() - next.x() ) + ( cur.y() - next.y() ) * ( cur.y() - next.y() ) ) <= 0.0625 / resolution /resolution )
                it->deletePoint(it->getElement(i+1));
            else if ( ( ( cur.x() - next.x() ) * ( cur.x() - next.x() ) + ( cur.y() - next.y() ) * ( cur.y() - next.y() ) ) >= 1.5625 / resolution /resolution )
                it->insertPointNext( it->getElement(i), 1/resolution );
//            qDebug()<<"15";
//            qDebug()<<"fill_point: "<<i;
        }
    }
//    qDebug()<<"21";
    if ( m_path->pointsNumber() < 2 ) return;
//    qDebug()<<"22";
    for ( int i = 0 ; i < m_path->pointsNumber() - 1 ; i ++ )
    {
//        qDebug()<<"23";
        QPointF cur = m_path->getPoint( i ),
            next = m_path->getPoint( i + 1 );
//        qDebug()<<"24";
        if ( ( ( cur.x() - next.x() ) * ( cur.x() - next.x() ) + ( cur.y() - next.y() ) * ( cur.y() - next.y() ) ) <= 0.0625 / resolution /resolution )
            m_path->deletePoint(m_path->getElement(i+1));
        else if ( ( ( cur.x() - next.x() ) * ( cur.x() - next.x() ) + ( cur.y() - next.y() ) * ( cur.y() - next.y() ) ) >= 1.5625 / resolution /resolution )
            m_path->insertPointNext( m_path->getElement(i), 1/resolution );
//        qDebug()<<"25";
//        qDebug()<<"fill_point: "<<i;
    }
}

//func_openYAML
void MainWindow::openYaml(QString yamlPath)
{
    qDebug()<<"250121#try to open yaml: " << yamlPath;
    std::fstream fs;
    fs.open( UTF8ToGBK(yamlPath) );
    if( !fs.is_open() )
    {
        QMessageBox::warning(this, tr(WINDOW_ERROR), tr("读取yaml失败，请检查文件路径！\n"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    if (fs.peek() == std::ifstream::traits_type::eof()) {
        QMessageBox::critical(this, tr("YAML error"), tr("文件不能为空文件"));
        return;
    }
    fs.close();
//    std::string str, yamlContent;
//    while( std::getline(fs,str) ){
//        yamlContent += str + ' ';
//    }
//    fs.close();

//    std::istringstream iss;
//    iss.str( yamlContent );

//    while ( iss >> str )
//    {
//        if ( str == "resolution:" )
//        {
//            iss >> resolution;
//        }
//        else if ( str == "origin:" )
//        {
//            std::string tmp;
//            iss >> tmp;
//            origin_x = std::stod( tmp.substr( 1, tmp.size() - 2 ) );
//            iss >> tmp;
//            origin_y = std::stod( tmp.substr( 0, tmp.size() - 1 ) );
//            iss >> tmp;
//            yaw = std::stod( tmp.substr( 0, tmp.size() - 1 ) );
//        }
//    }
//    qDebug()<<"resolution: "<<resolution<<" origin_x: "<<origin_x<<" origin_y: "<<origin_y;
    try {
        // 加载 YAML 文件
        YAML::Node node = YAML::LoadFile( UTF8ToGBK(yamlPath) );
        if (node["origin"].IsDefined()) {
            std::vector<double> origin_tmp = node["origin"].as<std::vector<double>>();
            origin_x = origin_tmp[0];
            origin_y = origin_tmp[1];
            yaw = origin_tmp[2];
        } else {
            QMessageBox::critical(this, tr("YAML error"), tr("当前文件找不到origin项"));
            return;
        }
        if (node["resolution"].IsDefined()) {
            resolution = node["resolution"].as<double>();
        } else {
            QMessageBox::critical(this, tr("YAML error"), tr("当前文件找不到resolution项"));
            return;
        }
        qDebug()<<"resolution: "<<resolution<<" origin_x: "<<origin_x<<" origin_y: "<<origin_y;
    } catch (const YAML::BadFile& e) {
        QMessageBox::critical(this, tr("YAML 打开失败"), QString("文件无法打开: %1").arg(e.what()));
    } catch (const YAML::ParserException& e) {
        QMessageBox::critical(this, tr("YAML 语法错误"), QString("解析失败: %1").arg(e.what()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("未知错误"), QString("异常信息: %1").arg(e.what()));
    }
    if ( resolution == 0 )
    {
        QMessageBox::warning( this, tr(WINDOW_ERROR), tr("请检查yaml文件数据，读取resolution与origin值失败" ) );
        return;
    }
    ui->statusbar->showMessage("yaml file is opened : " + yamlPath, 3000);

    startRect = new startRectItem(QPointF(-origin_x / resolution, pgmPixmapItem->pixmap().height() - 1 + origin_y / resolution),
                                  1.4 / resolution, 1.7 / resolution, - yaw * 180.0 / M_PI );
    pgmScene->addItem(startRect);

    //T1.5:yaml 就绪 → 同步地图坐标参数给前视图层
    if (fovLayer && pgmPixmapItem) {
        fovLayer->setMapInfo(resolution, origin_x, origin_y, pgmPixmapItem->pixmap().height());
        fovRefreshPaths();
    }
    //T2.4:同样同步给直播层（像素↔世界换算）
    if (liveLayer && pgmPixmapItem) {
        liveLayer->setMapInfo(resolution, origin_x, origin_y, pgmPixmapItem->pixmap().height());
    }
}

//打开yaml文件
void MainWindow::on_doOpenYaml_triggered()
{

    QMessageBox msgBox;
    msgBox.setText("是否加载同一路径下的同名yaml文件?");
    //TODO_ymal:管理能否操作yaml相关的Action的变量
    msgBox.setInformativeText("若不加载yaml文件，则不能进行轨迹宽度设置、轨迹保存!");
    QPushButton* yesButton = msgBox.addButton(tr("是"), QMessageBox::AcceptRole);
    QPushButton* moreButton = msgBox.addButton(tr("选择文件"), QMessageBox::ActionRole);
//        QPushButton* noButton = msgBox.addButton(tr("否"), QMessageBox::RejectRole);
    msgBox.addButton(tr("取消"), QMessageBox::RejectRole);

    msgBox.exec();

    if(msgBox.clickedButton() == yesButton)
    {
        yamlPath = ConstImage.left(ConstImage.lastIndexOf(".")) + ".yaml";
    }
    else if ( msgBox.clickedButton() == moreButton )
    {
        yamlPath = QFileDialog::getOpenFileName(this, tr("Open Yaml"), ConstImage, tr("Yaml FIles (*.yaml);;"));
    }
    else
    {
        return;
    }

    openYaml(yamlPath);
    ConstYaml = yamlPath;
}

//func_设置轨迹宽度
void MainWindow::setPathWidth(QString str)
{
    int width = str.toInt();
    if ( resolution == 0 )
    {
        QMessageBox::critical(this, tr(WINDOW_ERROR), tr("未加载yaml或未获得地图信息"));
        return;
    }
    m_path_width = width / 100.0 / resolution;

    for ( auto& it : m_path_list )
        it->setPathWidth(m_path_width);
    m_path->setPathWidth(m_path_width);
}

//func_绘制虚拟墙
/*
void MainWindow::drawVirtualWall( QList<QPointF> m_list )
{
    QPointF p_1 = m_list[0];
    QPointF p_2 = m_list[1];

    cv::line( imgTmp, cv::Point2d( p_1.x(), p_1.y() ), cv::Point2d( p_2.x(), p_2.y() ), 0, 3 );
    //获取tmp图片
    QPixmap pgmPixmap =CvMat8UToQPixmap(imgTmp);
    //更新图片
    updateImage(pgmPixmap);
    flag = true;

    setActionStatus(true);
    ui->statusbar->showMessage("");
}
*/
//231015-腾飞
void MainWindow::drawVirtualWall( QList<QPointF> m_list )
{
    //    QPointF p_1 = m_list[0];
    //    QPointF p_2 = m_list[1];

    //    cv::line( imgTmp, cv::Point2d( p_1.x(), p_1.y() ), cv::Point2d( p_2.x(), p_2.y() ), 0, 3 );
    QPointF pmin(-1,-1);
    if(m_list[m_list.size()-1]!=pmin){
        QPointF p_wei[m_list.size()];
        for(int i=0;i<m_list.size();++i){
            p_wei[i]=m_list[i];
        }
        for(int i=0;i<m_list.size()-1;++i){
            cv::line( imgTmp, cv::Point2d( p_wei[i].x(), p_wei[i].y() ), cv::Point2d( p_wei[i+1].x(), p_wei[i+1].y() ), 0, 3 );
        }

        //获取tmp图片
        QPixmap pgmPixmap =CvMat8UToQPixmap(imgTmp);
        //更新图片
        updateImage(pgmPixmap);
    }
    else{
        flag = true;
        setActionStatus(true);
        ui->statusbar->showMessage("");
    }
}

//绘制虚拟墙
void MainWindow::on_doDrawLine_triggered()
{

    pgmScene->drawLine2img();
    setActionStatus(false);
    ui->statusbar->showMessage( "左键点击加入点，右键点击结束绘制" );

}

//保存轨迹到png
void MainWindow::on_doDrawTraj_png_triggered()
{
    cv::Mat pngTraj = imgTmp;

    cv::cvtColor( pngTraj, pngTraj, cv::COLOR_GRAY2BGR );

    cv::Scalar color = cv::Scalar(255,0,0);

    for ( auto& it : m_path_list )
    {
        for ( int i = 1; i < it->pointsNumber(); i ++ )
        {
            QPointF last_p = it->getPoint( i - 1 );
            QPointF curr_p = it->getPoint( i );
            cv::circle( pngTraj, cv::Point2d( curr_p.x(), curr_p.y() ), 3, color, 4);
            cv::line( pngTraj, cv::Point2d( last_p.x(), last_p.y() ), cv::Point2d( curr_p.x(), curr_p.y() ), color, 3 );
        }
    }
    for ( int i = 1; i < m_path->pointsNumber(); i ++ )
    {
        QPointF last_p = m_path->getPoint( i - 1 );
        QPointF curr_p = m_path->getPoint( i );
        cv::circle( pngTraj, cv::Point2d( curr_p.x(), curr_p.y() ), 3, color, 4);
        cv::line( pngTraj, cv::Point2d( last_p.x(), last_p.y() ), cv::Point2d( curr_p.x(), curr_p.y() ), color, 3 );
    }

    QString pngTrajPath;
    if( ConstImage == "" )
    {
        pngTrajPath = ConstImage = getUserPath();
    }
    else
    {
        pngTrajPath = ConstImage.left(ConstImage.lastIndexOf(".")) + ".png";
    }
    pngTrajPath = QFileDialog::getSaveFileName(this, tr("Save image"), pngTrajPath,
                                                       tr("Image PNG (*.png);;"
                                                          "All files (*);;"));

    QFile file( pngTrajPath );
    //指定打开文件模式
    if ( !file.open(QIODevice::WriteOnly )) {
        QMessageBox::critical(this, tr(WINDOW_ERROR), tr("请检查路径！\n当前路径不可写"));
        return;
    }
    cv::imwrite( UTF8ToGBK(pngTrajPath), pngTraj);

    qDebug()<<"file.size()"<<file.size();
    if ( !file.exists() || file.size() == 0 )
    {
        QMessageBox::warning( this, tr(WINDOW_ERROR), tr("文件保存失败\n注意不能包含中文路径"));
    }

    file.close();
}

//绘制多边形区域
void MainWindow::on_do_draw_polygons_area_triggered()
{
    pgmScene->create_mapSelect_polygon();
    pgmScene->addItem( mapSelected );
    setActionStatus(false);
    ui->statusbar->showMessage( "左键点击绘制点，右键点击结束绘制" );
}

//填充区域
void MainWindow::on_do_draw_ploygons_triggered()
{
    if ( mapSelected->scene() == pgmScene )
        pgmScene->removeItem(mapSelected);

    std::vector<std::vector<cv::Point>> contours;
    for ( size_t i = 0; i < mapSelected->polygons.size(); i ++ )
    {
        std::vector<cv::Point> pts;
        for ( size_t j = 0; j < mapSelected->polygons[i].size(); j ++ )
        {
            pts.push_back(cv::Point((int)(mapSelected->polygons[i][j]->getPoint().x()), (int)(mapSelected->polygons[i][j]->getPoint().y())));
        }
        contours.push_back(pts);
    }
    cv::drawContours( imgTmp, contours, -1, 0, 2, 8 );
    cv::drawContours( imgTmp, contours, -1, 255, cv::FILLED, 8 );

    //获取tmp图片
    QPixmap pgmPixmap =CvMat8UToQPixmap(imgTmp);
    //更新图片
    updateImage(pgmPixmap);
    flag = true;
}

//调整多边形区域
void MainWindow::on_do_adjust_polygons_area_triggered()
{
    pgmScene->addItem(mapSelected);
}

//绘制覆盖式轨迹
void MainWindow::on_do_draw_full_covered_path_triggered()
{
    //检查yaml文件是否加载
    if ( resolution == 0 )
    {
        on_doOpenYaml_triggered();
    }
    if ( resolution == 0 )
    {
        return;
    }
    //判断是否绘制了多边形区域
    if(mapSelected->polygons.size()==0){
        QMessageBox warning;
        warning.setWindowTitle(tr("错误"));
        warning.setText(tr("不存在多边形区域！"));
        warning.exec();
        return;
    }
    //起始点
    QPointF startPoint = QPointF(-origin_x / resolution, pgmPixmapItem->pixmap().height() - 1 + origin_y / resolution);
    //输入框
    coverPathDialog = new CoverPathDialog(startPoint);
    //信号处理
    connect(coverPathDialog, &CoverPathDialog::sendData, this,
            &MainWindow::receiveCoverPathValue );
    connect(coverPathDialog, &CoverPathDialog::sendData_pre, this,
            &MainWindow::show_smooth_coverPathEdges );
    connect(coverPathDialog, &CoverPathDialog::selectStartPoint, this, &MainWindow::selectingStartPoint);
    connect(pgmScene, &BQGraphicsScene::selected_startPoint_accept, coverPathDialog, &CoverPathDialog::setStartPoint);

    //阻塞对话框
//    dialog->exec();

    //非模态对话框
    coverPathDialog->setAttribute( Qt::WA_QuitOnClose, false );
    coverPathDialog->setAttribute(Qt::WA_DeleteOnClose);
//    dialog->setWindowTitle(tr(""));
    coverPathDialog->show();
}

//slot_绘制覆盖式轨迹_选点
void MainWindow::selectingStartPoint()
{
    setActionStatus(false);
    ui->statusbar->showMessage("左键点击选点，右键点击取消");
    pgmScene->select_startPoint();
}

//func_外接圆半径的平方
double outerCircleRadius2(QPointF a, QPointF b, QPointF c)
{
    //R = ab*bc*ca/(4S)
    //R^2 = ab2*bc2*ca2/(16S2)
    //S = (1/2)*(x1y2+x2y3+x3y1-x1y3-x2y1-x3y2);
    double S16 = 4.0 * ( a.x() * b.y() + b.x() * c.y() + c.x() * a.y() - a.x() * c.y() - b.x() * a.y() - c.x() * b.y() )
                * ( a.x() * b. y() + b.x() * c.y() + c.x() * a.y() - a.x() * c.y() - b.x() * a.y() - c.x() * b.y() );
    if ( S16 == 0 ) return DBL_MAX;

    double ab,bc,ca;
    ab = DIS2(a,b);
    bc = DIS2(b,c);
    ca = DIS2(c,a);
    return ab * bc * ca / S16;
}

//func_∠abc的边向量内积
double innerProductABC(QPointF a, QPointF b, QPointF c)
{
    std::vector<double> ba,bc;
    ba.push_back( a.x() - b.x() );
    ba.push_back( a.y() - b.y() );
    bc.push_back( c.x() - b.x() );
    bc.push_back( c.y() - b.y() );

    return std::inner_product( ba.begin(), ba.end(), bc.begin(), 0 );
}

//func_从mapSelected中获得contours2d
std::vector<std::vector<QPointF>> MainWindow::mapSelectedTOcontours2d(MapSelectItem* a)
{
    std::vector<std::vector<QPointF>> contours2d;
    for ( size_t i = 0; i < a->polygons.size(); ++ i )
    {
        std::vector<QPointF> pts;
        for ( size_t j = 0; j < a->polygons[i].size(); ++ j )
        {
            pts.push_back( QPointF( a->polygons[i][j]->getPoint() ) );
        }
        contours2d.push_back(pts);
    }
    return contours2d;
}
//func_修正边缘后的多边形集
std::vector<std::vector<QPointF>> MainWindow::smooth_coverPathEdges(std::vector<std::vector<QPointF>> contours2d, int w, double mr)
{
    double minOuterRadius = ( mr + w / 100.0 / 2.0 ) / resolution;
    double minDivideDis = minOuterRadius * 15.0 / 180.0 * M_PI;

    qDebug()<< "minDivideDis: "<< minDivideDis;
    //将多边形按minDivideDis插值
    for ( size_t i = 0; i < contours2d.size(); ++ i )
    {
        for ( size_t j = 0; j < contours2d[i].size(); ++ j )
        {
            QPointF a(contours2d[i][j]), b;
            if ( j + 1 == contours2d[i].size() ) b = contours2d[i][0];
            else b = contours2d[i][j + 1];

            double dis = sqrt( DIS2(a,b) );
            int k = (int)( dis / minDivideDis );
            for ( int u = 1; u < k; ++ u )
            {
                QPointF p( ( u * b.x() + ( k - u ) * a.x() ) / k, ( u * b.y() + ( k - u ) * a.y() ) / k );
                contours2d[i].insert( contours2d[i].begin() + ( ++ j ), p );
//                qDebug()<<"insert to:"<<j<<" point: "<< p;
            }
        }
    }

    //当此次循环没有进行点的调整，才退出
    bool flag = true;

    int times = 0;
    while(flag)
    {
         ++ times;
        flag = false;
        //生成区域的path
        QPainterPath path;
        for ( size_t i = 0; i < contours2d.size(); ++ i )
        {
            QPolygonF polygon;
            for ( size_t j = 0; j < contours2d[i].size(); ++ j )
            {
                polygon << contours2d[i][j];
            }
            path.addPolygon(polygon);
        }

        for ( size_t i = 0; i < contours2d.size(); ++ i )
        {
            for ( size_t j = 0; j < contours2d[i].size(); ++ j )
            {
                QPointF a,b,c;
                if ( j == 0 ) a = contours2d[i][contours2d[i].size() - 1];
                else a = contours2d[i][j - 1];
                b = contours2d[i][j];
                if ( j == contours2d[i].size() - 1 ) c = contours2d[i][0];
                else c = contours2d[i][j + 1];

                if ( DIS2(a,c) < minDivideDis * minDivideDis )
                {
                    contours2d[i].erase( contours2d[i].begin() + ( j -- ) );    //j --
                    flag = true;
//                    qDebug()<<"dis<minDivideDis: "<<i<<","<<j;
                    continue;
                }

                QPointF t( ( a.x() + b.x() + c.x() ) / 3.0 , ( a.y() + b.y() + c.y() ) / 3.0 );
//                if ( path.contains( t ) && ( outerCircleRadius2( a, b, c ) < minOuterRadius * minOuterRadius * 0.99 || innerProductABC(a, b, c) >= 0 ) )
                if ( path.contains( t ) && outerCircleRadius2( a, b, c ) < minOuterRadius * minOuterRadius * 0.99 )
                {
//                    qDebug()<<"point: "<<a<<b<<c;
                    double theta = atan2(a.x() - c.x(), c.y() - a.y());
                    double ED = minOuterRadius - sqrt( minOuterRadius * minOuterRadius - 1.0 / 4.0 * ( DIS2(a,c) ) );
                    QPointF E( ( a.x() + c.x() ) / 2.0 , ( a.y() + c.y() ) / 2.0 );
                    QPointF D1( E.x() + ED * cos(theta), E.y() + ED * sin(theta) ),
                        D2( E.x() - ED * cos(theta), E.y() - ED * sin(theta) );

//                    qDebug()<<"theta: "<<theta;
//                    qDebug()<<"ac DIS: "<<sqrt(DIS2(a,c));
//                    qDebug()<<"minOuterRadius: "<<minOuterRadius;
//                    qDebug()<<"ED: "<<ED;
//                    qDebug()<<"E:"<<E;
//                    qDebug()<<"D: "<<D1<<D2;
                    if ( DIS2(b,D1) > DIS2(b,D2) ) D1 = D2;
//                    qDebug()<<"contours2d["<<i<<"]["<<j<<"]: "<<contours2d[i][j];
                    contours2d[i][j] = D1;
//                    qDebug()<<"changed to : "<<D1;
                    flag = true;
                }
            }
        }
    }
    qDebug()<<"times: "<<times;

    return contours2d;
}

//func_从QPainterPath中提取边缘并生成一个std::vector<std::vector<QPointF>>类型的边缘（contours）
std::vector<std::vector<QPointF>> MainWindow::extractContoursFromPainterPath(const QPainterPath& path)
{
    std::vector<std::vector<QPointF>> contours2d;

    QPainterPathStroker stroker;
    stroker.setWidth(1.0); // 设置边缘宽度

    QPainterPath strokePath = stroker.createStroke(path); // 提取边缘

    // 将边缘路径转换为轮廓
    QList<QPolygonF> polygons = strokePath.toSubpathPolygons();
    for (const QPolygonF& polygon : polygons) {
        std::vector<QPointF> contour2d;
        for (const QPointF& point : polygon) {
            contour2d.push_back(point);
        }
        contours2d.push_back(contour2d);
    }

    return contours2d;
}

//func_从QPainterPath中提取轮廓并生成一个std::vector<std::vector<QPointF>>类型的轮廓（contours）
std::vector<std::vector<QPointF>> contours2dFromPainterPath(const QPainterPath& path)
{
    std::vector<std::vector<QPointF>> contours2d;

    QList<QPolygonF> polygons = path.toFillPolygons();

    for (const QPolygonF& polygon : polygons) {
        std::vector<QPointF> contour2d;
        for (const QPointF& point : polygon) {
            contour2d.push_back(point);
        }
        contours2d.push_back(contour2d);
    }

    return contours2d;
}

//func_从contours2d得到QPainterPath
QPainterPath MainWindow::QPainterPathFromContours2d(std::vector<std::vector<QPointF>> contours2d)
{
    QPainterPath path;
    for ( size_t i = 0; i < contours2d.size(); ++ i )
    {
        QPolygonF polygon;
        for ( size_t j = 0; j < contours2d[i].size(); ++ j )
        {
            polygon << contours2d[i][j];
        }
        path.addPolygon(polygon);
    }
    return path;
}

//slot_绘制覆盖式轨迹
void MainWindow::receiveCoverPathValue(int w, double mr, int type, QPointF startP, qreal startDirection)
{
//    ui->statusbar->showMessage( "正在计算。。。" );
    //初始化graph
    int cols = imgTmp.cols, rows = imgTmp.rows;
    Graph toCover = Graph(rows, cols);
    Graph canAccess = Graph(rows, cols);

    //获得一个修整后的区域
    std::vector<std::vector<QPointF>> contours2d = smooth_coverPathEdges(mapSelectedTOcontours2d(mapSelected), w, mr);
    //预计覆盖区域的path
    QPainterPath path = QPainterPathFromContours2d(contours2d);
    qDebug()<<"path.elementCount:"<<path.elementCount();

    /*
//test->
//1-更快！！
    // 获取开始时间点
    auto start = std::chrono::high_resolution_clock::now();

    //预计覆盖区域的path
    QPainterPath tmp_path = QPainterPathFromContours2d(contours2d);
    // 创建一个 QImage 对象，所有点初始化为白色
    QImage image(cols, rows, QImage::Format_Grayscale8);
    image.fill(Qt::white);

    // 创建 QPainter 对象并将 path 渲染到 image 上
    QPainter painter(&image);
    painter.setPen(QPen(Qt::black, 1));  // 设置画笔颜色和宽度
    painter.fillPath(tmp_path, Qt::black);

    // 将 QImage 转换为 cv::Mat
    cv::Mat mat(image.height(), image.width(), CV_8UC1, (void*)image.bits(), image.bytesPerLine());

    // 获取结束时间点
    auto finish = std::chrono::high_resolution_clock::now();

    // 计算时间差
    std::chrono::duration<double, std::milli> elapsed = finish - start;
    qDebug() << "Elapsed time1: " << elapsed.count() << " ms";
//2
    // 获取开始时间点
    auto tmp_start = std::chrono::high_resolution_clock::now();

    cv::Mat imgTest = toCover.getContours2d(contours2d);
    // 获取结束时间点
    auto tmp_finish = std::chrono::high_resolution_clock::now();
    // 计算时间差
    std::chrono::duration<double, std::milli> tmp_elapsed = tmp_finish - tmp_start;
    qDebug() << "Elapsed time2: " << tmp_elapsed.count() << " ms";
    cv::namedWindow("path", cv::WINDOW_NORMAL);
    cv::imshow("path", imgTest);
    return;
//test-<
    */

    qDebug()<<"开始update";
    /*
    //这里手动update，少循环一边
    QRectF boundingRect = path.boundingRect();
    for ( int i = qMax(0, (int)(boundingRect.top())); i < qMin(rows,(int)(boundingRect.bottom())); ++ i )
    {
        qDebug()<<"row:"<<i;
        for ( int j = qMax(0, (int)(boundingRect.left())); j < qMin(cols, (int)(boundingRect.right())); ++ j )
        {
            //划定车辆可运行区域//划定车辆覆盖范围
            if ( path.contains( QPointF( j, i ) ) ) //这里注意坐标系(x,y)对应(j,i);
            {
                canAccess.graph[i][j] = true;
                toCover.graph[i][j] = true;
            }
        }
    }
    *///以下两行替代
    toCover.updateFromContours2d(contours2d);
    canAccess.updateFromContours2d(contours2d);

    //用于检查graph的形状
//    toCover.show();
//    return;
    //    //->以下是准备工作
    //    //获得起点信息
    //    QPointF(-origin_x / resolution, pgmPixmapItem->pixmap().height() - 1 + origin_y / resolution);
    //    //绘制json点
    //    m_path->addNextPoint( QPointF( pixel_x, pixel_y ) );
    //    //-<以上是准备工作

    QPointF startPoint = startP;
    double angle = 0.0;
    double w_ = w / 100.0 / resolution;
    double mr_ = mr / resolution;
    double step = w_;

    PathSimulate car(startPoint, angle, w_, mr_);

    qDebug()<<"sin(45):"<<sin(M_PI * 45.0 / 180.0);
    qDebug()<<"sin(90):"<<sin(M_PI * 90.0 / 180.0 );

    //起始点
    m_path->clear();
    m_path->addNextPoint( car.curPoint );
//    car.fd(20);
    qDebug()<<"test4";
    pgmScene->addItem(m_path);
    qDebug()<<"test5";

    //以下，进行轨迹绘制
    bool isNearEdge = false;
    bool isNotEnd = true;
    int n = 10000;
    int k = 0;
    int m_path_fixed = 1;//为什么m_path_fixed从1开始，因为第0个点未贴边，不要
    bool isFirstRound = true;
    bool setStartPoint = false;
    int roundTimes = 1;
    while(n-- && isNotEnd)
    {
        k ++;
        //判断是否返回起点
        if ( k > 4  && DIS2( car.curPoint, startPoint ) <= step * step )
        {
            qDebug()<<"圈数："<<roundTimes++;
            qDebug()<<"已循环至起点，更新地图1";
            QPolygonF polygon;
            //绘制m_path附近的widthPX2(w_/2)宽度的多边形，再将其从toCover.graph中去除
            //先绘制一边
            for ( int i = m_path_fixed; i < m_path->pointsNumber() - 1; ++ i )
            {
                const QPointF& p1 = m_path->getPoint( i );
                const QPointF& p2 = m_path->getPoint( i + 1 );

                QLineF line(p1, p2);
                QPointF normal = line.normalVector().unitVector().p2() - p1;
                QPointF offset = normal * w_ / 2;
                QPointF p1a = p1 + offset;
                QPointF p2a = p2 + offset;

                polygon << p1a << p2a;
//                qDebug()<<"p12a:"<<normal<<offset<<p1a<<p2a;
            }
            qDebug()<<"已循环至起点，更新地图2";
            //再绘制一边
            for ( int i = m_path->pointsNumber() - 1; i > m_path_fixed; -- i )
            {
                const QPointF& p1 = m_path->getPoint( i );
                const QPointF& p2 = m_path->getPoint( i - 1 );

                QLineF line(p1, p2);
                QPointF normal = line.normalVector().unitVector().p2() - p1;
                QPointF offset = normal * w_ / 2;
                QPointF p1b = p1 + offset;
                QPointF p2b = p2 + offset;

                polygon << p1b << p2b;
//                qDebug()<<"p12b:"<<normal<<offset<<p1b<<p2b;
            }
            m_path_fixed = m_path->pointsNumber() - 1;
            qDebug()<<"已循环至起点，更新地图3";
            //graph更新
            QPainterPath pathPolygon;
            pathPolygon.addPolygon(polygon);
            path = path.subtracted(pathPolygon);

            //这里简化一下graph
            qDebug()<<"contours2dFromPainterPath:size:"<<contours2dFromPainterPath(path).size();
            qDebug()<<"卡住测试2";
            toCover.updateFromContours2d(contours2dFromPainterPath(path));
//            toCover.openOperation(w_/2);    //这里已经toGraph(更新graph)了
//            path = toCover.pointsToPath();
//            qDebug()<<"卡住测试4"<<path.isEmpty();
//            qDebug()<<"contours2dFromPainterPath:size:"<<contours2dFromPainterPath(path).size();
//            return;
            QPainterPath tmp_painterPath = path;
            graphShow* pathShow = new graphShow(&tmp_painterPath);
            pgmScene->addItem(pathShow);
//            return;
//            std::vector<std::vector<QPointF>> contours2d_tmp = smooth_coverPathEdges(contours2dFromPainterPath(path), w, mr);
//            path = QPainterPathFromContours2d(contours2d_tmp);

            std::vector<std::vector<QPointF>> contours2d_tmp = smooth_coverPathEdges(toCover.openOperationTOcontours2d(w_/2), w, mr);
            path = QPainterPathFromContours2d(contours2d_tmp);
            qDebug()<<"卡住测试3";
            toCover.updateFromContours2d(contours2dFromPainterPath(path));
            qDebug()<<"已循环至起点，更新地图4,contours2d_tmp.size:"<<contours2d_tmp.size()<<"contours2d_tmp[0].size:"<<contours2d_tmp[0].size();
            //更新car位姿   //角度不改变了，提高鲁棒性
//            car.curPoint = QPointF( startPoint.x() + w_ * sin(angle), startPoint.y() - w_ * cos(angle) );
            car.curPoint = QPointF( startPoint.x() + w_ * sin(car.angle), startPoint.y() - w_ * cos(car.angle) );
//            car.angle = angle;
            setStartPoint = true;  //用于下一次判断
            k = 0;  //k恢复
//            m_path->addNextPoint( car.curPoint );   //测试用，请注释
            qDebug()<<"已循环至起点，更新地图5";
//            toCover.show(); //测试用，请注释
            qDebug()<<"卡住测试5";
//            break;  //测试用，请注释
            if(contours2d_tmp.size() == 1 && contours2d_tmp[0].size() == 0)
            {
                QMessageBox::information(coverPathDialog, tr("覆盖结束"), tr("覆盖完成"));
                break;
            }
        }

        //让car贴边
        int t = 4;  //防止car不贴边,进行多角度扫描
        int chances = 1;    //角度扫描控制
        while(!isNearEdge && t--)
        {
            qDebug()<<"t:"<<t;
            if( isFirstRound && ++t )
//                car.angle = angle + M_PI * 45.0 / 180.0;
                car.angle = M_PI * startDirection / 180.0;
            else if( t == 3 )
                car.angle = car.angle + M_PI * 90.0 / 180.0;
            car.fd(w_/2);

            //判断car isNear
            auto IsNear = [](PathSimulate car, Graph toCover) -> bool
            {
                std::vector<QPoint> carRound = car.round();
                /*
                qDebug()<<"carRound.size():"<<carRound.size();//利用test查看一下carRound的情况
                carRoundCircle* test = new carRoundCircle(car.curPoint,carRound);//利用test查看一下carRound的情况
                qDebug()<<"add test1";
                pgmScene->addItem(test);//利用test查看一下carRound的情况
                qDebug()<<"add test2";
                return true;
                */
                for ( auto& it : carRound )
                {
                    if ( !toCover.graph[it.y()][it.x()] )   //这里同上坐标系(x,y)对应(j,i);
                    {
                        qDebug()<<"Is Near!, the point is:"<< it.x()<<it.y();
                        return true;
                        break;
                    }
                }
                return false;
            };
            /*
            IsNear();//利用test查看一下carRound的情况
            qDebug()<<"test3";
            break;//利用test查看一下carRound的情况
            */

            if ( IsNear(car, toCover) )
            {
                t = 4;//贴边，恢复t值
                qDebug()<<"卡住测试1";
                car.bk(w_/2);
                double left = -5*w_, right = w_/2;
                for ( ; left < w_/2 && car.fd(left) && IsNear(car, toCover); car.bk(left), left += 0.1 * w_ ); //让left值是非贴边的
                car.bk(left);   //for循环中最后一步没有进行car.bk(left);
                while ( left + 0.005 * w_ < right )
                {
                    double mid = left + ( right - left ) / 2;
                    car.fd(mid);
                    if ( IsNear(car, toCover) )
                        right = mid;
                    else
                        left = mid;

                    car.bk(mid);
                }
                if( left >= w_/2 && chances >= 0 )    //left>=10表明没有能贴边的位置
                {
                    if( chances == 1 )
                        car.angle -= M_PI * 45.0 * chances / 180.0;
                    else if( chances == 0 )
                        car.angle += M_PI * 90.0 * chances / 180.0;
                    chances --;
                    qDebug()<<"卡住测试7:angle:"<<car.angle/M_PI*180;
                }
                else if ( left >= w_/2 && chances < 0 )   //chances<0表明三个角度都试过了，没成功
                {
                    isNotEnd = false;
                    qDebug()<<"卡住测试6";
                    break;
                }
                else    //left<10至少是贴边了
                {
                    chances = 1;

                    car.fd(right);  //为什么是right呢，因为需要贴边//或者，贴边时用的round参数大一些//先看效果
                    isNearEdge = true;
                    qDebug()<<"1:fd"<<right<<"left="<<left;
/*利用test查看一下carRound的情况
                    std::vector<QPoint> carRound = car.round();
                    qDebug()<<"carRound.size():"<<carRound.size();//利用test查看一下carRound的情况
                    carRoundCircle* test = new carRoundCircle(car.curPoint,carRound);//利用test查看一下carRound的情况
                    pgmScene->addItem(test);//利用test查看一下carRound的情况
                    qDebug()<<"卡住测试8";
*/

                }

            }

        }
        qDebug()<<"卡住测试9";
        /*
        break;//利用test查看一下carRound的情况
        */
        //贴边失败，结束覆盖
        if( t <= 0 )
        {
            qDebug()<<"t:"<<t;
            QMessageBox::information(coverPathDialog, tr("覆盖结束"), tr("未能贴边,当前位置太空旷"));
            break;
        }
        if(!isNotEnd)
        {
            qDebug()<<"卡住测试10";
            QMessageBox::information(coverPathDialog, tr("覆盖结束"), tr("贴边失败，当前位置太狭窄"));
            qDebug()<<"卡住测试11";
            break;
        }
        if (isFirstRound)
        {
            startPoint = car.curPoint;
            qDebug()<<"setFirstStartPoint:"<<startPoint;
            isFirstRound = false;
        }
        if ( setStartPoint )
        {
            startPoint = car.curPoint;
            qDebug()<<"setStartPoint:"<<startPoint;
            setStartPoint = false;
        }
        //isNearEdge之后，car就贴边了，开始螺旋式覆盖
        m_path->addNextPoint( car.curPoint );
        //判断边缘的方向，给出angle
        auto getRightEdgeAngle = [=]() -> double
        {
            std::vector<QPoint> carRound = car.round();
            double x_sum = 0, y_sum = 0;
            int num = 0;
            for ( auto& it : carRound )
            {
                if ( !toCover.graph[it.y()][it.x()] )   //这里同上坐标系(x,y)对应(j,i);
                {
                    ++ num;
                    x_sum += it.x();
                    y_sum += it.y();
                }
            }
            qDebug()<<"贴边点num:"<<num;
            if(num){
//                qDebug()<<"num:"<<num;
                x_sum /= num;
                y_sum /= num;
                return atan2( y_sum - car.curPoint.y(), x_sum - car.curPoint.x() ) - M_PI * 90.0 / 180.0;
            }
            else
            {
                return car.angle;   //这里怎么写待定//因为之前进行了fd(right)所以应该是贴边的
            }
        };
        car.angle = getRightEdgeAngle();
        qDebug()<<"angle:"<<car.angle;
        car.fd(step);
        isNearEdge = false;
//        m_path->addNextPoint( car.curPoint );
//        break;  //angle 测试
    }
    if ( QMessageBox::Yes == QMessageBox::information(this, tr(""),tr("是否填充全部轨迹？"),QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes))
    {
        on_do_fill_path_triggered();
    }
    return;
}

//slot_预计覆盖区域
void MainWindow::show_smooth_coverPathEdges(int w, double mr)
{
    emit cleanLastSCP();
    disconnect(this, &MainWindow::cleanLastSCP, this, nullptr);
    ui->statusbar->showMessage( "正在计算。。。" );
    std::vector<std::vector<QPointF>> contours2d = smooth_coverPathEdges(mapSelectedTOcontours2d(mapSelected), w, mr);
    smoothedCoverPath * SCP = new smoothedCoverPath(contours2d);
    pgmScene->addItem(SCP);
    ui->statusbar->showMessage( "已显示预计覆盖区域", 1500 );
    connect(this, &MainWindow::cleanLastSCP, this, [=](){ pgmScene->removeItem(SCP); delete SCP; });
}

//保存多边形区域
void MainWindow::on_do_save_polygons_area_triggered()
{
    QString csvPath;
    if (ConstImage != "")
    {
        csvPath = ConstImage.left(ConstImage.lastIndexOf("/"));
    }
    else if (ConstJson != "")
    {
        csvPath = ConstJson.left(ConstJson.lastIndexOf("/"));
    }
    else
    {
        csvPath = getUserPath();
    }
    csvPath = QFileDialog::getSaveFileName(this, tr("Save csv"), csvPath,
                                             tr("file CSV (*.csv);;"
                                                "All files (*);;"));
    std::ofstream outFile;
    outFile.open( UTF8ToGBK(csvPath), std::ios::out );

    for ( size_t i = 0; i < mapSelected->polygons.size(); ++ i )
    {
        outFile << "#polygon " << i << std::endl;
        for ( size_t j = 0; j < mapSelected->polygons[i].size(); ++ j )
        {
            outFile << std::to_string( mapSelected->polygons[i][j]->getPoint().x() ) << ','
                    << std::to_string( mapSelected->polygons[i][j]->getPoint().y() ) << std::endl;
        }
        outFile << "#end" << std::endl;
    }

    outFile.close();
}

//加载多边形区域
void MainWindow::on_do_load_polygons_area_triggered()
{
    QString csvPath;
    if (ConstImage != "")
    {
        csvPath = ConstImage.left(ConstImage.lastIndexOf("/"));
    }
    else if (ConstJson != "")
    {
        csvPath = ConstJson.left(ConstJson.lastIndexOf("/"));
    }
    else
    {
        csvPath = getUserPath();
    }
    csvPath = QFileDialog::getOpenFileName(this, tr("Open csv"), csvPath, tr("CSV FIles (*.csv);;"));

    std::ifstream inFile( UTF8ToGBK(csvPath), std::ios::in );

    if ( !inFile.is_open() )
    {
        QMessageBox::warning( this, tr(WINDOW_ERROR), tr("打开文件失败，请检查路径") );
        return;
    }
    else
    {
        initMapSelect();
        pgmScene->addItem( mapSelected );

        std::string line;
        while ( std::getline( inFile, line ) )
        {
            if ( line[1] == 'p' ) continue;
            else if ( line[1] == 'e' ) mapSelected->cur_ploygon_addPoint( QPointF( 0, 0 ), true );
            else
            {
                qreal x,y;
                x = std::stod( line.substr( 0, line.find(',') ) );
                y = std::stod( line.substr( line.find(',') + 1 ) );
                mapSelected->cur_ploygon_addPoint( QPointF( x, y ), false );
                qDebug()<<"add Point: "<<x<<','<<y;
            }
        }
    }
    inFile.close();
}

//清除多边形
void MainWindow::on_do_clean_polygons_area_triggered()
{
    initMapSelect();
}

//多边形橡皮擦
void MainWindow::on_doDrawWhitePolygon_triggered()
{
    if ( erase )
    {
        disconnect(pgmScene, SIGNAL(erase_ploygon_addPoint(QPointF)), erase, SLOT(pushPoint(QPointF)));
        delete erase;
    }

    erase = new erasePolygon;
    connect(pgmScene, SIGNAL(erase_ploygon_addPoint(QPointF)), erase, SLOT(pushPoint(QPointF)));

    pgmScene->drawWhitePolygon();
    pgmScene->addItem(erase);
    ui->statusbar->showMessage( "左键点击加入点，右键点击结束绘制", 5000 );
}

//func_多边形橡皮擦
void MainWindow::drawWhitePolygon(QList<QPointF> m_list)
{
    if ( erase && erase->scene() == pgmScene )
        pgmScene->removeItem(erase);

    std::vector<cv::Point> pts;
    for ( int i = 0; i < m_list.size(); ++ i )
    {
        pts.push_back( cv::Point( m_list[i].x(), m_list[i].y() ) );
    }
    cv::fillPoly( imgTmp, pts, 255, 8, 0 );

    //获取tmp图片
    QPixmap pgmPixmap =CvMat8UToQPixmap(imgTmp);
    //更新图片
    updateImage(pgmPixmap);
    flag = true;
}

//追加轨迹
void MainWindow::on_doDrawPathContinue_triggered()
{
    pgmScene->startCreate();
    setActionStatus(false);
    pgmScene->addItem(m_path);
    ui->statusbar->showMessage( "左键点击加入点，右键点击结束绘制" );
}

//func_计算两个点的叉积
double MainWindow::crossProduct(const QPointF& p1, const QPointF& p2) {
    return -(p1.x() * p2.y() - p1.y() * p2.x());    //因为y方向向下，所以顺逆时针相反
}

//func_判断一系列点是否按顺时针方向组成闭合图形
bool MainWindow::isClockwise() {
    int numPoints = m_path->pointsNumber();
    double sum = 0.0;

    for (int i = 0; i < numPoints; ++i) {
        const QPointF& current = m_path->getPoint(i);
        const QPointF& next = m_path->getPoint((i + 1) % numPoints);

        sum += crossProduct(current, next);
    }

    return sum < 0.0;
}

//func_找到给定点 startPoint 到区域 contours2d 内最近的点
QPointF MainWindow::findNearestPoint(const std::vector<std::vector<QPointF>>& contours2d, const QPointF& startPoint) {
    double minDistance = std::numeric_limits<double>::max();
    QPointF nearestPoint = startPoint;  //这里赋一个初值，不至于说最后没结果

    for (const auto& contour : contours2d) {
        for (const auto& point : contour) {
            double d = DIS2(point, startPoint);
            if (d < minDistance) {
                minDistance = d;
                nearestPoint = point;
            }
        }
    }

    return nearestPoint;
}
//slot_绘制rtk覆盖式轨迹
void MainWindow::receiveCoverPath_rtk_Value(int w, double mr, int pathType, int directionType, int mapAccuracy)
{
    qreal minx=std::numeric_limits<double>::max(),miny=std::numeric_limits<double>::max(),
        maxx=std::numeric_limits<double>::lowest(),maxy=std::numeric_limits<double>::lowest();
    for ( int i = 0; i < m_path->pointsNumber(); ++ i )
    {
        qreal x = m_path->getPoint(i).x(),
            y = m_path->getPoint(i).y();
        if ( x < minx ) minx = x;
        if ( x > maxx ) maxx = x;
        if ( y < miny ) miny = y;
        if ( y > maxy ) maxy = y;
    }
    qreal dvax = minx - 50,
        dvay = miny - 50;
    m_path->move(QPointF(-dvax,-dvay)); //将m_path移至（0，0）

    qreal magnification = mapAccuracy * resolution * 100 / w;
    int cols = (int)(maxx - minx + 100) * magnification,
        rows = (int)(maxy - miny + 100) * magnification;
    resolution /= magnification;
    m_path->trans(magnification); //将m_path放大magnification倍
    qDebug()<<"magnification:"<<dvax<<dvay<<cols<<rows<<magnification<<resolution;

    Graph toCover = Graph(rows, cols);

    //信息
    QPointF startPoint = m_path->getPoint( m_path->pointsNumber() - 1 );
    double angle;
    double w_ = w / 100.0 / resolution;
    double mr_ = mr / resolution;

    std::vector<std::vector<QPointF>> contours2d;
    //首先判断这个边界是顺时针还是逆时针
    if (isClockwise()) {
        std::vector<QPointF> contour2d;
        int numPoints = m_path->pointsNumber();
        for ( int i = 0; i < numPoints; ++ i )
        {
            const QPointF& p1 = m_path->getPoint(i);
            const QPointF& p2 = m_path->getPoint((i + 1) % numPoints);
            QLineF line(p1, p2);
            QPointF normal = line.normalVector().unitVector().p2() - p1;
            QPointF offset = normal * w_ / 2;
            QPointF p1a = p1 - offset;  //右侧
            contour2d.push_back(p1a);
        }
        contours2d.push_back(contour2d);
    } else {
        std::vector<QPointF> contour2d;
        int numPoints = m_path->pointsNumber();
        for ( int i = 0; i < numPoints; ++ i )
        {
            const QPointF& p1 = m_path->getPoint(i);
            const QPointF& p2 = m_path->getPoint((i + 1) % numPoints);
            QLineF line(p1, p2);
            QPointF normal = line.normalVector().unitVector().p2() - p1;
            QPointF offset = normal * w_ / 2;
            QPointF p1a = p1 + offset;  //左侧
            contour2d.push_back(p1a);
        }
        contours2d.push_back(contour2d);
    }
    contours2d = smooth_coverPathEdges(contours2d, w, mr);
//    qDebug()<<"闪退测试1"<<contours2d.size()<<contours2d[0].size();
//    smoothedCoverPath * SCP = new smoothedCoverPath(contours2d);
//    pgmScene->addItem(SCP);
    toCover.updateFromContours2d(contours2d);
    QPainterPath ppath = QPainterPathFromContours2d(contours2d);
//    qDebug()<<"闪退测试2"<<ppath.isEmpty();

    QPointF tmp_start = findNearestPoint(contours2d,startPoint);
    angle = atan2( tmp_start.y() - startPoint.y(), tmp_start.x() - startPoint.x() );    //atan2(0,0) == 0;

    //调整car的位置
    PathSimulate car(tmp_start, angle, w_, mr_);
    car.fd(w_/2);
    car.angle += M_PI * 90 / 180;
//    car.fd(w_);

    //以下，进行轨迹绘制
    bool isNearEdge = false;
    bool isNotEnd = true;
    int n = 10000;
    int k = 0;
    int m_path_fixed = m_path->pointsNumber();
    bool isFirstRound = true;
    bool setStartPoint = false;
    int roundTimes = 1;
    while(n-- && isNotEnd)
    {
        qDebug()<<"闪退测试3 angle:"<<car.angle;
        k ++;
        //判断是否返回起点
        if ( k > 4  && DIS2( car.curPoint, startPoint ) <= 1/resolution * 1/resolution )
        {
            qDebug()<<"圈数："<<roundTimes++;
            QPolygonF polygon;
            for ( int i = m_path_fixed; i < m_path->pointsNumber() - 1; ++ i )
            {
                const QPointF& p1 = m_path->getPoint( i );
                const QPointF& p2 = m_path->getPoint( i + 1 );
//                QLineF line(p1, p2);
//                QPointF normal = line.normalVector().unitVector().p2() - p1;
                QPointF diff = p2 - p1;
                QPointF normal(-diff.y(), diff.x());
                QPointF offset = normal / sqrt(DIS2(p1, p2)) * w_ / 2;
                QPointF p1a = p1 + offset;
                QPointF p2a = p2 + offset;
                polygon << p1a << p2a;
            }
            for ( int i = m_path->pointsNumber() - 1; i > m_path_fixed; -- i )
            {
                const QPointF& p1 = m_path->getPoint( i );
                const QPointF& p2 = m_path->getPoint( i - 1 );
                QPointF diff = p2 - p1;
                QPointF normal(-diff.y(), diff.x());
                QPointF offset = normal / sqrt(DIS2(p1, p2)) * w_ / 2;
                QPointF p1b = p1 + offset;
                QPointF p2b = p2 + offset;
                polygon << p1b << p2b;
            }
            m_path_fixed = m_path->pointsNumber();
            //graph更新
            QPainterPath pathPolygon;
            pathPolygon.addPolygon(polygon);
//            BPolygonItem* BPI = new BPolygonItem(polygon);
//            pgmScene->addItem(BPI);
            ppath = ppath.subtracted(pathPolygon);
            toCover.updateFromContours2d(contours2dFromPainterPath(ppath));
            std::vector<std::vector<QPointF>> contours2d_tmp = smooth_coverPathEdges(toCover.openOperationTOcontours2d(w_), w, mr);
            ppath = QPainterPathFromContours2d(contours2d_tmp);
            toCover.updateFromContours2d(contours2dFromPainterPath(ppath));
            car.curPoint = QPointF( startPoint.x() + w_ * sin(car.angle), startPoint.y() - w_ * cos(car.angle) );
            setStartPoint = true;  //用于下一次判断
            k = 0;  //k恢复
//            smoothedCoverPath * SCP = new smoothedCoverPath(contours2d_tmp);
//            pgmScene->addItem(SCP);
            if(contours2d_tmp.size() == 1 && contours2d_tmp[0].size() == 0)
            {
                QMessageBox::information(coverPathDialog, tr("覆盖结束"), tr("覆盖完成"));
                break;
            }
        }

//        qDebug()<<"闪退测试4";
        //让car贴边
        int t = 4;  //防止car不贴边,进行多角度扫描
        int chances = 1;    //角度扫描控制
        while(!isNearEdge && t--)
        {
//            qDebug()<<"闪退测试5";
            car.angle = car.angle + M_PI * 90.0 / 180.0;//这个是配合下面的那个chances的
            qDebug()<<"卡住测试7.1:angle:"<<car.angle/M_PI*180;
            car.fd(w_/2); //让car垂直90度前进以贴边

            auto IsNear = [](PathSimulate car, Graph toCover) -> bool
            {
//                qDebug()<<"闪退测试5.2"<<car.curPoint;
                std::vector<QPoint> carRound = car.round();
                for ( auto& it : carRound )
                {
//                    qDebug()<<"闪退测试5.3"<<it.y()<<it.x();
//                    qDebug()<<toCover.graph[it.y()][it.x()];
                    if ( !toCover.graph[it.y()][it.x()] )   //这里同上坐标系(x,y)对应(j,i);
                    {
//                        qDebug()<<"闪退测试5.4";
                        qDebug()<<"Is Near!, the point is:"<< it.x()<<it.y();
                        return true;
                        break;
                    }
                }
                return false;
            };

            if ( !IsNear(car, toCover) )
            {
                car.angle -= M_PI * 90.0 / 180.0;
//                qDebug()<<"卡住测试7.2:angle:"<<car.angle/M_PI*180;
//                qDebug()<<"闪退测试6";
//                t++;
            }
            if ( IsNear(car, toCover) )
            {
//                qDebug()<<"闪退测试5.1";
                t = 4;//贴边，恢复t值
                qDebug()<<"卡住测试1";
                car.bk(w_/2);
                double left = -w_, right = w_/2;
                for ( ; left < w_/2 && car.fd(left) && IsNear(car, toCover); car.bk(left), left += 0.1 * w_ ); //让left值是非贴边的
                car.bk(left);   //for循环中最后一步没有进行car.bk(left);
                while ( left + 0.005 * w_ < right )
                {
                    double mid = left + ( right - left ) / 2;
                    car.fd(mid);
                    if ( IsNear(car, toCover) )
                        right = mid;
                    else
                        left = mid;

                    car.bk(mid);
                }
                if( left >= w_/2 && chances >= 0 )    //left>=w_/2表明没有能贴边的位置
                {
                    car.angle -= M_PI * 135.0 * chances / 180.0;
                    chances --;
//                    qDebug()<<"卡住测试7:angle:"<<car.angle/M_PI*180;
                }
                else if ( left >= w_/2 && chances < 0 )   //chances<0表明三个角度都试过了，没成功
                {
                    isNotEnd = false;
                    qDebug()<<"卡住测试6";
                    break;
                }
                else    //left<w_/2至少是贴边了
                {
                    chances = 1;

                    car.fd(right);  //为什么是right呢，因为需要贴边//或者，贴边时用的round参数大一些//先看效果
                    isNearEdge = true;
                    qDebug()<<"1:fd"<<right<<"left="<<left;
                }
            }
        }
//        qDebug()<<"闪退测试7";
        //贴边失败，结束覆盖
        if( t <= 0 )
        {
            qDebug()<<"t:"<<t;
            QMessageBox::information(coverPathDialog, tr("覆盖结束"), tr("未能贴边,当前位置太空旷"));
            break;
        }
        if(!isNotEnd)
        {
            qDebug()<<"卡住测试10";
            QMessageBox::information(coverPathDialog, tr("覆盖结束"), tr("贴边失败，当前位置太狭窄"));
            qDebug()<<"卡住测试11";
            break;
        }
        if (isFirstRound)
        {
            startPoint = car.curPoint;
            qDebug()<<"setFirstStartPoint:"<<startPoint;
            isFirstRound = false;
        }
        if ( setStartPoint )
        {
            startPoint = car.curPoint;
            qDebug()<<"setStartPoint:"<<startPoint;
            setStartPoint = false;
        }
        //isNearEdge之后，car就贴边了，开始螺旋式覆盖
        m_path->addNextPoint( car.curPoint );

        //判断边缘的方向，给出angle
        auto getRightEdgeAngle = [=]() -> double
        {
            std::vector<QPoint> carRound = car.round();
            double x_sum = 0, y_sum = 0;
            int num = 0;
            for ( auto& it : carRound )
            {
                if ( !toCover.graph[it.y()][it.x()] )   //这里同上坐标系(x,y)对应(j,i);
                {
                    ++ num;
                    x_sum += it.x();
                    y_sum += it.y();
                }
            }
            qDebug()<<"贴边点num:"<<num;
                if(num){
                //                qDebug()<<"num:"<<num;
                x_sum /= num;
                y_sum /= num;
                return atan2( y_sum - car.curPoint.y(), x_sum - car.curPoint.x() ) - M_PI * 90.0 / 180.0;
            }
            else
            {
                return car.angle;   //这里怎么写待定//因为之前进行了fd(right)所以应该是贴边的
            }
        };
        car.angle = getRightEdgeAngle();
        qDebug()<<"angle:"<<car.angle;
        car.fd(1/resolution);
        isNearEdge = false;
    }

    resolution *= magnification;
    m_path->trans(1/magnification);
    m_path->move(QPointF(dvax,dvay));   //将m_path移到偏移位置
    on_do_adjust_triggered();

    if ( QMessageBox::Yes == QMessageBox::information(this, tr("rtk-覆盖"),tr("是否填充全部轨迹？"),QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes))
    {
        on_do_fill_path_triggered();
    }
    on_do_adjust_triggered();
    return;
}

//rtk覆盖式轨迹
void MainWindow::on_do_rtk_full_cover_path_triggered()
{
    //检查resolution属性是否提取
    if ( resolution == 0 )
    {
        QMessageBox::critical(this,tr("rtk"),tr("请检查是否加载了json文件"));
        return;
    }
    //判断是否加载了轨迹
    if ( m_path->pointsNumber() < 3 )
    {
        QMessageBox::warning( this, tr(WINDOW_ERROR), tr("轨迹点太少或未加载轨迹") );
        return;
    }
    //输入框
    CoverPath_Rtk_Dialog* dialog = new CoverPath_Rtk_Dialog();

    connect(dialog, &CoverPath_Rtk_Dialog::sendData, this, &MainWindow::receiveCoverPath_rtk_Value);

    dialog->setAttribute( Qt::WA_QuitOnClose, false );
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    //    dialog->setWindowTitle(tr(""));
    dialog->show();

}

//func_打开一张1000*1000的空白地图
void MainWindow::openBlankPixmap()  //注释类似on_doOpen_triggered()一律省略
{
    if(flag){
        if(QMessageBox::Yes==QMessageBox::question(this,tr("尚未保存"),tr("是否返回操作？"),QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes)){
            return;
        }
    }
    cleanImage();
    QPixmap blankPixmap(1000, 1000);
    blankPixmap.fill(Qt::white);
    imgSrc = QPixmapToCvMat8U(blankPixmap);
    imgTmp = imgSrc.clone();
    pgmPixmapItem = pgmScene->addPixmap(blankPixmap);
    pgmScene->setSceneRect(QRectF(blankPixmap.rect().left()-1000,blankPixmap.rect().top()-1000,3000,3000));
    on_do_adjust_triggered();
    this->setWindowTitle("rtk地图 - Map Tool");
    setActionStatus(true);
    size->setText(QString::number(976.5625, 'f', 2) + "KB");
    pixel->setText(QString::number(pgmPixmapItem->pixmap().width())
                   + " x " + QString::number(pgmPixmapItem->pixmap().height()));
    flag = false;
    isBlankMap = true;
    //rtk地图没有yaml文件
}

//仅打开Json文件
void MainWindow::on_doOpenJsonExclusively_triggered()   //注释类似on_doOpenJson_triggered()一律省略
{
    if ( !isBlankMap ){
        if ( pgmPixmapItem )
        {
            QMessageBox msgBox;
            msgBox.setText("是否采用空白地图?");
            msgBox.setInformativeText("rtk场景一般采用空白地图!");
            QPushButton* yesButton = msgBox.addButton(tr("是"), QMessageBox::AcceptRole);
            QPushButton* noButton = msgBox.addButton(tr("否"), QMessageBox::RejectRole);
            msgBox.addButton(tr("取消"), QMessageBox::RejectRole);
            msgBox.exec();
            if ( msgBox.clickedButton() == yesButton )
            {
                openBlankPixmap();
            }
            else if ( msgBox.clickedButton() == noButton )
            {
                ;
            }
            else
            {
                return;
            }
        }
        else
        {
            openBlankPixmap();
        }
    }

    if(ConstJson == "" && ConstImage != "")
        ConstJson = ConstImage.left(ConstImage.lastIndexOf("/"));
    else if (ConstJson == "")
        ConstJson = getUserPath();
    QString jsonPath = QFileDialog::getOpenFileName(this, tr("Open Json"), ConstJson, tr("Json FIles (*.json);;"));
    ConstJson = jsonPath;
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::critical(this, tr(WINDOW_ERROR), tr("不能打开json文件！\n请检查路径！"));
        return;
    }
    QTextStream stream(&file);
    setStreamUtf8(stream);
    jsonContent = stream.readAll();
    file.close();
    ui->statusbar->showMessage("json file is opened : " + jsonPath, 3000);
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonContent.toUtf8(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError && !doc.isNull()) {
        QMessageBox::critical(this, tr("json-rtk"), tr("Json格式错误！") + jsonError.errorString());
        return;
    }

    qDebug()<<"json 格式正确";
    m_path->clear();
    pgmScene->addItem(m_path);
    QJsonObject rootObj = doc.object();
    QJsonValue pointsValue = rootObj.value("points");
    if ( pointsValue.type() == QJsonValue::Array )
    {
        QJsonArray pointsArray = pointsValue.toArray();
        /*-------------------different_code-----------------------*/
        if ( pointsArray.size() < 2 )
        {
            QMessageBox::warning(this, tr("json-rtk"), tr("json内的点数量太少！"));
            return;
        }
        int jsoni = 0;
        qreal x1,x2,pix1,pix2,y1,piy1;
        /*-------------------different_code-----------------------*/
        for ( int i = 0; i < pointsArray.size(); i ++ )
        {
            QJsonValue pointValue = pointsArray.at(i);
            // 判断是否是object类型
            if (pointValue.type() == QJsonValue::Object) {
                // 转换为QJsonObject类型
                QJsonObject pointObj = pointValue.toObject();
                /*-------------------different_code-----------------------*/
                if ( jsoni == 0 )
                {
                    x1 = pointObj.value( "x" ).toDouble();
                    pix1 = pointObj.value( "pixel_x" ).toDouble();
                    y1 = pointObj.value( "y" ).toDouble();
                    piy1 = pointObj.value( "pixel_y" ).toDouble();
                    jsoni ++;
                }
                else if ( jsoni == 1 )
                {
                    x2 = pointObj.value( "x" ).toDouble();
                    pix2 = pointObj.value( "pixel_x" ).toDouble();
                    resolution = ( x1 - x2 ) / ( pix1 - pix2 );
                    origin_x = x1 - pix1 * resolution;
                    origin_y = y1 - ( 999 - piy1 ) * resolution;    //这里是按height为1000计算
                    qDebug()<<"resolution:"<<resolution<<"origin_x:"<<origin_x<<"origin_y:"<<origin_y;
                    jsoni ++;
                }
                /*-------------------different_code-----------------------*/
                QJsonValue point_pixel_x = pointObj.value( "pixel_x" );
                qreal pixel_x = point_pixel_x.toDouble();
                QJsonValue point_pixel_y = pointObj.value( "pixel_y" );
                qreal pixel_y = point_pixel_y.toDouble();

                bool has_attr = false;
                int arr[ATTR_NUM] = {0};
                if ( pointObj.value("road_id") != QJsonValue::Undefined && (has_attr = true) )
                    arr[0] = pointObj.value("road_id").toInt();
                if ( pointObj.value("road_width") != QJsonValue::Undefined && (has_attr = true) )
                    arr[1] = pointObj.value("road_width").toInt();
                if ( pointObj.value("bypass") != QJsonValue::Undefined && (has_attr = true) )
                    arr[2] = pointObj.value("bypass").toInt();
                if ( pointObj.value("seismic") != QJsonValue::Undefined && (has_attr = true) )
                    arr[3] = pointObj.value("seismic").toInt();
                if ( pointObj.value("turnAround") != QJsonValue::Undefined && (has_attr = true) )
                    arr[4] = pointObj.value("turnAround").toInt();
                if ( pointObj.value("trash") != QJsonValue::Undefined && (has_attr = true) )
                    arr[5] = pointObj.value("trash").toInt();
                if ( pointObj.value("charge") != QJsonValue::Undefined && (has_attr = true) )
                    arr[6] = pointObj.value("charge").toInt();
                if ( pointObj.value("back") != QJsonValue::Undefined && (has_attr = true) )
                    arr[7] = pointObj.value("back").toInt();
                if ( pointObj.value("clean_flag") != QJsonValue::Undefined && (has_attr = true) )
                    arr[8] = pointObj.value("clean_flag").toInt();
                if ( pointObj.value("parking_area") != QJsonValue::Undefined && (has_attr = true) )
                    arr[9] = pointObj.value("parking_area").toInt();
                if ( pointObj.value("curve_paths") != QJsonValue::Undefined && (has_attr = true) )
                    arr[10] = pointObj.value("curve_paths").toInt();
                if ( pointObj.value("ignore_obstacles") != QJsonValue::Undefined && (has_attr = true) )
                    arr[11] = pointObj.value("ignore_obstacles").toInt();
                if ( pointObj.value("reverse_traj") != QJsonValue::Undefined && (has_attr = true) )
                    arr[12] = pointObj.value("reverse_traj").toInt();

                std::map<QString, QString> self_df;
                QStringList keys = pointObj.keys();
                for ( auto it = keys.begin(); it != keys.end(); it ++ )
                {
                    if ( !KEY.count(*it) && (has_attr = true) )
                        self_df[*it] = QString::number( pointObj.value( *it ).toInt() );
                }

                if ( has_attr )
                    m_path->addNextPoint( QPointF( pixel_x, pixel_y ), arr, self_df );
                else
                    m_path->addNextPoint( QPointF( pixel_x, pixel_y ) );
            }
        }
    }
    /*-------------------different_code-----------------------*/
    on_do_adjust_triggered();
    ui->doOpenYaml->setEnabled(false);
    ui->doOpenJson->setEnabled(false);
    ui->doOpenJson_multiMPath->setEnabled(false);
    /*-------------------different_code-----------------------*/
}

//轨迹标注-框选点
void MainWindow::on_do_selectMultiPoint_polygen_triggered()
{
    pgmScene->create_pointSelect_polygon();
    pgmScene->addItem( pointSelected );
    setActionStatus(false);
    ui->statusbar->showMessage( "左键点击绘制点，右键点击结束绘制" );
}

//轨迹标注-清除框选
void MainWindow::on_do_clean_pointSelect_area_triggered()
{
    initPointSelect();
}
//func_轨迹标注_getAttributeData
void MainWindow::getAttributeData(std::string& str, int (&arr)[ATTR_NUM], std::map<QString, QString>& self_df)
{
    QString key,val;
    bool iskey = true;
    int index = 0;
    for( uint i = 0; i < str.size(); ++ i )
    {
        if ( index < ATTR_NUM )
        {
            if(str[i] == ',')
            {
                arr[index ++ ] = val.toInt();
                val = "";
            }
            else
                val += str[i];
        }
        else
        {
            if(str[i] == ',')
            {
                if(iskey)
                    iskey=false;
                else
                {
                    self_df[key]=val;
                    key = "";
                    val = "";
                    iskey=true;
                }
                continue;
            }
            else if(iskey)
                key+=str[i];
            else
                val+=str[i];
        }
    }
//    for (int i = 0; i < ATTR_NUM; i ++) qDebug()<< "attr_get_" << i << ": " << arr[i];
}
//slot_轨迹标注_进行赋值
void MainWindow::setAttributeData_area(std::string str)
{
    int arr[ATTR_NUM];
    std::map<QString, QString> self_df;
    getAttributeData(str, arr, self_df);

    for ( int i = 0; i < point_idx.size() - 1; ++ i )
    {
        BPathItem* tmp = m_path_list[i];
        for ( int j = 0; j < point_idx[i].size(); ++ j )
        {
            tmp->getElement(point_idx[i][j])->initAttr(arr,self_df);
        }
    }
    for ( int i = 0; i < point_idx[m_path_list.size()].size(); ++ i )
    {
        m_path->getElement(point_idx[m_path_list.size()][i])->initAttr(arr,self_df);
    }
}
//轨迹标注-添加属性
void MainWindow::on_do_addAttribute_triggered()
{
    //判断选框中有哪些点
    QPainterPath path = QPainterPathFromContours2d(mapSelectedTOcontours2d(pointSelected));
    point_idx.clear();
    int pointContainNum = 0;
    QList<int> idx;
    for ( auto& it : m_path_list )
    {
        for ( int i = 0; i < it->pointsNumber(); ++ i )
            if ( path.contains(it->getPoint(i)) )
                idx.push_back(i);
        point_idx.push_back(idx);
        pointContainNum += idx.size();
        idx.clear();
    }
    for ( int i = 0; i < m_path->pointsNumber(); ++ i )
        if ( path.contains(m_path->getPoint(i)) )
            idx.push_back(i);
    point_idx.push_back(idx);
    pointContainNum += idx.size();
    idx.clear();
    //如果没有点选中，则直接退出
    if ( pointContainNum == 0 )
    {
        QMessageBox::warning(this, tr(WINDOW_ERROR), tr("区域内没有点"));
        return;
    }
//    point_idx = idx;    //将idx传给MainWindow

    //多点编辑-判断是否属性统一，不统一进行提示
    attribute *attrTest = nullptr;
    bool attrIsSame = true;
    for ( int i = 0; i < point_idx.size() - 1; ++ i )
    {
        BPathItem* tmp = m_path_list[i];
        for ( int j = 0; j < point_idx[i].size(); ++ j )
        {
            if(nullptr == attrTest)
                attrTest = tmp->getElement(point_idx[i][j])->attr;
            auto b = tmp->getElement(point_idx[i][j])->attr;
            if(attrIsSame){
                if( (bool)b ^ (bool)attrTest || ( (bool)b && (bool)attrTest && *b != *attrTest ) )
                    attrIsSame = false;
            }
        }
    }
    for ( int i = 0; i < point_idx[m_path_list.size()].size(); ++ i )
    {
        if(nullptr == attrTest)
            attrTest = m_path->getElement(point_idx[m_path_list.size()][i])->attr;
        auto b = m_path->getElement(point_idx[m_path_list.size()][i])->attr;
        if(attrIsSame){
            if( (bool)b ^ (bool)attrTest || ( (bool)b && (bool)attrTest && *b != *attrTest ) )
                attrIsSame = false;
        }
    }
    if(!attrIsSame)
    {
        QMessageBox msgBox;
        msgBox.setText("选取点属性不统一！");
        msgBox.setInformativeText("是否继续进行属性编辑");
        QPushButton* yesButton = msgBox.addButton(tr("是"), QMessageBox::AcceptRole);
        QPushButton* noButton = msgBox.addButton(tr("否"), QMessageBox::RejectRole);

        msgBox.exec();

        if ( msgBox.clickedButton() == yesButton )
        {
            attrTest = nullptr;
        }
        else if ( msgBox.clickedButton() == noButton )
        {
            return;
        }
    }

    //开始更改属性
    initPointSelect();  //将选框重置了
    for ( int i = 0; i < point_idx.size() - 1; ++ i )
    {
        BPathItem* tmp = m_path_list[i];
        for ( int j = 0; j < point_idx[i].size(); ++ j )  //改一下绿色
        {
            tmp->getElement(point_idx[i][j])->color_seed = 3840;
            tmp->getElement(point_idx[i][j])->colorChange_colorSeed();
        }
    }
    for ( int i = 0; i < point_idx[m_path_list.size()].size(); ++ i )  //改一下绿色
    {
        m_path->getElement(point_idx[m_path_list.size()][i])->color_seed = 3840;
        m_path->getElement(point_idx[m_path_list.size()][i])->colorChange_colorSeed();
    }
    //打开AttributeAddDialog
    AttributeAddDialog *aad = new AttributeAddDialog(this,attrTest);
    connect(aad, &AttributeAddDialog::sendData, this, &MainWindow::setAttributeData_area); //这里直接用setAttributeData_area解决赋值问题
    aad->setAttribute(Qt::WA_QuitOnClose, false);
    aad->setAttribute(Qt::WA_DeleteOnClose);
    aad->show();
}

//轨迹标注/轨迹编辑-点选点(选定开始/结束点)
void MainWindow::on_do_selectMultiPoint_pointSelect_triggered()
{
    pgmScene->select_addAttribute_point();
    setActionStatus(false);
    ui->statusbar->showMessage( "开始点选区域，请选择开始点!" );
    connect( pgmScene, &BQGraphicsScene::selected_addAttribute_firstPoint,
            this, [this](QGraphicsItem* tmp_item) {
                firstPoint = tmp_item;
                ui->statusbar->showMessage( "已选择开始点，请选择结束点!" );
                qDebug()<<"240814#firstPoint:"<<firstPoint;
            });
    connect( pgmScene, &BQGraphicsScene::selected_addAttribute_secondPoint,
            this, [this](QGraphicsItem* tmp_item) {
                secondPoint = tmp_item;
                ui->statusbar->showMessage( "已选择结束点!", 5000 );
                disconnect(pgmScene, &BQGraphicsScene::selected_addAttribute_firstPoint, this, nullptr);
                disconnect(pgmScene, &BQGraphicsScene::selected_addAttribute_secondPoint, this, nullptr);
                setActionStatus(true);
                qDebug()<<"240814#secondPoint:"<<secondPoint;
                qDebug()<<"a_240814#secondPoint:"<<firstPoint->parentItem();
                qDebug()<<"a_240814#secondPoint:"<<secondPoint->parentItem();
                if (firstPoint->parentItem() == secondPoint->parentItem() )
                {
                    qDebug()<<"b_240814#firstPoint->parentItem() == secondPoint->parentItem()";
                    for (auto &pItem : pointSelected_line){
                        if (pItem == nullptr) continue;
                        pItem->colorChange_colorSeed();
                        pItem->setSelected(false);
                    }
                    pointSelected_line.clear();
                    BPathItem* tmp_pathItem = static_cast<BPathItem*>(firstPoint->parentItem());
                    bool to_select = false;
                    for (int i = 0; i < tmp_pathItem->pointsNumber(); ++ i){
                        BPointItem* tmp_pointItem = tmp_pathItem->getElement(i);
                        if (to_select || tmp_pointItem == firstPoint || tmp_pointItem == secondPoint ){
                            pointSelected_line.push_back(tmp_pointItem);
                            tmp_pointItem->colorChange_selected();
                            tmp_pointItem->setSelected(true);
                            qDebug()<<"b_240814#tmp_pointItem->setSelected(true);";
                        }
                        if (tmp_pointItem == firstPoint) to_select = !to_select;
                        if (tmp_pointItem == secondPoint) to_select = !to_select;
                    }
                }
                else
                {
                    QMessageBox::critical(this, tr("轨迹标注"), tr("选择的两个点不在同一条轨迹上！"));
                }
            });
}

//轨迹标注//轨迹编辑-取消选点
void MainWindow::on_do_clean_pointSelect_pointSelect_triggered()
{
    for (auto &pItem : pointSelected_line){
        if (pItem == nullptr) continue;
        pItem->colorChange_colorSeed();
        pItem->setSelected(false);
    }
    pointSelected_line.clear();
    ui->statusbar->showMessage( "已取消选点!", 5000 );
}
void MainWindow::setAttributeData_line(std::string str)
{
    int arr[ATTR_NUM];
    std::map<QString, QString> self_df;
    getAttributeData(str, arr, self_df);

    for ( auto& pItem : pointSelected_line ){
        if (pItem == nullptr) continue;
        pItem->initAttr(arr, self_df);
    }
}
//轨迹标注-按点选区添加属性
void MainWindow::on_do_addAttribute_pointSelect_triggered()
{
    //如果没有点选中，则直接退出
    if ( pointSelected_line.empty() )
    {
        QMessageBox::warning(this, tr(WINDOW_ERROR), tr("区域内没有点"));
        return;
    }
    //多点编辑-判断是否属性统一，不统一进行提示
    attribute *attrTest = nullptr;
    bool attrIsSame = true;
    for ( auto &pItem : pointSelected_line )
    {
        if (pItem == nullptr) continue;
        auto b = pItem->attr;
        if(nullptr == attrTest) attrTest = b;
        if(attrIsSame){
            if( (bool)b ^ (bool)attrTest || ( (bool)b && (bool)attrTest && *b != *attrTest ) )
                attrIsSame = false;
        }
    }
    if(!attrIsSame)
    {
        QMessageBox msgBox;
        msgBox.setText("选取点属性不统一！");
        msgBox.setInformativeText("是否继续进行属性编辑");
        QPushButton* yesButton = msgBox.addButton(tr("是"), QMessageBox::AcceptRole);
        QPushButton* noButton = msgBox.addButton(tr("否"), QMessageBox::RejectRole);

        msgBox.exec();

        if ( msgBox.clickedButton() == yesButton )
        {
            attrTest = nullptr;
        }
        else if ( msgBox.clickedButton() == noButton )
        {
            return;
        }
    }
    //开始更改属性
    //打开AttributeAddDialog
    AttributeAddDialog *aad = new AttributeAddDialog(this,attrTest);
    connect(aad, &AttributeAddDialog::sendData, this, &MainWindow::setAttributeData_line); //这里直接用setAttributeData_line解决赋值问题
    aad->setAttribute(Qt::WA_QuitOnClose, false);
    aad->setAttribute(Qt::WA_DeleteOnClose);
    aad->show();
}


//清除所有轨迹
void MainWindow::on_doCleanAllMPath_triggered()
{
    for ( auto& it : m_path_list )
    {
        pgmScene->removeItem(it);
        delete it;
    }
    m_path_list.clear();
    if ( m_path->scene() == pgmScene )
        pgmScene->removeItem(m_path);
    m_path->clear();

    //T1.5:轨迹清空 → 前视图层重算（清空扇形）
    fovRefreshPaths();
}

//保存并进行下一条轨迹绘制
void MainWindow::on_do_draw_next_mpath_triggered()
{
    m_path_list.push_back(m_path);    disconnect(pgmScene, SIGNAL(updatePoint(QPointF,QList<QPointF>,bool)), m_path, SLOT(pushPoint(QPointF,QList<QPointF>,bool)));

    m_path = new BPathItem;
    m_path->setPathWidth(m_path_width);

    //BPathItem : m_path
    connect(pgmScene, SIGNAL(updatePoint(QPointF,QList<QPointF>,bool)), m_path, SLOT(pushPoint(QPointF,QList<QPointF>,bool)));

    pgmScene->addItem(m_path);

    //T1.5:轨迹集合变化 → 前视图层重算
    fovRefreshPaths();
}

//加载Json文件(多条轨迹)
void MainWindow::on_doOpenJson_multiMPath_triggered()
{
    if(ConstJson == "" && ConstImage != "")
        ConstJson = ConstImage.left(ConstImage.lastIndexOf("/"));
    else if(ConstJson == "")
        ConstJson = getUserPath();

    QString jsonPath = QFileDialog::getOpenFileName(this, tr("Open Json"), ConstJson, tr("Json FIles (*.json);;"));
    ConstJson = jsonPath;
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::critical(this, tr(WINDOW_ERROR), tr("不能打开json文件！\n请检查路径！"));
        return;
    }
    QTextStream stream(&file);
    setStreamUtf8(stream);
    jsonContent = stream.readAll();
    file.close();
    ui->statusbar->showMessage("Json文件已被打开，路径为 : " + jsonPath, 3000);
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonContent.toUtf8(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError && !doc.isNull()) {
        QMessageBox::critical(this, tr("json-rtk"), tr("Json格式错误！") + jsonError.errorString());
        return;
    }
    m_path->clear();
    pgmScene->addItem(m_path);

    /*-------------------different_code-----------------------*/
    int network_id  = INT_MIN;
    bool all_has_network_id = true;
    /*-------------------different_code-----------------------*/

    QJsonObject rootObj = doc.object();
    QJsonValue pointsValue = rootObj.value("points");
    if ( pointsValue.type() == QJsonValue::Array )
    {
        QJsonArray pointsArray = pointsValue.toArray();
        qDebug()<<"pointsArray.size();"<<pointsArray.size();
        for ( int i = 0; i < pointsArray.size(); i ++ )
        {
            qDebug()<<"i:"<<i;
            QJsonValue pointValue = pointsArray.at(i);
            if (pointValue.type() == QJsonValue::Object) {
                QJsonObject pointObj = pointValue.toObject();
                QJsonValue point_pixel_x = pointObj.value( "pixel_x" );
                qreal pixel_x = point_pixel_x.toDouble();
                QJsonValue point_pixel_y = pointObj.value( "pixel_y" );
                qreal pixel_y = point_pixel_y.toDouble();
                bool has_attr = false;
                int arr[ATTR_NUM] = {0};
                if ( pointObj.value("road_id") != QJsonValue::Undefined && (has_attr = true) )
                    arr[0] = pointObj.value("road_id").toInt();
                if ( pointObj.value("road_width") != QJsonValue::Undefined && (has_attr = true) )
                    arr[1] = pointObj.value("road_width").toInt();
                if ( pointObj.value("bypass") != QJsonValue::Undefined && (has_attr = true) )
                    arr[2] = pointObj.value("bypass").toInt();
                if ( pointObj.value("seismic") != QJsonValue::Undefined && (has_attr = true) )
                    arr[3] = pointObj.value("seismic").toInt();
                if ( pointObj.value("turnAround") != QJsonValue::Undefined && (has_attr = true) )
                    arr[4] = pointObj.value("turnAround").toInt();
                if ( pointObj.value("trash") != QJsonValue::Undefined && (has_attr = true) )
                    arr[5] = pointObj.value("trash").toInt();
                if ( pointObj.value("charge") != QJsonValue::Undefined && (has_attr = true) )
                    arr[6] = pointObj.value("charge").toInt();
                if ( pointObj.value("back") != QJsonValue::Undefined && (has_attr = true) )
                    arr[7] = pointObj.value("back").toInt();
                if ( pointObj.value("clean_flag") != QJsonValue::Undefined && (has_attr = true) )
                    arr[8] = pointObj.value("clean_flag").toInt();
                if ( pointObj.value("parking_area") != QJsonValue::Undefined && (has_attr = true) )
                    arr[9] = pointObj.value("parking_area").toInt();
                if ( pointObj.value("curve_paths") != QJsonValue::Undefined && (has_attr = true) )
                    arr[10] = pointObj.value("curve_paths").toInt();
                if ( pointObj.value("ignore_obstacles") != QJsonValue::Undefined && (has_attr = true) )
                    arr[11] = pointObj.value("ignore_obstacles").toInt();
                if ( pointObj.value("reverse_traj") != QJsonValue::Undefined && (has_attr = true) )
                    arr[12] = pointObj.value("reverse_traj").toInt();
                std::map<QString, QString> self_df;
                /*-------------------different_code-----------------------*/
                bool is_new_road = false;
                bool has_network_id = false;
                /*-------------------different_code-----------------------*/
                QStringList keys = pointObj.keys();
                for ( auto it = keys.begin(); it != keys.end(); it ++ )
                {
                    if ( !KEY.count(*it) && (has_attr = true) ){
                        self_df[*it] = QString::number( pointObj.value( *it ).toInt() );
                        /*-------------------different_code-----------------------*/
                        if ( (*it) == "network_id" )
                        {
                            has_network_id = true;
                            if ( network_id == INT_MIN )
                                network_id = pointObj.value( *it ).toInt();
                            else if ( network_id != pointObj.value( *it ).toInt() )
                            {
                                is_new_road = true;
                                network_id = pointObj.value( *it ).toInt();
                            }
                        }
                        /*-------------------different_code-----------------------*/
                    }
                }
                /*-------------------different_code-----------------------*/
                if ( is_new_road )
                    on_do_draw_next_mpath_triggered();
                /*-------------------different_code-----------------------*/

                if ( has_attr )
                    m_path->addNextPoint( QPointF( pixel_x, pixel_y ), arr, self_df );
                else
                    m_path->addNextPoint( QPointF( pixel_x, pixel_y ) );

                /*-------------------different_code-----------------------*/
                if ( !has_network_id )
                    all_has_network_id = false;
                /*-------------------different_code-----------------------*/
            }
        }
    }
    /*-------------------different_code-----------------------*/
    if ( !all_has_network_id )
        QMessageBox::critical(this, tr(WINDOW_ERROR), tr("此Json文件含有未定义network_id的节点，不符合规则"));
    if ( resolution == 0 )
        on_doOpenYaml_triggered();
    /*-------------------different_code-----------------------*/
}

//绘制功能区
void MainWindow::on_do_draw_functional_area_triggered()
{
    pgmScene->create_areaSelect_polygon();
    pgmScene->addItem(areaSelected);
    setActionStatus(false);
    ui->statusbar->showMessage( "左键点击绘制点，右键点击结束绘制" );
}

//清除当前功能区
void MainWindow::on_do_clean_current_functional_area_triggered()
{
    initAreaSelect();
}

//清除所有功能区
void MainWindow::on_do_clean_all_functional_area_triggered()
{
    for ( auto& it : areaSelected_list )
    {
        pgmScene->removeItem(it);
        delete it;
    }
    areaSelected_list.clear();
    initAreaSelect();
}

//保存并进行下一个功能区绘制
void MainWindow::on_do_draw_next_functional_area_triggered()
{
    areaSelected_list.push_back(areaSelected);
    disconnect(pgmScene, SIGNAL(areaSelect_polygon_addPoint(QPointF,bool)),
               areaSelected, SLOT(cur_ploygon_addPoint(QPointF,bool)));

    SetFunctionalAreaName* dialog = new SetFunctionalAreaName;
    connect(dialog, &SetFunctionalAreaName::sendText, areaSelected, &MapSelectItem::setText);
    dialog->exec();
    disconnect(dialog, &SetFunctionalAreaName::sendText, areaSelected, &MapSelectItem::setText);

    areaSelected = new MapSelectItem;

    //MapSelectedItem : areaSelected
    connect(pgmScene, SIGNAL(areaSelect_polygon_addPoint(QPointF,bool)),
            areaSelected, SLOT(cur_ploygon_addPoint(QPointF,bool)));

}

//功能区绘制-停车区绘制
void MainWindow::on_do_draw_parkingLot_area_triggered()
{
    if (!parkingLotSelected.isNull() && parkingLotSelected->isComplete()) {
        disconnect(pgmScene, nullptr, parkingLotSelected, nullptr );
        parkingLotSelected_list.append(parkingLotSelected);
        parkingLotSelected = nullptr;
    }
    initParkingLotSelect();
    pgmScene->create_parkingLot_polygon();
    if (!parkingLotSelected.isNull()) {
        pgmScene->addItem(parkingLotSelected);
    } else {
        qCritical() << "initParkingLotSelect() 未成功创建 parkingLotSelected！";
        QMessageBox::critical(this, tr("创建失败"),
                              tr("创建停车区域失败，parkingLotSelected 为空。"));
        return;
    }
    setActionStatus(false);
    parkingLotSelected->setName(QString("停车区%1").arg(parkingLotSelected_list.size() + 1));
    ui->statusbar->showMessage( "绘制停车区步骤：1.左键点击长边起始点；2.左键点击长边终点；3.左键点击短边终点" );
}

//功能区绘制-停车区保存
void MainWindow::on_do_save_parkingLot_area_triggered()
{
    // 保存当前绘制完成的停车区
    if (!parkingLotSelected.isNull()) {
        if (parkingLotSelected->isComplete()) {
            disconnect(pgmScene, nullptr, parkingLotSelected, nullptr);
            parkingLotSelected_list.append(parkingLotSelected);
            parkingLotSelected = nullptr;
        } else {
            qWarning() << "parkingLotSelected还没有创建完成，流程有误！";
            QMessageBox::critical(this, tr(WINDOW_ERROR),
                                  tr("当前的停车区绘制还没有创建完成，流程有误！"));
            return;
        }
    }
    if ( resolution_isZero() ) return;
    const int height = pgmPixmapItem->pixmap().height();
    QJsonArray rectanglesArray;

    for (const auto& parkingArea : parkingLotSelected_list) {
        if (parkingArea.isNull()) continue;
        QJsonObject jsonObj;
        QPointF startP = parkingArea->getStartPoint();
        QPointF endP = parkingArea->getEndPoint();
        QJsonArray rectangleLongEdge;
        QJsonArray point1;
        point1.append( startP.x() * resolution + origin_x );
        point1.append( ( height - 1 - startP.y() ) * resolution + origin_y );
        rectangleLongEdge.append(point1);

        QJsonArray point2;
        point2.append( endP.x() * resolution + origin_x );
        point2.append( ( height - 1 - endP.y() ) * resolution + origin_y );
        rectangleLongEdge.append(point2);

        jsonObj["rectangle_long_edge"] = rectangleLongEdge;
        jsonObj["rectangle_width"] = parkingArea->getWidth() * resolution;

        rectanglesArray.append(jsonObj);
    }

    if (rectanglesArray.isEmpty()) {
        QMessageBox::warning(this, tr("保存失败"), tr("没有可保存的停车区域！"));
        return;
    }

    generateDefaultParkingAreaPath();

    QString savePath = QFileDialog::getSaveFileName(this, tr("Save parking_Area_Json"), parkingAreaJsonPath,
                                                       tr("file Json (*.json);;"
                                                          "All files (*);;"));
    if (savePath.isEmpty()) return;
    if (!savePath.endsWith(".json", Qt::CaseInsensitive)) {
        savePath += ".json";
    }
    QFile file( savePath );
    if (file.open( QIODevice::WriteOnly | QIODevice::Truncate )) {
        QJsonDocument doc(rectanglesArray);
        file.write(doc.toJson(QJsonDocument::Indented));  // 使用缩进格式保存
        file.close();
        this->ui->statusbar->showMessage(tr("停车区 JSON 文件已保存到: ") + file.fileName(), 5000);
        parkingAreaJsonPath = savePath;
    } else {
        QMessageBox::critical(this, tr("保存失败"), tr("无法打开文件进行保存，请检查权限或路径！"));
    }
}

//功能区绘制-停车区加载
void MainWindow::on_do_load_parkingLot_area_triggered()
{
    if ( resolution_isZero() ) return;
    generateDefaultParkingAreaPath();
    // 1. 选择 JSON 文件
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("加载停车区 JSON 文件"),
        parkingAreaJsonPath,
        tr("JSON 文件 (*.json);;所有文件 (*)"));

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("打开失败"), tr("无法打开指定文件进行读取。"));
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    // 2. 解析 JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        QMessageBox::critical(this, tr("格式错误"), tr("JSON 格式错误或不是数组结构！"));
        return;
    }

    // 3. 清空当前已加载的区域
    for (auto& item : parkingLotSelected_list) {
        if (!item.isNull()) {
            if (item->scene()) pgmScene->removeItem(item);
            delete item;
        }
    }
    parkingLotSelected_list.clear();

    int height = pgmPixmapItem->pixmap().height();

    // 4. 解析每一个停车区对象
    QJsonArray array = doc.array();
    for (const auto& value : array) {
        if (!value.isObject()) continue;
        QJsonObject obj = value.toObject();

        if (!obj.contains("rectangle_long_edge") || !obj.contains("rectangle_width"))
            continue;

        QJsonArray edgeArray = obj["rectangle_long_edge"].toArray();
        if (edgeArray.size() != 2) continue;

        QPointF start, end;

        // 解析起点
        QJsonArray point1 = edgeArray[0].toArray();
        if (point1.size() == 2) {
            double x = (point1[0].toDouble() - origin_x) / resolution;
            double y = height - 1 - (point1[1].toDouble() - origin_y) / resolution;
            start = QPointF(x, y);
        }

        // 解析终点
        QJsonArray point2 = edgeArray[1].toArray();
        if (point2.size() == 2) {
            double x = (point2[0].toDouble() - origin_x) / resolution;
            double y = height - 1 - (point2[1].toDouble() - origin_y) / resolution;
            end = QPointF(x, y);
        }

        double width = obj["rectangle_width"].toDouble() / resolution;

        QPointF direction = end - start;
        QPointF normDir(direction.y(), -direction.x());
        normDir /= std::hypot(normDir.x(), normDir.y()); // 单位化

        QPointF widthPoint = start + normDir * width; // 反推出来的短边终点

        // 5. 创建并添加 ParkingLotItem
        ParkingLotItem* item = new ParkingLotItem;
        item->setStartPoint(start);
        item->setEndPoint(end);
        item->setWidth(widthPoint);
        item->setName(QString("停车区%1").arg(parkingLotSelected_list.size() + 1));
        pgmScene->addItem(item);
        parkingLotSelected_list.append(item);
    }

    ui->statusbar->showMessage(tr("停车区数据加载完成，共加载 %1 个区域").arg(parkingLotSelected_list.size()), 5000);

}

//func_parkingAreaJsonPath预处理
void MainWindow::generateDefaultParkingAreaPath()
{
    if (parkingAreaJsonPath.isEmpty()) {
        parkingAreaJsonPath = !ConstJson.isEmpty() ? ConstJson :
                                  (!ConstImage.isEmpty() ? ConstImage : getUserPath());
        // 统一处理默认文件名：加 _parking_area.json 后缀
        QFileInfo fileInfo(parkingAreaJsonPath);
        QString baseName = fileInfo.completeBaseName(); // 不含扩展名
        QString dirPath = fileInfo.absolutePath();      // 目录
        parkingAreaJsonPath = dirPath + "/" + baseName + "_parking_area.json";
    }
}

//记录功能区
void MainWindow::on_do_save_functional_area_triggered()
{
    //使用csv的保存逻辑，暂时弃用，改用json保存
//    QString csvPath = ConstImage.left(ConstImage.lastIndexOf("/"));
//    csvPath = QFileDialog::getSaveFileName(this, tr("Save csv"), csvPath,
//                                           tr("file CSV (*.csv);;"
//                                              "All files (*);;"));
//    std::ofstream outFile;
//    outFile.open( UTF8ToGBK(csvPath), std::ios::out );
//    //开始保存
//    int i = 0;
//    for ( auto& it : areaSelected_list )
//    {
//        for ( auto& it2 : it->polygons )
//        {
//            outFile << "#polygon " << i ++ << ':' << UTF8ToGBK( it->getText() ) << std::endl;
//            for ( auto& it3 : it2 )
//            {
//                outFile << std::to_string( it3->getPoint().x() ) << ','
//                        << std::to_string( it3->getPoint().y() ) << std::endl;
//            }
//            outFile << "#end" << std::endl;
//        }
//    }
//    outFile.close();

    return;

    QString areaPath = ConstImage.left(ConstImage.lastIndexOf("/"));
    areaPath = QFileDialog::getSaveFileName(this, tr("记录功能区"), areaPath,
                                               tr("file Json (*.json);;"
                                                  "All files (*);;"));

    QJsonArray pointsArray;

}


void MainWindow::on_do_load_functional_area_triggered()
{

}

bool MainWindow::resolution_isZero()
{
    if ( resolution == 0 )
    {
        QMessageBox::critical(this,tr("json-rtk"), tr("需要(2选1):\n1.打开一张地图并加载yaml文件\n2.打开json-rtk文件"));
        return true;
    }
    return false;
}

//func_移除pointSelected_line内的空指针
void MainWindow::eraseNullptr_pointSelected_line()
{
    pointSelected_line.erase(
        std::remove_if(pointSelected_line.begin(), pointSelected_line.end(),
                       [](const QPointer<BPointItem>& item) { return item.isNull(); }),
        pointSelected_line.end());

}

//轨迹编辑-复制点选区
void MainWindow::on_do_copyPoints_pointSelect_triggered()
{
    if ( resolution_isZero() ) return;

    eraseNullptr_pointSelected_line();
    //如果没有点选中，则直接退出
    if ( pointSelected_line.empty() )
    {
        QMessageBox::warning(this, tr(WINDOW_ERROR), tr("区域内没有点"));
        return;
    }

//    if ( pointSelected_line[0]->parentItem() == m_path )
//        qDebug()<< "241211#正确读取";
//    else
//    {
//        QMessageBox::critical(this, tr(WINDOW_ERROR), tr("选取点的所在轨迹必须为当前编辑轨迹！"));
//        return;
//    }
    //多点复制-打开编辑框
    //输入框
    copyPoints_dialog = new copyPointsDialog(this, resolution);
    //信号处理
    connect(copyPoints_dialog, &copyPointsDialog::selectBasePoint, this,
            &MainWindow::selectingCopyPointsBasePoint );
    connect(copyPoints_dialog, &copyPointsDialog::selectOffsetPoint, this,
            &MainWindow::selectingCopyPointsOffsetPoint );
    connect(pgmScene, &BQGraphicsScene::selected_copyPointsBasePoint, copyPoints_dialog, &copyPointsDialog::setBasePoint);
    connect(pgmScene, &BQGraphicsScene::selected_copyPointsOffsetPoint, copyPoints_dialog, &copyPointsDialog::setOffsetPoint);
    connect(pgmScene, &BQGraphicsScene::selected_copyPointsBasePoint, this, [this](QPointF p) {
//        this->ui->statusbar->showMessage(tr("已选择基准点: (%1, %2)").arg(p.x()).arg(p.y()));
        if (this->ui->statusbar) {
            this->ui->statusbar->showMessage(tr("已选择基准点: (%1, %2)").arg(p.x()).arg(p.y()));
        } else {
            qDebug() << "statusbar is nullptr!";
        }
//        copyPoints_dialog->raise(); //这里别使用activateWindow()
        if (copyPoints_dialog) {
            copyPoints_dialog->raise();
        } else {
            qDebug() << "copyPoints_dialog is nullptr!";
        }
    });
    connect(pgmScene, &BQGraphicsScene::selected_copyPointsOffsetPoint, this, [this](QPointF p) {
//        this->ui->statusbar->showMessage(tr("已选择偏移点: (%1, %2)").arg(p.x()).arg(p.y()));
        if (this->ui->statusbar) {
            this->ui->statusbar->showMessage(tr("已选择偏移点: (%1, %2)").arg(p.x()).arg(p.y()));
        } else {
            qDebug() << "statusbar is nullptr!";
        }
        if (copyPoints_dialog) {
            copyPoints_dialog->raise();
        } else {
            qDebug() << "copyPoints_dialog is nullptr!";
        }
    });
    connect(copyPoints_dialog, &copyPointsDialog::apply_pointsOffset_copy, this,
            &MainWindow::copyPoints_insertNext );

    //非模态对话框
    copyPoints_dialog->setAttribute( Qt::WA_QuitOnClose, false );
    copyPoints_dialog->setAttribute(Qt::WA_DeleteOnClose);
    //    dialog->setWindowTitle(tr(""));
    copyPoints_dialog->show();
}

//slot_复制点选区_选择基准点
void MainWindow::selectingCopyPointsBasePoint()
{
    ui->statusbar->showMessage("左键点击选点");
    pgmScene->select_copyPointsBasePoint();
}

//slot_复制点选区_选择偏移点
void MainWindow::selectingCopyPointsOffsetPoint()
{
    ui->statusbar->showMessage("左键点击选点");
    pgmScene->select_copyPointsOffsetPoint();
}

//slot_复制点选区_复制
void MainWindow::copyPoints_insertNext(QPointF offset)
{
    eraseNullptr_pointSelected_line();
    BPathItem* tmp_pathItem = static_cast<BPathItem*>(pointSelected_line[0]->parentItem());
    int index_insert = tmp_pathItem->getSequence(pointSelected_line.back());
    for ( auto& pItem : pointSelected_line )
    {
        if (pItem == nullptr) continue;
        QPointF* new_p = new QPointF( pItem->getPoint() + offset );
        attribute* new_attr = nullptr;
        if ( pItem->attr )
            new_attr = new attribute( pItem->attr );
        tmp_pathItem->insertPoint( ++ index_insert, new_p, new_attr );
    }
    on_do_clean_pointSelect_pointSelect_triggered();
    copyPoints_dialog->close();
}

//轨迹编辑-删除点选区
void MainWindow::on_do_deletePoints_pointSelect_triggered()
{
    eraseNullptr_pointSelected_line();
    //如果没有点选中，则直接退出
    if ( pointSelected_line.empty() )
    {
        QMessageBox::warning(this, tr(WINDOW_ERROR), tr("区域内没有点"));
        return;
    }
    BPathItem* tmp_pathItem = static_cast<BPathItem*>(pointSelected_line[0]->parentItem());
    for (auto& pItem : pointSelected_line)
    {
        if (pItem == nullptr) continue;
        tmp_pathItem->deletePoint(pItem);
    }
    pointSelected_line.clear();
}

//轨迹编辑-移动点选区
void MainWindow::on_do_movePoints_pointSelect_triggered()
{
    if ( resolution_isZero() ) return;

    //如果没有点选中，则直接退出
    if ( pointSelected_line.empty() )
    {
        QMessageBox::warning(this, tr(WINDOW_ERROR), tr("区域内没有点"));
        return;
    }

    //多点移动-打开编辑框
    movePoints_dialog = new copyPointsDialog(this, resolution);
//    movePoints_dialog->ui->pushButton_apply_copy->setText("移动点选区");
    movePoints_dialog->setApplyButtonText("移动点选区");
    movePoints_dialog->setWindowTitle("移动编辑");
    //信号处理
    connect(movePoints_dialog, &copyPointsDialog::selectBasePoint, this,
            &MainWindow::selectingCopyPointsBasePoint );
    connect(movePoints_dialog, &copyPointsDialog::selectOffsetPoint, this,
            &MainWindow::selectingCopyPointsOffsetPoint );
    connect(pgmScene, &BQGraphicsScene::selected_copyPointsBasePoint, movePoints_dialog, &copyPointsDialog::setBasePoint);
    connect(pgmScene, &BQGraphicsScene::selected_copyPointsOffsetPoint, movePoints_dialog, &copyPointsDialog::setOffsetPoint);
    connect(pgmScene, &BQGraphicsScene::selected_copyPointsBasePoint, this, [this](QPointF p) {
//        this->ui->statusbar->showMessage(tr("已选择基准点: (%1, %2)").arg(p.x()).arg(p.y()));
        if (this->ui->statusbar) {
            this->ui->statusbar->showMessage(tr("已选择基准点: (%1, %2)").arg(p.x()).arg(p.y()));
        } else {
            qDebug() << "statusbar is nullptr!";
        }
//        movePoints_dialog->raise();//这里别使用activateWindow()
        if (movePoints_dialog) {
            movePoints_dialog->raise();
        } else {
            qDebug() << "movePoints_dialog is nullptr!";
        }
    });
    connect(pgmScene, &BQGraphicsScene::selected_copyPointsOffsetPoint, this, [this](QPointF p) {
//        this->ui->statusbar->showMessage(tr("已选择偏移点: (%1, %2)").arg(p.x()).arg(p.y()));
        if (this->ui->statusbar) {
            this->ui->statusbar->showMessage(tr("已选择偏移点: (%1, %2)").arg(p.x()).arg(p.y()));
        } else {
            qDebug() << "statusbar is nullptr!";
        }
//        movePoints_dialog->raise();
        if (movePoints_dialog) {
            movePoints_dialog->raise();
        } else {
            qDebug() << "movePoints_dialog is nullptr!";
        }
    });
    connect(movePoints_dialog, &copyPointsDialog::apply_pointsOffset_copy, this,
            &MainWindow::movePoints_pointSelected );


    //非模态对话框
    movePoints_dialog->setAttribute( Qt::WA_QuitOnClose, false );
    movePoints_dialog->setAttribute(Qt::WA_DeleteOnClose);
    //    dialog->setWindowTitle(tr(""));
    movePoints_dialog->show();
}

//slot_轨迹编辑-移动点选区_移动
void MainWindow::movePoints_pointSelected(QPointF offset)
{
    eraseNullptr_pointSelected_line();
//    BPathItem* tmp_pathItem = static_cast<BPathItem*>(pointSelected_line[0]->parentItem());
    for ( auto& pItem : pointSelected_line )
    {
        if (pItem == nullptr) continue;
        pItem->rpoint() += offset;
    }
    pgmScene->update();
}

//rtk轨迹编辑-点选区-沿轨迹绘制虚拟墙
void MainWindow::on_do_drawLine_pointsSelectedBased_triggered()
{
    if ( resolution_isZero() ) return;

    eraseNullptr_pointSelected_line();
    // 如果没有点选中，则直接退出
    if ( pointSelected_line.size() < 2 )
    {
        QMessageBox::warning(this, tr(WINDOW_ERROR), tr("至少选中2个轨迹点"));
        return;
    }
//    on_do_modifyPGMSize_triggered();
    drawLine_PointsSelectedBased_Dialog = new DrawLine_PointsSelectedBased_Dialog(this, resolution);
    connect(drawLine_PointsSelectedBased_Dialog, &DrawLine_PointsSelectedBased_Dialog::drawLine_PointsSelectedBased_signal,
            this, &MainWindow::drawLine_pointsSelectedBased);
    //非模态对话框
    drawLine_PointsSelectedBased_Dialog->setAttribute( Qt::WA_QuitOnClose, false );
    drawLine_PointsSelectedBased_Dialog->setAttribute(Qt::WA_DeleteOnClose);
    //    dialog->setWindowTitle(tr(""));
    drawLine_PointsSelectedBased_Dialog->show();
}

//slot_沿轨迹绘制虚拟墙_绘制
void MainWindow::drawLine_pointsSelectedBased(int offsetDirection, int offsetDistance,
                                    int extendLength, bool curveClosure, int lineWidth)
{
    eraseNullptr_pointSelected_line();
    qDebug() << "250112#drawLine_pointsSelectedBased:" << offsetDirection << ' ' <<
        offsetDistance << ' '  << extendLength << ' ' << curveClosure << ' ' <<
        lineWidth;

    qreal offD = offsetDistance / 100.0 / resolution,
          extL = extendLength / 100.0 / resolution,
          lineW = lineWidth / 100.0 / resolution;

    QPixmap pgmPixmap = CvMat8UToQPixmap(imgTmp);

    // 3. 创建 QPainter 对象并与 QPixmap 关联
    QPainter painter(&pgmPixmap);
//    painter.setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);

    // 4. 设置绘图属性，比如画笔颜色和粗细
    QPen pen(Qt::black);
    pen.setWidth(lineW);
    painter.setPen(pen);

    QPolygonF Ploygon;
    Ploygon.clear();
    for ( int i = 0; i < pointSelected_line.size(); i ++ )
    {
        if (i == 0)
        {
            QPointF a = pointSelected_line[0]->getPoint(), b = pointSelected_line[1]->getPoint();
            qreal theta = atan2(a.y() - b.y(), b.x() - a.x());
            qreal alpha = theta - 0.5 * M_PI, l1 = offD + 0.5 * lineW, l2 = extL;
            QPointF res;
            if (offsetDirection == 0 )
                res = a + QPointF(cos(alpha) * l1, - sin(alpha) * l1) - QPointF(cos(theta) * l2, - sin(theta) * l2);
            else
                res = a - QPointF(cos(alpha) * l1, - sin(alpha) * l1) - QPointF(cos(theta) * l2, - sin(theta) * l2);
            Ploygon << res;
        }
        else if (i == pointSelected_line.size() - 1)
        {
            QPointF a = pointSelected_line[i - 1]->getPoint(), b = pointSelected_line[i]->getPoint();
            qreal theta = atan2(a.y() - b.y(), b.x() - a.x());
            qreal alpha = theta - 0.5 * M_PI, l1 = offD + 0.5 * lineW, l2 = extL;
            QPointF res;
            if (offsetDirection == 0 )
                res = b + QPointF(cos(alpha) * l1, - sin(alpha) * l1) + QPointF(cos(theta) * l2, - sin(theta) * l2);
            else
                res = b - QPointF(cos(alpha) * l1, - sin(alpha) * l1) + QPointF(cos(theta) * l2, - sin(theta) * l2);
            Ploygon << res;
        }
        else
        {
            QPointF a = pointSelected_line[i - 1]->getPoint(), b = pointSelected_line[i]->getPoint(), c = pointSelected_line[i + 1]->getPoint();
            qreal theta_1 = atan2(a.y() - b.y(), b.x() - a.x()), theta_2 = atan2(b.y() - c.y(), c.x() - b.x());
            qreal beta = 0.5 * (theta_2 - theta_1), l1 = offD + 0.5 * lineW;
            qreal l2 = l1 * tan(beta), alpha = theta_1 - 0.5 * M_PI;
            QPointF res;
            if (offsetDirection == 0 )
                res = b + QPointF(cos(alpha) * l1, - sin(alpha) * l1) + QPointF(cos(theta_1) * l2, - sin(theta_1) * l2);
            else
                res = b - QPointF(cos(alpha) * l1, - sin(alpha) * l1) - QPointF(cos(theta_1) * l2, - sin(theta_1) * l2);
            Ploygon << res;
        }
    }
    if (curveClosure) Ploygon << Ploygon.at(0);
    painter.drawPolyline(Ploygon);

    // 6. 结束绘制
    painter.end();

    //更新图片
    pgmPixmapItem->setPixmap(pgmPixmap);
    imgTmp = QPixmapToCvMat8U(pgmPixmap);
    flag = true;
    qDebug() << "250111:" << ConstImage;
}

//rtk轨迹编辑-修改地图尺寸
void MainWindow::on_do_modifyPGMSize_triggered()
{
    if ( resolution_isZero() ) return;
    //获取tmp图片
    QPixmap pgmPixmap = CvMat8UToQPixmap(imgTmp);
    qreal h1 = pgmPixmap.height();
    // 计算场景矩形以包含所有项
    QRectF itemsRect = pgmScene->itemsBoundingRect();

    QPixmap extendedPixmap(itemsRect.width() + 1000, itemsRect.height() + 1000);
    qreal h2 = extendedPixmap.height();
    extendedPixmap.fill(Qt::white);  // 用白色填充整个区域

    // 使用 QPainter 将原始图像绘制到新的 QPixmap 上，左上角对齐
    QPainter painter(&extendedPixmap);
    painter.drawPixmap(0, 0, pgmPixmap);  // 绘制原始图像到新 QPixmap 的左上角
    painter.end();  // 结束绘制
    //更新图片
    updateImage(extendedPixmap);
    imgTmp = QPixmapToCvMat8U(extendedPixmap);

//    origin_y = y1 - ( 999 - piy1 ) * resolution;    //这里是按height为1000计算
    origin_y = origin_y + ( h1 - h2 ) * resolution;
    size->setText("文件大小可保存后查看");
    pixel->setText(QString::number(extendedPixmap.width())
                   + " x " + QString::number(extendedPixmap.height()));
//    flag = true;
}

//rtk轨迹编辑-修正yaml文件(需要rtk建图时yaml原文件)(若rtk地图含有绘制内容，则必须修正)
void MainWindow::on_do_saveYaml_forRTK_triggered()
{
    if(ConstImage != "")
    {
        ConstYaml = ConstImage.left(ConstImage.lastIndexOf("/"));
    }
    else if (ConstJson != "")
    {
        ConstYaml = ConstJson.left(ConstJson.lastIndexOf("/"));
    }
    else
    {
        ConstYaml = getUserPath();
    }
    QString yamlPath = QFileDialog::getOpenFileName(this, tr("Open YAML"), ConstYaml, tr("YAML FIles (*.yaml *.yml);;"));
    ConstYaml = yamlPath;
    std::string filePath = UTF8ToGBK(yamlPath);

    // 1. 检查文件是否存在
    std::ifstream file(filePath);
    if (!file.is_open()) {
        QMessageBox::critical(this, tr("YAML error"), tr("文件不存在或者文件无法打开！"));
        return;  // 退出函数
    }
    // 2. 检查文件是否为空
    if (file.peek() == std::ifstream::traits_type::eof()) {
        QMessageBox::critical(this, tr("YAML error"), tr("文件不能为空文件，必须为原始的rtk录轨迹保存得到的yaml文件！"));
        return;
    }
    try {
        YAML::Node node;
        node = YAML::LoadFile(filePath);
        // 4. 检查必需的键是否存在
        if (!node["image"]) {
            QMessageBox::critical(this, tr("YAML error"), tr("文件必须为原始的rtk录轨迹保存得到的yaml文件！\n当前文件缺少image项"));
            return;
        }
        node["resolution"] = resolution;
//        node["origin"] = YAML::Node({origin_x, origin_y, yaw});
        node["origin"] = YAML::Node();
        node["origin"].push_back(origin_x);
        node["origin"].push_back(origin_y);
        node["origin"].push_back(yaw);
        // 将修改后的数据写回文件
        YAML::Emitter emitter;
        emitter << node;
        qDebug()<< "saveYaml_forRTK: " << emitter.c_str();
        std::ofstream fout(filePath);
        fout << std::fixed << std::setprecision(6) << node;
//        fout << emitter.c_str();
        fout.close();
    } catch (const YAML::BadFile& e) {
        // 4. 捕获并处理异常
        qDebug() << "Error loading YAML file: " << e.what();
    }
    openYaml(yamlPath); //直接打开这个文件测试yaml是否保存成功了
}

//func_检查是否处于有效的 ROS 环境
//func_拼接加载用户 bash 环境的前缀
//macOS 上 ~/.bashrc 经常不存在，原来的 `source ~/.bashrc && ...` 会直接失败，
//导致后面的 roslaunch/rosbag 命令永远不执行，这里改成"存在才加载"
static QString bashRcPreamble()
{
    return QStringLiteral("if [ -f \"$HOME/.bashrc\" ]; then . \"$HOME/.bashrc\"; fi; ");
}

bool MainWindow::isROSEnvironmentReady()
{
#ifdef Q_OS_WIN
    QMessageBox::warning(this, "错误", "离线建图相关功能需要 ROS1 (Linux) 环境，Windows 版暂不支持。");
    return false;
#endif

    QProcess checkProcess;
    QString command = bashRcPreamble() + "command -v roslaunch";
    checkProcess.start("bash", QStringList() << "-c" << command);
    checkProcess.waitForFinished(5000);

    if (checkProcess.state() != QProcess::NotRunning)
        checkProcess.kill();

    if (checkProcess.exitStatus() != QProcess::NormalExit || checkProcess.exitCode() != 0) {
#ifdef Q_OS_MACOS
        QMessageBox::warning(this, "错误",
                             "未检测到有效的 ROS 环境。\n\n"
                             "roslaunch / rosbag 等 ROS1 工具官方不支持 macOS，"
                             "「离线建图」相关功能需要在 Linux 机器或 Docker 容器中使用。\n"
                             "本程序的其余功能（打开地图、绘制轨迹、属性编辑等）不受影响。");
#else
        QMessageBox::warning(this, "错误", "未检测到有效的 ROS 环境，请确认运行环境。");
#endif
        return false;
    }

    return true;
}

//func_启动命令，带日志输出
bool MainWindow::startProcessWithLog(QProcess*& process,
                                     const QString& command,
                                     const QString& logPrefix)
{
    if (process) {
        QMessageBox::warning(this, "提示", logPrefix + " 已在运行！");
        return false;
    }

    QString logDirPath = QDir::homePath() + "/ros_logs/";
    QDir dir(logDirPath);
    if (!dir.exists()) dir.mkpath(".");

    QString logFilePathOut = logDirPath + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_" + logPrefix + ".log";

    process = new QProcess(this);
    QFile* logFile = new QFile(logFilePathOut, process);  // 自动清理

    bool logEnabled = false;
    if (logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        process->setStandardOutputFile(logFilePathOut);
        process->setStandardErrorFile(logFilePathOut, QIODevice::Append);
        logEnabled = true;
    }

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, logFile, logPrefix, &process](int exitCode, QProcess::ExitStatus) {
                if (logFile->isOpen()) logFile->close();
                logFile->deleteLater();
                process->deleteLater();
                process = nullptr;

                QString msg = QString("%1 结束，退出码：%2").arg(logPrefix).arg(exitCode);
                QMessageBox::information(this, "运行结束", msg);
            });

    connect(process, &QProcess::errorOccurred,
            this, [this, logFile, logPrefix, &process](QProcess::ProcessError) {
                if (logFile->isOpen()) logFile->close();
                logFile->deleteLater();
                process->deleteLater();
                process = nullptr;

                QMessageBox::critical(this, "错误", logPrefix + " 启动失败！\n详细信息：" + process->errorString());
            });

    QString fullCommand = bashRcPreamble() + command;
    process->start("bash", QStringList() << "-c" << fullCommand);

    if (!process->waitForStarted(3000)) {
        if (logFile->isOpen()) logFile->close();
        logFile->deleteLater();
        process->deleteLater();
        process = nullptr;
        QMessageBox::critical(this, "错误", logPrefix + " 启动超时！");
        return false;
    }

    if (logEnabled) {
        QMessageBox::information(this, "启动中",
                                 logPrefix + " 启动中，日志保存至：\n" + logFilePathOut);
    } else {
        QMessageBox::information(this, "启动中",
                                 logPrefix + " 启动中，但日志未能写入文件。\n日志目录：" + logFilePathOut);
    }
    return true;
}

//func_结束进程
void MainWindow::killProcess(QProcess*& process, const QString& processName)
{
    if (process && process->state() == QProcess::Running) {
        process->terminate();
        if (!process->waitForFinished(5000)) {
            process->kill();
            if(process->waitForFinished(3000))  // 等待 kill 生效
                QMessageBox::information(this, "已强制退出", processName + " 进程已强制退出。");
            else
                QMessageBox::information(this, "错误", processName + " 已下发kill命令，但还未退出。");
        }

        // ❗不要 deleteLater 或置空，让 startProcessWithLog 的 finished 槽来处理
        QMessageBox::information(this, "已终止", processName + " 进程已终止。");
    } else {
        QMessageBox::information(this, "提示", processName + " 未在运行。");
    }
}

//离线建图-启动建图节点
void MainWindow::on_do_roslaunch_lslidar_mapping_triggered()
{
    if (!isROSEnvironmentReady()) return;
    startProcessWithLog(mappingProcess, "roslaunch mapping_lslidar.launch", "lslidar_mapping");
}

//离线建图-播放激光雷达数据
void MainWindow::on_do_rosbag_play_triggered()
{
    if (!isROSEnvironmentReady()) return;

    // 1. 选择 rosbag 文件（可选）
    QString bagFilePath = QFileDialog::getOpenFileName(this, "选择要播放的 rosbag 文件", QDir::homePath(), "*.bag");
    if (bagFilePath.isEmpty()) return;

    // 2. 调出配置对话框
    RosbagPlayDialog dlg(bagFilePath, this);
    if (dlg.exec() != QDialog::Accepted) return;

    auto topics = dlg.selectedTopics();
    if (topics.isEmpty()) {
        QMessageBox::warning(this, "提示", "请至少选择一个 topic");
        return;
    }
    double rate = dlg.playRate();

    // 3. 构造 rosbag play 命令
    QString cmd = QString("rosbag play \"%1\"").arg(bagFilePath);
    for (const auto &t : topics) {
        cmd += QString(" --topic %1").arg(t);
    }
    cmd += QString(" -r %1").arg(rate);

    startProcessWithLog(rosbagPlayProcess, cmd, "rosbag_play");

}

//离线建图-保存地图文件
void MainWindow::on_do_save_mapping_file_triggered()
{
    if (!isROSEnvironmentReady()) return;

    // 1. 选择目标文件，格式不限
    QString filePath = QFileDialog::getOpenFileName(this, "选择保存目标文件", QDir::homePath(), "*.*");
    if (filePath.isEmpty()) return;

    // 2. 提取路径与文件名
    QFileInfo fileInfo(filePath);
    QString dirPath = fileInfo.absolutePath();
    QString baseName = fileInfo.completeBaseName(); // 不带扩展名的文件名

    // 3. 构造 rosservice 命令
    QString rosCmd = QString("rosservice call /save_map \"request_cmd: 2\nmap_save_path: '%1'\nmap_save_name: '%2'\"")
                         .arg(dirPath, baseName);

    startProcessWithLog(saveMapProcess, rosCmd, "save_map");

}

//离线建图-结束建图与rosbag节点
void MainWindow::on_do_kill_mapping_and_rosbag_Node_triggered()
{
    killProcess(mappingProcess, "lslidar_mapping");
    killProcess(rosbagPlayProcess, "rosbag_play");
}

