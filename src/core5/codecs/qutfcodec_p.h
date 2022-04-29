/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2018 Intel Corporation.
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

#ifndef QUTFCODEC_P_H
#define QUTFCODEC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#include <QtCore/qendian.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

#include <private/qstringconverter_p.h>
#include <private/qtextcodec_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(textcodec)

class QUtf8Codec : public QTextCodec {
public:
    ~QUtf8Codec();

    QByteArray name() const override;
    int mibEnum() const override;

    QString convertToUnicode(const char *, int, ConverterState *) const override;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const override;
    void convertToUnicode(QString *target, const char *, int, ConverterState *) const;
};

class QUtf16Codec : public QTextCodec {
protected:
public:
    QUtf16Codec() { e = DetectEndianness; }
    ~QUtf16Codec();

    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;

    QString convertToUnicode(const char *, int, ConverterState *) const override;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const override;

protected:
    DataEndianness e;
};

class QUtf16BECodec : public QUtf16Codec {
public:
    QUtf16BECodec() : QUtf16Codec() { e = BigEndianness; }
    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
};

class QUtf16LECodec : public QUtf16Codec {
public:
    QUtf16LECodec() : QUtf16Codec() { e = LittleEndianness; }
    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
};

class QUtf32Codec : public QTextCodec {
public:
    QUtf32Codec() { e = DetectEndianness; }
    ~QUtf32Codec();

    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;

    QString convertToUnicode(const char *, int, ConverterState *) const override;
    QByteArray convertFromUnicode(const QChar *, int, ConverterState *) const override;

protected:
    DataEndianness e;
};

class QUtf32BECodec : public QUtf32Codec {
public:
    QUtf32BECodec() : QUtf32Codec() { e = BigEndianness; }
    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
};

class QUtf32LECodec : public QUtf32Codec {
public:
    QUtf32LECodec() : QUtf32Codec() { e = LittleEndianness; }
    QByteArray name() const override;
    QList<QByteArray> aliases() const override;
    int mibEnum() const override;
};

#endif // textcodec

QT_END_NAMESPACE

#endif // QUTFCODEC_P_H
