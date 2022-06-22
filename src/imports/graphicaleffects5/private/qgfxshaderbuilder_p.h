// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGFXSHADERBUILDER_P_H
#define QGFXSHADERBUILDER_P_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtCore/QUrl>
#include <QtShaderTools/private/qshaderbaker_p.h>
#include <QtGui/private/qshader_p.h>

#include <QtQml/QJSValue>

QT_BEGIN_NAMESPACE

class QTemporaryFile;
class QGfxShaderBuilder : public QObject
{
    Q_OBJECT

public:
    QGfxShaderBuilder();

    Q_INVOKABLE QVariantMap gaussianBlur(const QJSValue &parameters);
    Q_INVOKABLE QUrl buildVertexShader(const QByteArray &code);
    Q_INVOKABLE QUrl buildFragmentShader(const QByteArray &code);

private:
    QUrl buildShader(const QByteArray &code, QShader::Stage stage, QTemporaryFile *output);

    int m_maxBlurSamples = 0;
    QShaderBaker m_shaderBaker;

    QTemporaryFile *m_fragmentShader = nullptr;
    QTemporaryFile *m_vertexShader = nullptr;
};

QT_END_NAMESPACE

#endif // QGFXSHADERBUILDER_P_H
