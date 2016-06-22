/****************************************************************************
** Meta object code from reading C++ file 'qsc.hpp'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "qsc.hpp"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qsc.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data__QScintilla[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      12,   32,   32,   32, 0x08,

       0        // eod
};

static const char qt_meta_stringdata__QScintilla[] = {
    "_QScintilla\0slot_text_changed()\0\0"
};

void _QScintilla::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        _QScintilla *_t = static_cast<_QScintilla *>(_o);
        switch (_id) {
        case 0: _t->slot_text_changed(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData _QScintilla::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject _QScintilla::staticMetaObject = {
    { &QsciScintilla::staticMetaObject, qt_meta_stringdata__QScintilla,
      qt_meta_data__QScintilla, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &_QScintilla::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *_QScintilla::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *_QScintilla::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata__QScintilla))
        return static_cast<void*>(const_cast< _QScintilla*>(this));
    return QsciScintilla::qt_metacast(_clname);
}

int _QScintilla::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QsciScintilla::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
