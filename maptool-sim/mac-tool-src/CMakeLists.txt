# MapTool —— 跨平台构建（Windows / macOS / Linux）
cmake_minimum_required(VERSION 3.16)

project(MapTool VERSION 1.0.0)

# ---------- C++ 标准 ----------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------- Qt 自动化处理 ----------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow ${CMAKE_CURRENT_SOURCE_DIR}/dialog)

# ---------- Qt 5.15 / 6.x 双版本支持 ----------
# 优先找 Qt6（Homebrew `brew install qt`），找不到则回退 Qt5（`brew install qt@5`）
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg)

# Qt6 在 Windows 上还需要 Core5Compat 才能用 QTextCodec（GBK 路径转换）
# macOS/Linux 不做 GBK 转换，无需该模块
if(QT_VERSION_MAJOR GREATER_EQUAL 6 AND WIN32)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
endif()

message(STATUS "Using Qt ${QT_VERSION_MAJOR} (${Qt${QT_VERSION_MAJOR}_VERSION})")

# ---------- 第三方库 ----------
# 只链接实际用到的 OpenCV 模块（core/imgproc/imgcodecs/highgui），
# 避免把全部 OpenCV 模块打进 .app（体积从 ~270MB 降到 ~50MB）
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)
find_package(yaml-cpp REQUIRED)

# ---------- 头文件搜索路径 ----------
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    dialog/
    items/
    tools/
    control/
    mainwindow/
)

# ---------- 源文件 ----------
set(SOURCES
    control/pathSimulate.cpp
    dialog/attributeAddDialog.cpp
    dialog/copypointsdialog.cpp
    dialog/coverPathDialog.cpp
    dialog/coverPath_rtk_Dialog.cpp
    dialog/drawLine_PointsSelectedBased_Dialog.cpp
    dialog/rosbagplaydialog.cpp
    dialog/setFunctionalAreaName.cpp
    items/Graph.cpp
    items/GraphicsType.cpp
    dialog/aboutdialog.cpp
    items/ParkingLotItem.cpp
    items/erasePolygon.cpp
    items/test.cpp
    mainwindow/graphicsview.cpp
    main.cpp
    mainwindow/mainwindow.cpp
    items/mapSelectItem.cpp
    items/startRect.cpp
)

set(HEADERS
    control/pathSimulate.h
    dialog/attributeAddDialog.h
    dialog/copypointsdialog.h
    dialog/coverPathDialog.h
    dialog/coverPath_rtk_Dialog.h
    dialog/drawLine_PointsSelectedBased_Dialog.h
    dialog/rosbagplaydialog.h
    dialog/setFunctionalAreaName.h
    items/Graph.h
    items/GraphicsType.h
    dialog/aboutdialog.h
    items/ParkingLotItem.h
    items/erasePolygon.h
    items/test.h
    mainwindow/graphicsview.h
    mainwindow/mainwindow.h
    items/mapSelectItem.h
    items/startRect.h
    tools/UTF8ToGBEx.h
    tools/qtcompat.h
)

set(FORMS
    dialog/aboutdialog.ui
    dialog/attributeAddDialog.ui
    dialog/copypointsdialog.ui
    dialog/coverPathDialog.ui
    dialog/coverPath_rtk_Dialog.ui
    dialog/drawLine_PointsSelectedBased_Dialog.ui
    dialog/setFunctionalAreaName.ui
    mainwindow/mainwindow.ui
)

set(RESOURCES
    res/ImageDir.qrc
)

# Windows 独有资源（.exe 图标）
if(WIN32)
    list(APPEND RESOURCES res/ico.rc)
endif()

# ---------- 可执行文件 ----------
# macOS 上打成 .app bundle（macdeployqt 打包发布时必需）
if(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES}
    )
else()
    add_executable(${PROJECT_NAME}
        ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES}
    )
endif()

# ---------- 链接 ----------
set(QT_LIBS
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
)
if(QT_VERSION_MAJOR GREATER_EQUAL 6 AND WIN32)
    list(APPEND QT_LIBS Qt6::Core5Compat)
endif()

target_link_libraries(${PROJECT_NAME}
    ${QT_LIBS}
    ${OpenCV_LIBS}
)

# yaml-cpp：新版本导出 yaml-cpp::yaml-cpp，老版本只有变量形式
if(TARGET yaml-cpp::yaml-cpp)
    target_link_libraries(${PROJECT_NAME} yaml-cpp::yaml-cpp)
elseif(TARGET yaml-cpp)
    target_link_libraries(${PROJECT_NAME} yaml-cpp)
elseif(YAML_CPP_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} ${YAML_CPP_LIBRARIES})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE QT_DEPRECATED_WARNINGS)

# ---------- 输出目录 ----------
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/bin
    MACOSX_BUNDLE_BUNDLE_NAME "Map Tool"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.techinao.maptool"
    MACOSX_BUNDLE_COPYRIGHT "Map Tool"
)

# macOS 图标（res/MapTool.icns 存在时才设置）
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/res/MapTool.icns)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_ICON_FILE MapTool.icns
    )
    set_source_files_properties(res/MapTool.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${PROJECT_NAME} PRIVATE res/MapTool.icns)
endif()
