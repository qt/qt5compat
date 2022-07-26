# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qlinkedlist.pro.

#####################################################################
## tst_qlinkedlist Test:
#####################################################################

qt_internal_add_test(tst_qlinkedlist
    SOURCES
        tst_qlinkedlist.cpp
    LIBRARIES
        Qt::Core5Compat
)

qt_internal_undefine_global_definition(tst_qlinkedlist QT_NO_JAVA_STYLE_ITERATORS)
