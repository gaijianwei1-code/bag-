/****************************************************************************
** Meta object code from reading C++ file 'GraphicsType.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../maptool-sim/mac-tool-src/items/GraphicsType.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GraphicsType.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10BPointItemE_t {};
} // unnamed namespace

template <> constexpr inline auto BPointItem::qt_create_metaobjectdata<qt_meta_tag_ZN10BPointItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "BPointItem",
        "del_Point",
        "",
        "BPointItem*",
        "add_Next_Point",
        "set_Point_Attribute",
        "setAttributeData",
        "std::string"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'del_Point'
        QtMocHelpers::SignalData<void(BPointItem *)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'add_Next_Point'
        QtMocHelpers::SignalData<void(BPointItem *, qreal)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { QMetaType::QReal, 2 },
        }}),
        // Slot 'set_Point_Attribute'
        QtMocHelpers::SlotData<void(BPointItem *)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'setAttributeData'
        QtMocHelpers::SlotData<void(std::string)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<BPointItem, qt_meta_tag_ZN10BPointItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject BPointItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10BPointItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10BPointItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10BPointItemE_t>.metaTypes,
    nullptr
} };

void BPointItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<BPointItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->del_Point((*reinterpret_cast<std::add_pointer_t<BPointItem*>>(_a[1]))); break;
        case 1: _t->add_Next_Point((*reinterpret_cast<std::add_pointer_t<BPointItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<qreal>>(_a[2]))); break;
        case 2: _t->set_Point_Attribute((*reinterpret_cast<std::add_pointer_t<BPointItem*>>(_a[1]))); break;
        case 3: _t->setAttributeData((*reinterpret_cast<std::add_pointer_t<std::string>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< BPointItem* >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< BPointItem* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< BPointItem* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (BPointItem::*)(BPointItem * )>(_a, &BPointItem::del_Point, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (BPointItem::*)(BPointItem * , qreal )>(_a, &BPointItem::add_Next_Point, 1))
            return;
    }
}

const QMetaObject *BPointItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BPointItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10BPointItemE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QAbstractGraphicsShapeItem"))
        return static_cast< QAbstractGraphicsShapeItem*>(this);
    return QObject::qt_metacast(_clname);
}

int BPointItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void BPointItem::del_Point(BPointItem * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void BPointItem::add_Next_Point(BPointItem * _t1, qreal _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}
namespace {
struct qt_meta_tag_ZN9BPathItemE_t {};
} // unnamed namespace

template <> constexpr inline auto BPathItem::qt_create_metaobjectdata<qt_meta_tag_ZN9BPathItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "BPathItem",
        "pushPoint",
        "",
        "QPointF",
        "p",
        "QList<QPointF>",
        "list",
        "isFinished",
        "deletePoint",
        "BPointItem*",
        "point",
        "insertPointNext"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'pushPoint'
        QtMocHelpers::SlotData<void(QPointF, QList<QPointF>, bool)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 5, 6 }, { QMetaType::Bool, 7 },
        }}),
        // Slot 'deletePoint'
        QtMocHelpers::SlotData<void(BPointItem *)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 },
        }}),
        // Slot 'insertPointNext'
        QtMocHelpers::SlotData<void(BPointItem *, qreal)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 }, { QMetaType::QReal, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<BPathItem, qt_meta_tag_ZN9BPathItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject BPathItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9BPathItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9BPathItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9BPathItemE_t>.metaTypes,
    nullptr
} };

void BPathItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<BPathItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->pushPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<QPointF>>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3]))); break;
        case 1: _t->deletePoint((*reinterpret_cast<std::add_pointer_t<BPointItem*>>(_a[1]))); break;
        case 2: _t->insertPointNext((*reinterpret_cast<std::add_pointer_t<BPointItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<qreal>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< BPointItem* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< BPointItem* >(); break;
            }
            break;
        }
    }
}

const QMetaObject *BPathItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BPathItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9BPathItemE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QAbstractGraphicsShapeItem"))
        return static_cast< QAbstractGraphicsShapeItem*>(this);
    return QObject::qt_metacast(_clname);
}

int BPathItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
namespace {
struct qt_meta_tag_ZN15BQGraphicsSceneE_t {};
} // unnamed namespace

template <> constexpr inline auto BQGraphicsScene::qt_create_metaobjectdata<qt_meta_tag_ZN15BQGraphicsSceneE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "BQGraphicsScene",
        "updatePoint",
        "",
        "QPointF",
        "p",
        "QList<QPointF>",
        "list",
        "isFinished",
        "createFinished",
        "drawLine",
        "map_ploygon_addPoint",
        "createFinished_mp",
        "curMousePos",
        "erase_ploygon_addPoint",
        "createFinished_ep",
        "pointSelect_polygon_addPoint",
        "createFinished_pp",
        "areaSelect_polygon_addPoint",
        "createFinished_ap",
        "selected_startPoint_accept",
        "selected_startPoint_reject",
        "selected_addAttribute_firstPoint",
        "QGraphicsItem*",
        "selected_addAttribute_secondPoint",
        "selected_copyPointsBasePoint",
        "selected_copyPointsOffsetPoint",
        "created_parkingLot_polygon_setStartPoint",
        "created_parkingLot_polygon_setEndPoint",
        "created_parkingLot_polygon_setWidth"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'updatePoint'
        QtMocHelpers::SignalData<void(QPointF, QList<QPointF>, bool)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 5, 6 }, { QMetaType::Bool, 7 },
        }}),
        // Signal 'createFinished'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'drawLine'
        QtMocHelpers::SignalData<void(QList<QPointF>)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 2 },
        }}),
        // Signal 'map_ploygon_addPoint'
        QtMocHelpers::SignalData<void(QPointF, bool)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::Bool, 7 },
        }}),
        // Signal 'createFinished_mp'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'curMousePos'
        QtMocHelpers::SignalData<void(QPointF)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'erase_ploygon_addPoint'
        QtMocHelpers::SignalData<void(QPointF)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'createFinished_ep'
        QtMocHelpers::SignalData<void(QList<QPointF>)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 2 },
        }}),
        // Signal 'pointSelect_polygon_addPoint'
        QtMocHelpers::SignalData<void(QPointF, bool)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::Bool, 7 },
        }}),
        // Signal 'createFinished_pp'
        QtMocHelpers::SignalData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'areaSelect_polygon_addPoint'
        QtMocHelpers::SignalData<void(QPointF, bool)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::Bool, 7 },
        }}),
        // Signal 'createFinished_ap'
        QtMocHelpers::SignalData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'selected_startPoint_accept'
        QtMocHelpers::SignalData<void(QPointF)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'selected_startPoint_reject'
        QtMocHelpers::SignalData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'selected_addAttribute_firstPoint'
        QtMocHelpers::SignalData<void(QGraphicsItem *)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 22, 2 },
        }}),
        // Signal 'selected_addAttribute_secondPoint'
        QtMocHelpers::SignalData<void(QGraphicsItem *)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 22, 2 },
        }}),
        // Signal 'selected_copyPointsBasePoint'
        QtMocHelpers::SignalData<void(QPointF)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'selected_copyPointsOffsetPoint'
        QtMocHelpers::SignalData<void(QPointF)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'created_parkingLot_polygon_setStartPoint'
        QtMocHelpers::SignalData<void(QPointF)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'created_parkingLot_polygon_setEndPoint'
        QtMocHelpers::SignalData<void(QPointF)>(27, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Signal 'created_parkingLot_polygon_setWidth'
        QtMocHelpers::SignalData<void(QPointF)>(28, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<BQGraphicsScene, qt_meta_tag_ZN15BQGraphicsSceneE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject BQGraphicsScene::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsScene::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15BQGraphicsSceneE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15BQGraphicsSceneE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15BQGraphicsSceneE_t>.metaTypes,
    nullptr
} };

void BQGraphicsScene::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<BQGraphicsScene *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->updatePoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<QPointF>>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3]))); break;
        case 1: _t->createFinished(); break;
        case 2: _t->drawLine((*reinterpret_cast<std::add_pointer_t<QList<QPointF>>>(_a[1]))); break;
        case 3: _t->map_ploygon_addPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 4: _t->createFinished_mp(); break;
        case 5: _t->curMousePos((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 6: _t->erase_ploygon_addPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 7: _t->createFinished_ep((*reinterpret_cast<std::add_pointer_t<QList<QPointF>>>(_a[1]))); break;
        case 8: _t->pointSelect_polygon_addPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 9: _t->createFinished_pp(); break;
        case 10: _t->areaSelect_polygon_addPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 11: _t->createFinished_ap(); break;
        case 12: _t->selected_startPoint_accept((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 13: _t->selected_startPoint_reject(); break;
        case 14: _t->selected_addAttribute_firstPoint((*reinterpret_cast<std::add_pointer_t<QGraphicsItem*>>(_a[1]))); break;
        case 15: _t->selected_addAttribute_secondPoint((*reinterpret_cast<std::add_pointer_t<QGraphicsItem*>>(_a[1]))); break;
        case 16: _t->selected_copyPointsBasePoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 17: _t->selected_copyPointsOffsetPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 18: _t->created_parkingLot_polygon_setStartPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 19: _t->created_parkingLot_polygon_setEndPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 20: _t->created_parkingLot_polygon_setWidth((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 14:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QGraphicsItem* >(); break;
            }
            break;
        case 15:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QGraphicsItem* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF , QList<QPointF> , bool )>(_a, &BQGraphicsScene::updatePoint, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)()>(_a, &BQGraphicsScene::createFinished, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QList<QPointF> )>(_a, &BQGraphicsScene::drawLine, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF , bool )>(_a, &BQGraphicsScene::map_ploygon_addPoint, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)()>(_a, &BQGraphicsScene::createFinished_mp, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF )>(_a, &BQGraphicsScene::curMousePos, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF )>(_a, &BQGraphicsScene::erase_ploygon_addPoint, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QList<QPointF> )>(_a, &BQGraphicsScene::createFinished_ep, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF , bool )>(_a, &BQGraphicsScene::pointSelect_polygon_addPoint, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)()>(_a, &BQGraphicsScene::createFinished_pp, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF , bool )>(_a, &BQGraphicsScene::areaSelect_polygon_addPoint, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)()>(_a, &BQGraphicsScene::createFinished_ap, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF )>(_a, &BQGraphicsScene::selected_startPoint_accept, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)()>(_a, &BQGraphicsScene::selected_startPoint_reject, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QGraphicsItem * )>(_a, &BQGraphicsScene::selected_addAttribute_firstPoint, 14))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QGraphicsItem * )>(_a, &BQGraphicsScene::selected_addAttribute_secondPoint, 15))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF )>(_a, &BQGraphicsScene::selected_copyPointsBasePoint, 16))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF )>(_a, &BQGraphicsScene::selected_copyPointsOffsetPoint, 17))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF )>(_a, &BQGraphicsScene::created_parkingLot_polygon_setStartPoint, 18))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF )>(_a, &BQGraphicsScene::created_parkingLot_polygon_setEndPoint, 19))
            return;
        if (QtMocHelpers::indexOfMethod<void (BQGraphicsScene::*)(QPointF )>(_a, &BQGraphicsScene::created_parkingLot_polygon_setWidth, 20))
            return;
    }
}

const QMetaObject *BQGraphicsScene::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BQGraphicsScene::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15BQGraphicsSceneE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsScene::qt_metacast(_clname);
}

int BQGraphicsScene::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsScene::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    return _id;
}

// SIGNAL 0
void BQGraphicsScene::updatePoint(QPointF _t1, QList<QPointF> _t2, bool _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2, _t3);
}

// SIGNAL 1
void BQGraphicsScene::createFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void BQGraphicsScene::drawLine(QList<QPointF> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void BQGraphicsScene::map_ploygon_addPoint(QPointF _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void BQGraphicsScene::createFinished_mp()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void BQGraphicsScene::curMousePos(QPointF _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void BQGraphicsScene::erase_ploygon_addPoint(QPointF _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void BQGraphicsScene::createFinished_ep(QList<QPointF> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void BQGraphicsScene::pointSelect_polygon_addPoint(QPointF _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2);
}

// SIGNAL 9
void BQGraphicsScene::createFinished_pp()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void BQGraphicsScene::areaSelect_polygon_addPoint(QPointF _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1, _t2);
}

// SIGNAL 11
void BQGraphicsScene::createFinished_ap()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void BQGraphicsScene::selected_startPoint_accept(QPointF _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1);
}

// SIGNAL 13
void BQGraphicsScene::selected_startPoint_reject()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void BQGraphicsScene::selected_addAttribute_firstPoint(QGraphicsItem * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 14, nullptr, _t1);
}

// SIGNAL 15
void BQGraphicsScene::selected_addAttribute_secondPoint(QGraphicsItem * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 15, nullptr, _t1);
}

// SIGNAL 16
void BQGraphicsScene::selected_copyPointsBasePoint(QPointF _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 16, nullptr, _t1);
}

// SIGNAL 17
void BQGraphicsScene::selected_copyPointsOffsetPoint(QPointF _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 17, nullptr, _t1);
}

// SIGNAL 18
void BQGraphicsScene::created_parkingLot_polygon_setStartPoint(QPointF _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 18, nullptr, _t1);
}

// SIGNAL 19
void BQGraphicsScene::created_parkingLot_polygon_setEndPoint(QPointF _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 19, nullptr, _t1);
}

// SIGNAL 20
void BQGraphicsScene::created_parkingLot_polygon_setWidth(QPointF _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 20, nullptr, _t1);
}
QT_WARNING_POP
