# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qxmlinputsource.pro.

#####################################################################
## tst_qxmlinputsource Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qxmlinputsource LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qxmlinputsource
    SOURCES
        tst_qxmlinputsource.cpp
    LIBRARIES
        Qt::Core5Compat
        Qt::Network
        Qt::Xml
)

# This test and tst_qxmlsimplereader both bind the same hard-coded TCP
# port (1088). Serialize their runs:
if(TEST tst_qxmlinputsource)
    set_tests_properties(tst_qxmlinputsource PROPERTIES
        RESOURCE_LOCK tcp_port_1088)
endif()
