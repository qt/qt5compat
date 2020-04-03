# Generated from qt5compat.pro.

cmake_minimum_required(VERSION 3.15.0)

project(Qt5Compat # special case
    VERSION 6.0.0
    DESCRIPTION "Qt 5 Compatibility Libraries" # special case
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# special case begin
find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
# find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS SET_ME_TO_SOMETHING_USEFUL)
# special case end

qt_build_repo()
