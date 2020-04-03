TARGET = QtCore5Compat
QT     = core
MODULE = core5compat

QMAKE_DOCS = $$PWD/doc/qtcore5.qdocconf

PUBLIC_HEADERS += \
    qcore5global.h \
    qlinkedlist.h

PRIVATE_HEADERS +=

SOURCES += \
    qlinkedlist.cpp

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

load(qt_module)
