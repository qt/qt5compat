TARGET = QtCore5Compat
QT     = core
MODULE = core5compat

QT_FOR_CONFIG += core-private
QT_FOR_PRIVATE += core-private

QMAKE_DOCS = $$PWD/doc/qtcore5.qdocconf

include(codecs/codecs.pri)

PUBLIC_HEADERS += \
    qcore5global.h \
    qlinkedlist.h

PRIVATE_HEADERS +=

SOURCES += \
    qlinkedlist.cpp

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

load(qt_module)
