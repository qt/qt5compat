// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//! [example]
import QtQuick 2.12
import QtGraphicalEffects 1.12

Item {
    width: 300
    height: 300

    Image {
        id: bug
        source: "images/bug.jpg"
        sourceSize: Qt.size(parent.width, parent.height)
        smooth: true
        visible: false
    }

    GaussianBlur {
        anchors.fill: bug
        source: bug
        radius: 8
        samples: 16
    }
}
//! [example]
