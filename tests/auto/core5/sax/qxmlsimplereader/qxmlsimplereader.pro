CONFIG += testcase
TARGET = tst_qxmlsimplereader
INCLUDEPATH += parser

HEADERS +=  parser/parser.h
SOURCES += tst_qxmlsimplereader.cpp parser/parser.cpp

CONFIG += no_batch
QT = core network xml core5compat testlib

TESTDATA += encodings/* xmldocs/*
