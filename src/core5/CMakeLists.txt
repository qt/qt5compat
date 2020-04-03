# Generated from core5.pro.

#####################################################################
## Core5Compat Module:
#####################################################################

qt_add_module(Core5Compat
    SOURCES
        qcore5global.h
        qlinkedlist.cpp qlinkedlist.h
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:core5.pro:<TRUE>:
# MODULE = "core5compat"
qt_add_docs(Core5Compat
    doc/qtcore5.qdocconf
)

