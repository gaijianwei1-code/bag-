/****************************************************************************
** Meta object code from reading C++ file 'coverPathDialog.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../maptool-sim/mac-tool-src/dialog/coverPathDialog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'coverPathDialog.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN15CoverPathDialogE_t {};
} // unnamed namespace

template <> constexpr inline auto CoverPathDialog::qt_create_metaobjectdata<qt_meta_tag_ZN15CoverPathDialogE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "CoverPathDialog",
        "sendData",
        "",
        "QPointF",
        "sendData_pre",
        "selectStartPoint",
        "setStartPoint",
        "on_buttonOK_accepted",
        "on_buttonOK_rejected",
        "on_pushButton_coverPath_pre_clicked",
        "on_pushButton_selectPoint_clicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'sendData'
        QtMocHelpers::SignalData<void(int, double, int, QPointF, qreal)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Double, 2 }, { QMetaType::Int, 2 }, { 0x80000000 | 3, 2 },
            { QMetaType::QReal, 2 },
        }}),
        // Signal 'sendData_pre'
        QtMocHelpers::SignalData<void(int, double)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 2 }, { QMetaType::Double, 2 },
        }}),
        // Signal 'selectStartPoint'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'setStartPoint'
        QtMocHelpers::SlotData<void(QPointF)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 2 },
        }}),
        // Slot 'on_buttonOK_accepted'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_buttonOK_rejected'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_pushButton_coverPath_pre_clicked'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_pushButton_selectPoint_clicked'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CoverPathDialog, qt_meta_tag_ZN15CoverPathDialogE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject CoverPathDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15CoverPathDialogE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15CoverPathDialogE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15CoverPathDialogE_t>.metaTypes,
    nullptr
} };

void CoverPathDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CoverPathDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->sendData((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<qreal>>(_a[5]))); break;
        case 1: _t->sendData_pre((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<double>>(_a[2]))); break;
        case 2: _t->selectStartPoint(); break;
        case 3: _t->setStartPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 4: _t->on_buttonOK_accepted(); break;
        case 5: _t->on_buttonOK_rejected(); break;
        case 6: _t->on_pushButton_coverPath_pre_clicked(); break;
        case 7: _t->on_pushButton_selectPoint_clicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CoverPathDialog::*)(int , double , int , QPointF , qreal )>(_a, &CoverPathDialog::sendData, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CoverPathDialog::*)(int , double )>(_a, &CoverPathDialog::sendData_pre, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (CoverPathDialog::*)()>(_a, &CoverPathDialog::selectStartPoint, 2))
            return;
    }
}

const QMetaObject *CoverPathDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CoverPathDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15CoverPathDialogE_t>.strings))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CoverPathDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void CoverPathDialog::sendData(int _t1, double _t2, int _t3, QPointF _t4, qreal _t5)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2, _t3, _t4, _t5);
}

// SIGNAL 1
void CoverPathDialog::sendData_pre(int _t1, double _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void CoverPathDialog::selectStartPoint()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
