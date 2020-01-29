TARGET = QtCore5Compat
QT     = core

PUBLIC_HEADERS += \
    qcore5global.h \
    qlinkedlist.h

PRIVATE_HEADERS +=

SOURCES += \
    qlinkedlist.cpp

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

load(qt_module)
