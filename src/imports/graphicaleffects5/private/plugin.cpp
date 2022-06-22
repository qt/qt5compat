// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>

#include "qgfxshaderbuilder_p.h"
#include "qgfxsourceproxy_p.h"

QT_BEGIN_NAMESPACE

class QtGraphicalEffectsPrivatePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtGraphicalEffectsPrivatePlugin(QObject *parent = 0) : QQmlExtensionPlugin(parent) { }
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QByteArray(uri) == QByteArrayLiteral("Qt5Compat.GraphicalEffects.private"));
        qmlRegisterType<QGfxSourceProxy>(uri, 1, 0, "SourceProxy");
        qmlRegisterType<QGfxShaderBuilder>(uri, 1, 0, "ShaderBuilder");

        qmlRegisterModule(uri, QT_VERSION_MAJOR, QT_VERSION_MINOR);
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
