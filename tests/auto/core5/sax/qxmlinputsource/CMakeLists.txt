# Generated from qxmlinputsource.pro.

#####################################################################
## tst_qxmlinputsource Test:
#####################################################################

find_package(Qt6 COMPONENTS Core5Compat)
find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Xml)

qt_add_test(tst_qxmlinputsource
    SOURCES
        tst_qxmlinputsource.cpp
    PUBLIC_LIBRARIES
        Qt::Core5Compat
        Qt::Network
        Qt::Xml
)
