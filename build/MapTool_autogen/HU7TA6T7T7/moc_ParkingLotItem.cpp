/****************************************************************************
** Meta object code from reading C++ file 'ParkingLotItem.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../maptool-sim/mac-tool-src/items/ParkingLotItem.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ParkingLotItem.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN19ParkingLotPointItemE_t {};
} // unnamed namespace

template <> constexpr inline auto ParkingLotPointItem::qt_create_metaobjectdata<qt_meta_tag_ZN19ParkingLotPointItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ParkingLotPointItem",
        "pointMoved",
        "",
        "ParkingLotPointItem*",
        "QPointF"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'pointMoved'
        QtMocHelpers::SignalData<void(ParkingLotPointItem *, QPointF)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 }, { 0x80000000 | 4, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ParkingLotPointItem, qt_meta_tag_ZN19ParkingLotPointItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ParkingLotPointItem::staticMetaObject = { {
    QMetaObject::SuperData::link<MPointItem::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19ParkingLotPointItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19ParkingLotPointItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN19ParkingLotPointItemE_t>.metaTypes,
    nullptr
} };

void ParkingLotPointItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ParkingLotPointItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->pointMoved((*reinterpret_cast<std::add_pointer_t<ParkingLotPointItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
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
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< ParkingLotPointItem* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ParkingLotPointItem::*)(ParkingLotPointItem * , QPointF )>(_a, &ParkingLotPointItem::pointMoved, 0))
            return;
    }
}

const QMetaObject *ParkingLotPointItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ParkingLotPointItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN19ParkingLotPointItemE_t>.strings))
        return static_cast<void*>(this);
    return MPointItem::qt_metacast(_clname);
}

int ParkingLotPointItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MPointItem::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void ParkingLotPointItem::pointMoved(ParkingLotPointItem * _t1, QPointF _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}
namespace {
struct qt_meta_tag_ZN14ParkingLotItemE_t {};
} // unnamed namespace

template <> constexpr inline auto ParkingLotItem::qt_create_metaobjectdata<qt_meta_tag_ZN14ParkingLotItemE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ParkingLotItem",
        "setStartPoint",
        "",
        "QPointF",
        "setEndPoint",
        "setWidth",
        "selfUpdate",
        "ParkingLotPointItem*"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'setStartPoint'
        QtMocHelpers::SlotData<void(QPointF)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'setEndPoint'
        QtMocHelpers::SlotData<void(QPointF)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'setWidth'
        QtMocHelpers::SlotData<void(QPointF)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'selfUpdate'
        QtMocHelpers::SlotData<void(ParkingLotPointItem *, QPointF)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 2 }, { 0x80000000 | 3, 2 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ParkingLotItem, qt_meta_tag_ZN14ParkingLotItemE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ParkingLotItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14ParkingLotItemE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14ParkingLotItemE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14ParkingLotItemE_t>.metaTypes,
    nullptr
} };

void ParkingLotItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ParkingLotItem *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->setStartPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 1: _t->setEndPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 2: _t->setWidth((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 3: _t->selfUpdate((*reinterpret_cast<std::add_pointer_t<ParkingLotPointItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< ParkingLotPointItem* >(); break;
            }
            break;
        }
    }
}

const QMetaObject *ParkingLotItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ParkingLotItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14ParkingLotItemE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QAbstractGraphicsShapeItem"))
        return static_cast< QAbstractGraphicsShapeItem*>(this);
    return QObject::qt_metacast(_clname);
}

int ParkingLotItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
QT_WARNING_POP
