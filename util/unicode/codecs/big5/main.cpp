// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtextcodec.h>
#include <qbytearray.h>
#include <qstring.h>
#include <qdebug.h>
#include <qfile.h>
#include <qcoreapplication.h>
#include <qset.h>

struct Map { Map(uint u,  uint b) : uc(u),  b5(b) {} uint uc; uint b5; };

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTextCodec *big5 = QTextCodec::codecForName("Big5-hkscs");

#if 0
    QFile f("/home/lars/dev/qt-4.0/util/unicode/data/big5-eten.txt");
    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        if (line.startsWith("#"))
            continue;
        line.replace("0x", "");
        line.replace("U+", "");
        line.replace("\t", " ");
        line = line.simplified();
        QList<QByteArray> split = line.split(' ');
        bool ok;
        int b5 = split.at(0).toInt(&ok, 16);
        Q_ASSERT(ok);
        int uc = split.at(1).toInt(&ok, 16);
        Q_ASSERT(ok);
        if (b5 < 0x100)
            continue;
#else
    QFile f(":/BIG5");
    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        if (line.startsWith("CHARMAP"))
            break;
    }
    QSet<uint> b5_ok;
    QSet<uint> uc_ok;
    QList<Map> b5_to_uc_map;
    QList<Map> uc_to_b5_map;
    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        if (line.startsWith("%"))
            continue;
        if (line.startsWith("END CHARMAP"))
            break;
        line.replace("/x", "");
        line.replace("<U", "");
        line.replace(">", "");
        line.replace("\t", " ");
        line = line.simplified();
        QList<QByteArray> split = line.split(' ');
        bool ok;
        int b5 = split.at(1).toInt(&ok, 16);
        Q_ASSERT(ok);
        int uc = split.at(0).toInt(&ok, 16);
        Q_ASSERT(ok);
        if (b5 < 0x100 || uc > 0xffff)
            continue;
#endif

//         qDebug() << hex << "testing: '" << b5 << "' - '" << uc << "'";
        QByteArray ba;

        ba += (char)(uchar)(b5 >> 8);
        ba += (char)(uchar)(b5 & 0xff);

        QString s = big5->toUnicode(ba);
        Q_ASSERT(s.length() == 1);
        QString s2;
        s2 = QChar(uc);
        ba = big5->fromUnicode(s2);
        Q_ASSERT(ba.length() <= 2);
        int round;
        if (ba.length() == 1)
            round = (int)(uchar)ba[0];
        else
            round = ((int)(uchar)ba[0] << 8) + (int)(uchar)ba[1];
        if (b5 != round)
            uc_to_b5_map += Map(uc, b5);
        else
            b5_ok.insert(b5);

        if (s[0].unicode() != uc)
            b5_to_uc_map += Map(uc, b5);
        else
            uc_ok.insert(uc);
    };

    QList<QByteArray> list;
    for (Map m : std::as_const(b5_to_uc_map)) {
        if (!uc_ok.contains(m.b5))
            list += QByteArray("    { 0x" + QByteArray::number(m.b5, 16) + ", 0x" + QByteArray::number(m.uc, 16) + " }\n");;
    }
    std::sort(list.begin(), list.end());
    qDebug() << "struct B5Map b5_to_uc_map = {\n" << list.join() + "\n};";

    list = QList<QByteArray>();
    for (Map m : std::as_const(uc_to_b5_map))
        if (!b5_ok.contains(m.uc))
            list += QByteArray("    { 0x" + QByteArray::number(m.uc, 16) + ", 0x" + QByteArray::number(m.b5, 16) + " }\n");;
    std::sort(list.begin(), list.end());;
    qDebug() << "struct B5Map uc_to_b5_map = {\n" << list.join() + "\n};";
}
