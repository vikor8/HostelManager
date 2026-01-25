/****************************************************************************
** Meta object code from reading C++ file 'hostelmanager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../HostelManager/hostelmanager.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'hostelmanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_HostelManager_t {
    uint offsetsAndSizes[34];
    char stringdata0[14];
    char stringdata1[20];
    char stringdata2[1];
    char stringdata3[22];
    char stringdata4[24];
    char stringdata5[5];
    char stringdata6[16];
    char stringdata7[19];
    char stringdata8[10];
    char stringdata9[11];
    char stringdata10[13];
    char stringdata11[19];
    char stringdata12[15];
    char stringdata13[14];
    char stringdata14[12];
    char stringdata15[13];
    char stringdata16[15];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_HostelManager_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_HostelManager_t qt_meta_stringdata_HostelManager = {
    {
        QT_MOC_LITERAL(0, 13),  // "HostelManager"
        QT_MOC_LITERAL(14, 19),  // "on_btnToday_clicked"
        QT_MOC_LITERAL(34, 0),  // ""
        QT_MOC_LITERAL(35, 21),  // "on_btnRefresh_clicked"
        QT_MOC_LITERAL(57, 23),  // "on_dateEdit_dateChanged"
        QT_MOC_LITERAL(81, 4),  // "date"
        QT_MOC_LITERAL(86, 15),  // "initializeTable"
        QT_MOC_LITERAL(102, 18),  // "updateTableHeaders"
        QT_MOC_LITERAL(121, 9),  // "onAddRoom"
        QT_MOC_LITERAL(131, 10),  // "onEditRoom"
        QT_MOC_LITERAL(142, 12),  // "onDeleteRoom"
        QT_MOC_LITERAL(155, 18),  // "onManageCategories"
        QT_MOC_LITERAL(174, 14),  // "loadCategories"
        QT_MOC_LITERAL(189, 13),  // "onViewClients"
        QT_MOC_LITERAL(203, 11),  // "onAddClient"
        QT_MOC_LITERAL(215, 12),  // "onEditClient"
        QT_MOC_LITERAL(228, 14)   // "onDeleteClient"
    },
    "HostelManager",
    "on_btnToday_clicked",
    "",
    "on_btnRefresh_clicked",
    "on_dateEdit_dateChanged",
    "date",
    "initializeTable",
    "updateTableHeaders",
    "onAddRoom",
    "onEditRoom",
    "onDeleteRoom",
    "onManageCategories",
    "loadCategories",
    "onViewClients",
    "onAddClient",
    "onEditClient",
    "onDeleteClient"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_HostelManager[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   98,    2, 0x08,    1 /* Private */,
       3,    0,   99,    2, 0x08,    2 /* Private */,
       4,    1,  100,    2, 0x08,    3 /* Private */,
       6,    0,  103,    2, 0x08,    5 /* Private */,
       7,    0,  104,    2, 0x08,    6 /* Private */,
       8,    0,  105,    2, 0x08,    7 /* Private */,
       9,    0,  106,    2, 0x08,    8 /* Private */,
      10,    0,  107,    2, 0x08,    9 /* Private */,
      11,    0,  108,    2, 0x08,   10 /* Private */,
      12,    0,  109,    2, 0x08,   11 /* Private */,
      13,    0,  110,    2, 0x08,   12 /* Private */,
      14,    0,  111,    2, 0x08,   13 /* Private */,
      15,    0,  112,    2, 0x08,   14 /* Private */,
      16,    0,  113,    2, 0x08,   15 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QDate,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject HostelManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_HostelManager.offsetsAndSizes,
    qt_meta_data_HostelManager,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_HostelManager_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<HostelManager, std::true_type>,
        // method 'on_btnToday_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_btnRefresh_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_dateEdit_dateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QDate &, std::false_type>,
        // method 'initializeTable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateTableHeaders'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAddRoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onEditRoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDeleteRoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onManageCategories'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadCategories'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onViewClients'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAddClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onEditClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDeleteClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void HostelManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<HostelManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->on_btnToday_clicked(); break;
        case 1: _t->on_btnRefresh_clicked(); break;
        case 2: _t->on_dateEdit_dateChanged((*reinterpret_cast< std::add_pointer_t<QDate>>(_a[1]))); break;
        case 3: _t->initializeTable(); break;
        case 4: _t->updateTableHeaders(); break;
        case 5: _t->onAddRoom(); break;
        case 6: _t->onEditRoom(); break;
        case 7: _t->onDeleteRoom(); break;
        case 8: _t->onManageCategories(); break;
        case 9: _t->loadCategories(); break;
        case 10: _t->onViewClients(); break;
        case 11: _t->onAddClient(); break;
        case 12: _t->onEditClient(); break;
        case 13: _t->onDeleteClient(); break;
        default: ;
        }
    }
}

const QMetaObject *HostelManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *HostelManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_HostelManager.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int HostelManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
