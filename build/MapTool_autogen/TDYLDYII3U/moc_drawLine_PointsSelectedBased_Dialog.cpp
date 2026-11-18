/****************************************************************************
** Meta object code from reading C++ file 'drawLine_PointsSelectedBased_Dialog.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../maptool-sim/mac-tool-src/dialog/drawLine_PointsSelectedBased_Dialog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'drawLine_PointsSelectedBased_Dialog.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN35DrawLine_PointsSelectedBased_DialogE_t {};
} // unnamed namespace

template <> constexpr inline auto DrawLine_PointsSelectedBased_Dialog::qt_create_metaobjectdata<qt_meta_tag_ZN35DrawLine_PointsSelectedBased_DialogE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DrawLine_PointsSelectedBased_Dialog",
        "drawLine_PointsSelectedBased_signal",
        "",
        "on_spinBox_offsetDistance_valueChanged",
        "arg1",
        "on_spinBox_extendLength_valueChanged",
        "on_spinBox_lineWidth_valueChanged",
        "on_buttonBox_accepted"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'drawLine_PointsSelectedBased_signal'
        QtMocHelpers::SignalData<void(int, int, int, bool, int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Int, 2 }, { QMetaType::Int, 2 }, { QMetaType::Bool, 2 },
            { QMetaType::Int, 2 },
        }}),
        // Slot 'on_spinBox_offsetDistance_valueChanged'
        QtMocHelpers::SlotData<void(int)>(3, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 4 },
        }}),
        // Slot 'on_spinBox_extendLength_valueChanged'
        QtMocHelpers::SlotData<void(int)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 4 },
        }}),
        // Slot 'on_spinBox_lineWidth_valueChanged'
        QtMocHelpers::SlotData<void(int)>(6, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 4 },
        }}),
        // Slot 'on_buttonBox_accepted'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DrawLine_PointsSelectedBased_Dialog, qt_meta_tag_ZN35DrawLine_PointsSelectedBased_DialogE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DrawLine_PointsSelectedBased_Dialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN35DrawLine_PointsSelectedBased_DialogE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN35DrawLine_PointsSelectedBased_DialogE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN35DrawLine_PointsSelectedBased_DialogE_t>.metaTypes,
    nullptr
} };

void DrawLine_PointsSelectedBased_Dialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DrawLine_PointsSelectedBased_Dialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->drawLine_PointsSelectedBased_signal((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[5]))); break;
        case 1: _t->on_spinBox_offsetDistance_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->on_spinBox_extendLength_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->on_spinBox_lineWidth_valueChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->on_buttonBox_accepted(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DrawLine_PointsSelectedBased_Dialog::*)(int , int , int , bool , int )>(_a, &DrawLine_PointsSelectedBased_Dialog::drawLine_PointsSelectedBased_signal, 0))
            return;
    }
}

const QMetaObject *DrawLine_PointsSelectedBased_Dialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DrawLine_PointsSelectedBased_Dialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN35DrawLine_PointsSelectedBased_DialogE_t>.strings))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int DrawLine_PointsSelectedBased_Dialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void DrawLine_PointsSelectedBased_Dialog::drawLine_PointsSelectedBased_signal(int _t1, int _t2, int _t3, bool _t4, int _t5)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2, _t3, _t4, _t5);
}
QT_WARNING_POP
