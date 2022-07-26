# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qxml.pro.

#####################################################################
## tst_qxml Test:
#####################################################################

# Collect test data
list(APPEND test_data "0x010D.xml")

qt_internal_add_test(tst_qxml
    SOURCES
        tst_qxml.cpp
    LIBRARIES
        Qt::Core5Compat
    TESTDATA ${test_data}
)
