#####################################################################
## qtgraphicaleffectsprivate Plugin:
#####################################################################

set(qml_files
    "FastGlow.qml"
)
set_source_files_properties(FastGlow.qml PROPERTIES
    QT_QML_SOURCE_VERSION "1.0"
)

qt_internal_add_qml_module(qtgraphicaleffectsprivate
    URI "Qt5Compat.GraphicalEffects.private"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtGraphicalEffectsPrivatePlugin
    PLUGIN_TARGET qtgraphicaleffectsprivate
    NO_GENERATE_PLUGIN_SOURCE
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_QMLTYPES
    SOURCES
        plugin.cpp
        qgfxsourceproxy.cpp qgfxsourceproxy_p.h
    QML_FILES
        ${qml_files}
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)
