/****************************************************************************
** Meta object code from reading C++ file 'copypointsdialog.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../maptool-sim/mac-tool-src/dialog/copypointsdialog.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'copypointsdialog.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16copyPointsDialogE_t {};
} // unnamed namespace

template <> constexpr inline auto copyPointsDialog::qt_create_metaobjectdata<qt_meta_tag_ZN16copyPointsDialogE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "copyPointsDialog",
        "selectBasePoint",
        "",
        "selectOffsetPoint",
        "apply_pointsOffset_copy",
        "QPointF",
        "setBasePoint",
        "setOffsetPoint",
        "on_pushButton_first_clicked",
        "on_pushButton_second_clicked",
        "on_pushButton_apply_copy_clicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'selectBasePoint'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'selectOffsetPoint'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'apply_pointsOffset_copy'
        QtMocHelpers::SignalData<void(QPointF)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 2 },
        }}),
        // Slot 'setBasePoint'
        QtMocHelpers::SlotData<void(QPointF)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 2 },
        }}),
        // Slot 'setOffsetPoint'
        QtMocHelpers::SlotData<void(QPointF)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 5, 2 },
        }}),
        // Slot 'on_pushButton_first_clicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_pushButton_second_clicked'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_pushButton_apply_copy_clicked'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<copyPointsDialog, qt_meta_tag_ZN16copyPointsDialogE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject copyPointsDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16copyPointsDialogE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16copyPointsDialogE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16copyPointsDialogE_t>.metaTypes,
    nullptr
} };

void copyPointsDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<copyPointsDialog *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->selectBasePoint(); break;
        case 1: _t->selectOffsetPoint(); break;
        case 2: _t->apply_pointsOffset_copy((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 3: _t->setBasePoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 4: _t->setOffsetPoint((*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[1]))); break;
        case 5: _t->on_pushButton_first_clicked(); break;
        case 6: _t->on_pushButton_second_clicked(); break;
        case 7: _t->on_pushButton_apply_copy_clicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (copyPointsDialog::*)()>(_a, &copyPointsDialog::selectBasePoint, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (copyPointsDialog::*)()>(_a, &copyPointsDialog::selectOffsetPoint, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (copyPointsDialog::*)(QPointF )>(_a, &copyPointsDialog::apply_pointsOffset_copy, 2))
            return;
    }
}

const QMetaObject *copyPointsDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *copyPointsDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16copyPointsDialogE_t>.strings))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int copyPointsDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void copyPointsDialog::selectBasePoint()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void copyPointsDialog::selectOffsetPoint()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void copyPointsDialog::apply_pointsOffset_copy(QPointF _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}
QT_WARNING_POP
