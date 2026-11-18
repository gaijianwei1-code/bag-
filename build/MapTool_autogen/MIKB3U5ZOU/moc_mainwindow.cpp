/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../maptool-sim/mac-tool-src/mainwindow/mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.12.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "cleanLastSCP",
        "",
        "drawVirtualWall",
        "QList<QPointF>",
        "receiveCoverPathValue",
        "QPointF",
        "show_smooth_coverPathEdges",
        "drawWhitePolygon",
        "printAttribute",
        "std::string",
        "setAttributeData_area",
        "setAttributeData_line",
        "selectingStartPoint",
        "receiveCoverPath_rtk_Value",
        "selectingCopyPointsBasePoint",
        "selectingCopyPointsOffsetPoint",
        "copyPoints_insertNext",
        "movePoints_pointSelected",
        "drawLine_pointsSelectedBased",
        "on_doOpen_triggered",
        "on_doOpenJson_triggered",
        "on_doClose_triggered",
        "on_doSave_triggered",
        "on_doSaveJson_triggered",
        "on_doSaveJson_unfilled_triggered",
        "on_doSave_As_triggered",
        "on_doExit_triggered",
        "on_doRestore_triggered",
        "on_do_adjust_triggered",
        "on_doAbout_triggered",
        "on_do_AutoPS_wec_triggered",
        "on_doCleanJson_triggered",
        "on_actiontest_triggered",
        "on_do_draw_path_triggered",
        "on_do_fill_path_triggered",
        "on_doOpenYaml_triggered",
        "on_doDrawLine_triggered",
        "on_doDrawTraj_png_triggered",
        "on_do_draw_polygons_area_triggered",
        "on_do_draw_ploygons_triggered",
        "on_do_adjust_polygons_area_triggered",
        "on_do_draw_full_covered_path_triggered",
        "on_do_save_polygons_area_triggered",
        "on_do_load_polygons_area_triggered",
        "on_do_clean_polygons_area_triggered",
        "on_doDrawWhitePolygon_triggered",
        "on_doDrawPathContinue_triggered",
        "on_do_rtk_full_cover_path_triggered",
        "on_doOpenJsonExclusively_triggered",
        "on_do_selectMultiPoint_polygen_triggered",
        "on_do_clean_pointSelect_area_triggered",
        "on_do_addAttribute_triggered",
        "on_do_selectMultiPoint_pointSelect_triggered",
        "on_do_clean_pointSelect_pointSelect_triggered",
        "on_do_addAttribute_pointSelect_triggered",
        "on_doCleanAllMPath_triggered",
        "on_do_draw_next_mpath_triggered",
        "on_doOpenJson_multiMPath_triggered",
        "on_do_draw_functional_area_triggered",
        "on_do_clean_current_functional_area_triggered",
        "on_do_clean_all_functional_area_triggered",
        "on_do_draw_next_functional_area_triggered",
        "on_do_save_functional_area_triggered",
        "on_do_load_functional_area_triggered",
        "on_do_copyPoints_pointSelect_triggered",
        "on_do_deletePoints_pointSelect_triggered",
        "on_do_movePoints_pointSelect_triggered",
        "on_do_drawLine_pointsSelectedBased_triggered",
        "on_do_modifyPGMSize_triggered",
        "on_do_draw_parkingLot_area_triggered",
        "on_do_save_parkingLot_area_triggered",
        "on_do_saveYaml_forRTK_triggered",
        "on_do_load_parkingLot_area_triggered",
        "on_do_roslaunch_lslidar_mapping_triggered",
        "on_do_rosbag_play_triggered",
        "on_do_save_mapping_file_triggered",
        "on_do_kill_mapping_and_rosbag_Node_triggered"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'cleanLastSCP'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'drawVirtualWall'
        QtMocHelpers::SlotData<void(QList<QPointF>)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 2 },
        }}),
        // Slot 'receiveCoverPathValue'
        QtMocHelpers::SlotData<void(int, double, int, QPointF, qreal)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Double, 2 }, { QMetaType::Int, 2 }, { 0x80000000 | 6, 2 },
            { QMetaType::QReal, 2 },
        }}),
        // Slot 'show_smooth_coverPathEdges'
        QtMocHelpers::SlotData<void(int, double)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Double, 2 },
        }}),
        // Slot 'drawWhitePolygon'
        QtMocHelpers::SlotData<void(QList<QPointF>)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 2 },
        }}),
        // Slot 'printAttribute'
        QtMocHelpers::SlotData<void(std::string)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 2 },
        }}),
        // Slot 'setAttributeData_area'
        QtMocHelpers::SlotData<void(std::string)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 2 },
        }}),
        // Slot 'setAttributeData_line'
        QtMocHelpers::SlotData<void(std::string)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 2 },
        }}),
        // Slot 'selectingStartPoint'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'receiveCoverPath_rtk_Value'
        QtMocHelpers::SlotData<void(int, double, int, int, int)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Double, 2 }, { QMetaType::Int, 2 }, { QMetaType::Int, 2 },
            { QMetaType::Int, 2 },
        }}),
        // Slot 'selectingCopyPointsBasePoint'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'selectingCopyPointsOffsetPoint'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'copyPoints_insertNext'
        QtMocHelpers::SlotData<void(QPointF)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 2 },
        }}),
        // Slot 'movePoints_pointSelected'
        QtMocHelpers::SlotData<void(QPointF)>(18, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 2 },
        }}),
        // Slot 'drawLine_pointsSelectedBased'
        QtMocHelpers::SlotData<void(int, int, int, bool, int)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 }, { QMetaType::Int, 2 }, { QMetaType::Bool, 2 },
            { QMetaType::Int, 2 },
        }}),
        // Slot 'on_doOpen_triggered'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doOpenJson_triggered'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doClose_triggered'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doSave_triggered'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doSaveJson_triggered'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doSaveJson_unfilled_triggered'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doSave_As_triggered'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doExit_triggered'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doRestore_triggered'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_adjust_triggered'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doAbout_triggered'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_AutoPS_wec_triggered'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doCleanJson_triggered'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_actiontest_triggered'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_draw_path_triggered'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_fill_path_triggered'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doOpenYaml_triggered'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doDrawLine_triggered'
        QtMocHelpers::SlotData<void()>(37, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doDrawTraj_png_triggered'
        QtMocHelpers::SlotData<void()>(38, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_draw_polygons_area_triggered'
        QtMocHelpers::SlotData<void()>(39, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_draw_ploygons_triggered'
        QtMocHelpers::SlotData<void()>(40, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_adjust_polygons_area_triggered'
        QtMocHelpers::SlotData<void()>(41, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_draw_full_covered_path_triggered'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_save_polygons_area_triggered'
        QtMocHelpers::SlotData<void()>(43, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_load_polygons_area_triggered'
        QtMocHelpers::SlotData<void()>(44, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_clean_polygons_area_triggered'
        QtMocHelpers::SlotData<void()>(45, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doDrawWhitePolygon_triggered'
        QtMocHelpers::SlotData<void()>(46, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doDrawPathContinue_triggered'
        QtMocHelpers::SlotData<void()>(47, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_rtk_full_cover_path_triggered'
        QtMocHelpers::SlotData<void()>(48, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doOpenJsonExclusively_triggered'
        QtMocHelpers::SlotData<void()>(49, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_selectMultiPoint_polygen_triggered'
        QtMocHelpers::SlotData<void()>(50, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_clean_pointSelect_area_triggered'
        QtMocHelpers::SlotData<void()>(51, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_addAttribute_triggered'
        QtMocHelpers::SlotData<void()>(52, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_selectMultiPoint_pointSelect_triggered'
        QtMocHelpers::SlotData<void()>(53, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_clean_pointSelect_pointSelect_triggered'
        QtMocHelpers::SlotData<void()>(54, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_addAttribute_pointSelect_triggered'
        QtMocHelpers::SlotData<void()>(55, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doCleanAllMPath_triggered'
        QtMocHelpers::SlotData<void()>(56, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_draw_next_mpath_triggered'
        QtMocHelpers::SlotData<void()>(57, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_doOpenJson_multiMPath_triggered'
        QtMocHelpers::SlotData<void()>(58, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_draw_functional_area_triggered'
        QtMocHelpers::SlotData<void()>(59, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_clean_current_functional_area_triggered'
        QtMocHelpers::SlotData<void()>(60, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_clean_all_functional_area_triggered'
        QtMocHelpers::SlotData<void()>(61, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_draw_next_functional_area_triggered'
        QtMocHelpers::SlotData<void()>(62, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_save_functional_area_triggered'
        QtMocHelpers::SlotData<void()>(63, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_load_functional_area_triggered'
        QtMocHelpers::SlotData<void()>(64, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_copyPoints_pointSelect_triggered'
        QtMocHelpers::SlotData<void()>(65, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_deletePoints_pointSelect_triggered'
        QtMocHelpers::SlotData<void()>(66, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_movePoints_pointSelect_triggered'
        QtMocHelpers::SlotData<void()>(67, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_drawLine_pointsSelectedBased_triggered'
        QtMocHelpers::SlotData<void()>(68, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_modifyPGMSize_triggered'
        QtMocHelpers::SlotData<void()>(69, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_draw_parkingLot_area_triggered'
        QtMocHelpers::SlotData<void()>(70, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_save_parkingLot_area_triggered'
        QtMocHelpers::SlotData<void()>(71, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_saveYaml_forRTK_triggered'
        QtMocHelpers::SlotData<void()>(72, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_load_parkingLot_area_triggered'
        QtMocHelpers::SlotData<void()>(73, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_roslaunch_lslidar_mapping_triggered'
        QtMocHelpers::SlotData<void()>(74, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_rosbag_play_triggered'
        QtMocHelpers::SlotData<void()>(75, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_save_mapping_file_triggered'
        QtMocHelpers::SlotData<void()>(76, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_do_kill_mapping_and_rosbag_Node_triggered'
        QtMocHelpers::SlotData<void()>(77, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->cleanLastSCP(); break;
        case 1: _t->drawVirtualWall((*reinterpret_cast<std::add_pointer_t<QList<QPointF>>>(_a[1]))); break;
        case 2: _t->receiveCoverPathValue((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<qreal>>(_a[5]))); break;
        case 3: _t->show_smooth_coverPathEdges((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2]))); break;
        case 4: _t->drawWhitePolygon((*reinterpret_cast<std::add_pointer_t<QList<QPointF>>>(_a[1]))); break;
        case 5: _t->printAttribute((*reinterpret_cast<std::add_pointer_t<std::string>>(_a[1]))); break;
        case 6: _t->setAttributeData_area((*reinterpret_cast<std::add_pointer_t<std::string>>(_a[1]))); break;
        case 7: _t->setAttributeData_line((*reinterpret_cast<std::add_pointer_t<std::string>>(_a[1]))); break;
        case 8: _t->selectingStartPoint(); break;
        case 9: _t->receiveCoverPath_rtk_Value((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[5]))); break;
        case 10: _t->selectingCopyPointsBasePoint(); break;
        case 11: _t->selectingCopyPointsOffsetPoint(); break;
        case 12: _t->copyPoints_insertNext((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 13: _t->movePoints_pointSelected((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 14: _t->drawLine_pointsSelectedBased((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[5]))); break;
        case 15: _t->on_doOpen_triggered(); break;
        case 16: _t->on_doOpenJson_triggered(); break;
        case 17: _t->on_doClose_triggered(); break;
        case 18: _t->on_doSave_triggered(); break;
        case 19: _t->on_doSaveJson_triggered(); break;
        case 20: _t->on_doSaveJson_unfilled_triggered(); break;
        case 21: _t->on_doSave_As_triggered(); break;
        case 22: _t->on_doExit_triggered(); break;
        case 23: _t->on_doRestore_triggered(); break;
        case 24: _t->on_do_adjust_triggered(); break;
        case 25: _t->on_doAbout_triggered(); break;
        case 26: _t->on_do_AutoPS_wec_triggered(); break;
        case 27: _t->on_doCleanJson_triggered(); break;
        case 28: _t->on_actiontest_triggered(); break;
        case 29: _t->on_do_draw_path_triggered(); break;
        case 30: _t->on_do_fill_path_triggered(); break;
        case 31: _t->on_doOpenYaml_triggered(); break;
        case 32: _t->on_doDrawLine_triggered(); break;
        case 33: _t->on_doDrawTraj_png_triggered(); break;
        case 34: _t->on_do_draw_polygons_area_triggered(); break;
        case 35: _t->on_do_draw_ploygons_triggered(); break;
        case 36: _t->on_do_adjust_polygons_area_triggered(); break;
        case 37: _t->on_do_draw_full_covered_path_triggered(); break;
        case 38: _t->on_do_save_polygons_area_triggered(); break;
        case 39: _t->on_do_load_polygons_area_triggered(); break;
        case 40: _t->on_do_clean_polygons_area_triggered(); break;
        case 41: _t->on_doDrawWhitePolygon_triggered(); break;
        case 42: _t->on_doDrawPathContinue_triggered(); break;
        case 43: _t->on_do_rtk_full_cover_path_triggered(); break;
        case 44: _t->on_doOpenJsonExclusively_triggered(); break;
        case 45: _t->on_do_selectMultiPoint_polygen_triggered(); break;
        case 46: _t->on_do_clean_pointSelect_area_triggered(); break;
        case 47: _t->on_do_addAttribute_triggered(); break;
        case 48: _t->on_do_selectMultiPoint_pointSelect_triggered(); break;
        case 49: _t->on_do_clean_pointSelect_pointSelect_triggered(); break;
        case 50: _t->on_do_addAttribute_pointSelect_triggered(); break;
        case 51: _t->on_doCleanAllMPath_triggered(); break;
        case 52: _t->on_do_draw_next_mpath_triggered(); break;
        case 53: _t->on_doOpenJson_multiMPath_triggered(); break;
        case 54: _t->on_do_draw_functional_area_triggered(); break;
        case 55: _t->on_do_clean_current_functional_area_triggered(); break;
        case 56: _t->on_do_clean_all_functional_area_triggered(); break;
        case 57: _t->on_do_draw_next_functional_area_triggered(); break;
        case 58: _t->on_do_save_functional_area_triggered(); break;
        case 59: _t->on_do_load_functional_area_triggered(); break;
        case 60: _t->on_do_copyPoints_pointSelect_triggered(); break;
        case 61: _t->on_do_deletePoints_pointSelect_triggered(); break;
        case 62: _t->on_do_movePoints_pointSelect_triggered(); break;
        case 63: _t->on_do_drawLine_pointsSelectedBased_triggered(); break;
        case 64: _t->on_do_modifyPGMSize_triggered(); break;
        case 65: _t->on_do_draw_parkingLot_area_triggered(); break;
        case 66: _t->on_do_save_parkingLot_area_triggered(); break;
        case 67: _t->on_do_saveYaml_forRTK_triggered(); break;
        case 68: _t->on_do_load_parkingLot_area_triggered(); break;
        case 69: _t->on_do_roslaunch_lslidar_mapping_triggered(); break;
        case 70: _t->on_do_rosbag_play_triggered(); break;
        case 71: _t->on_do_save_mapping_file_triggered(); break;
        case 72: _t->on_do_kill_mapping_and_rosbag_Node_triggered(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MainWindow::*)()>(_a, &MainWindow::cleanLastSCP, 0))
            return;
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 73)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 73;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 73)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 73;
    }
    return _id;
}

// SIGNAL 0
void MainWindow::cleanLastSCP()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
