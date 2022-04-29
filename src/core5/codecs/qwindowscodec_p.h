/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QWINDOWSCODEC_P_H
#define QWINDOWSCODEC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtextcodec_p.h>

QT_REQUIRE_CONFIG(textcodec);

QT_BEGIN_NAMESPACE

class QWindowsLocalCodec : public QTextCodec
{
public:
    QWindowsLocalCodec();
    ~QWindowsLocalCodec();

    QString convertToUnicode(const char *, int, ConverterState *) const override;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const override;

    QByteArray name() const override;
    int mibEnum() const override;

};

QT_END_NAMESPACE

#endif
