# Generated from qxmlsimplereader.pro.

#####################################################################
## tst_qxmlsimplereader Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    encodings/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    xmldocs/*)
list(APPEND test_data ${test_data_glob})

find_package(Qt6 COMPONENTS Core5Compat)
find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Xml)

qt_add_test(tst_qxmlsimplereader
    SOURCES
        parser/parser.cpp parser/parser.h
        tst_qxmlsimplereader.cpp
    INCLUDE_DIRECTORIES
        parser
    PUBLIC_LIBRARIES
        Qt::Core5Compat
        Qt::Network
        Qt::Xml
    TESTDATA ${test_data}
)
