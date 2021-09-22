# Generated from qt5compat.pro.

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(Qt5Compat # special case
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt 5 Compatibility Libraries" # special case
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Network Xml Gui Quick)

qt_build_repo()
