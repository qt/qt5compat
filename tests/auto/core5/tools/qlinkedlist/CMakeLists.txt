# Generated from qlinkedlist.pro.

#####################################################################
## tst_qlinkedlist Test:
#####################################################################

qt_internal_add_test(tst_qlinkedlist
    SOURCES
        tst_qlinkedlist.cpp
    PUBLIC_LIBRARIES
        Qt::Core5Compat
)

set_target_properties(tst_qlinkedlist PROPERTIES
    QT_INTERNAL_UNDEF_QT_NO_JAVA_STYLE_ITERATORS TRUE)

## Scopes:
#####################################################################
