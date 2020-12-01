TARGET = QtCore5Compat
QT     = core
MODULE = core5compat

QT_FOR_CONFIG += core-private
QT_FOR_PRIVATE += core-private

QMAKE_DOCS = $$PWD/doc/qtcore5compat.qdocconf

include(codecs/codecs.pri)
include(sax/sax.pri)
include(serialization/serialization.pri)
include(text/text.pri)
include(tools/tools.pri)

PUBLIC_HEADERS += \
    qcore5global.h

PRIVATE_HEADERS +=

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

load(qt_module)
QMAKE_DOCS_TARGETDIR = qtcore5compat
