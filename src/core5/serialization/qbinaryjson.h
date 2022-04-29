/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore5Compat module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/
#ifndef QBINARYJSON_H
#define QBINARYJSON_H

#include <QtCore5Compat/qcore5global.h>
#include <QtCore/qjsondocument.h>

#if 0
// This is needed for generating the QBinaryJson forward header
#pragma qt_class(QBinaryJson)
#endif

QT_BEGIN_NAMESPACE

namespace QBinaryJson {

enum DataValidation {
    Validate,
    BypassValidation
};

Q_CORE5COMPAT_EXPORT QJsonDocument fromRawData(const char *data, int size, DataValidation validation = Validate);
Q_CORE5COMPAT_EXPORT const char *toRawData(const QJsonDocument &document, int *size);

Q_CORE5COMPAT_EXPORT QJsonDocument fromBinaryData(const QByteArray &data, DataValidation validation = Validate);
Q_CORE5COMPAT_EXPORT QByteArray toBinaryData(const QJsonDocument &document);

} // QBinaryJson

QT_END_NAMESPACE

#endif // QBINARYJSON_H
