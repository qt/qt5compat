# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial

# Generated from src.pro.

add_subdirectory(core5)
if(TARGET Qt::Gui AND TARGET Qt::Quick)
    add_subdirectory(imports)
endif()
