QT += core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MapTool
TEMPLATE = app
CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS

# 通用搜索路径（跨平台）
INCLUDEPATH += ./ \
            dialog/ \
            items/ \
            tools/ \
            control/ \
            mainwindow/

# ---------- Windows：沿用原来的本地编译路径 ----------
win32 {
    INCLUDEPATH += D:/Development_Tools/opencv/opencv-build/install/include \
                   D:/Development_Tools/opencv/opencv-build/install/include/opencv2 \
                   D:/Development_Tools/YAML_CPP/include

    LIBS += -L D:/Development_Tools/opencv/opencv-build/lib/libopencv_*.a
    LIBS += -L D:/Development_Tools/YAML_CPP/lib/libyaml-cpp.dll.a

    RC_FILE = res/ico.rc
}

# ---------- macOS / Linux：走 pkg-config（Homebrew、apt 装的 opencv4、yaml-cpp） ----------
macx|unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4 yaml-cpp

    # Homebrew 的 keg-only 或备用路径兜底
    INCLUDEPATH += /opt/homebrew/include /usr/local/include
    LIBS += -L/opt/homebrew/lib -L/usr/local/lib
}

SOURCES += \
    control/pathSimulate.cpp \
    dialog/attributeAddDialog.cpp \
    dialog/copypointsdialog.cpp \
    dialog/coverPathDialog.cpp \
    dialog/coverPath_rtk_Dialog.cpp \
    dialog/drawLine_PointsSelectedBased_Dialog.cpp \
    dialog/rosbagplaydialog.cpp \
    dialog/setFunctionalAreaName.cpp \
    items/Graph.cpp \
    items/GraphicsType.cpp \
    dialog/aboutdialog.cpp \
    items/ParkingLotItem.cpp \
    items/erasePolygon.cpp \
    items/test.cpp \
    mainwindow/graphicsview.cpp \
    main.cpp \
    mainwindow/mainwindow.cpp \
    mainwindow/fov_layer.cpp \
    ../../maptool-sim/core/grid_projection.cpp \
    ../../maptool-sim/core/fov_raycast.cpp \
    items/mapSelectItem.cpp \
    items/startRect.cpp

HEADERS += \
    control/pathSimulate.h \
    dialog/attributeAddDialog.h \
    dialog/copypointsdialog.h \
    dialog/coverPathDialog.h \
    dialog/coverPath_rtk_Dialog.h \
    dialog/drawLine_PointsSelectedBased_Dialog.h \
    dialog/rosbagplaydialog.h \
    dialog/setFunctionalAreaName.h \
    items/Graph.h \
    items/GraphicsType.h \
    dialog/aboutdialog.h \
    items/ParkingLotItem.h \
    items/erasePolygon.h \
    items/test.h \
    mainwindow/graphicsview.h \
    mainwindow/mainwindow.h \
    mainwindow/fov_layer.h \
    items/mapSelectItem.h \
    items/startRect.h \
    tools/UTF8ToGBEx.h \
    tools/qtcompat.h

FORMS += \
    dialog/aboutdialog.ui \
    dialog/attributeAddDialog.ui \
    dialog/copypointsdialog.ui \
    dialog/coverPathDialog.ui \
    dialog/coverPath_rtk_Dialog.ui \
    dialog/drawLine_PointsSelectedBased_Dialog.ui \
    dialog/setFunctionalAreaName.ui \
    mainwindow/mainwindow.ui

RESOURCES += \
    res/ImageDir.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
