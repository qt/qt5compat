// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest>

#include "qbinaryjson.h"

#include <QtCore/qendian.h>
#include "qjsonarray.h"
#include "qjsonobject.h"
#include "qjsonvalue.h"
#include "qjsondocument.h"
#include "qregularexpression.h"
#include "qscopeguard.h"

#include <limits>

#define INVALID_UNICODE "\xCE\xBA\xE1"
#define UNICODE_NON_CHARACTER "\xEF\xBF\xBF"
#define UNICODE_DJE "\320\202" // Character from the Serbian Cyrillic alphabet

class tst_QtJson: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void fromBinary();
    void toAndFromBinary_data();
    void toAndFromBinary();
    void invalidBinaryData();
    void compactArray();
    void compactObject();
    void validation();
    void testCompactionError();

    void binaryDataOutOfBoundsRead_data();
    void binaryDataOutOfBoundsRead();
private:
    QString testDataDir;
};

void tst_QtJson::initTestCase()
{
    testDataDir = QFileInfo(QFINDTESTDATA("test.json")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();
}

void tst_QtJson::fromBinary()
{
    QFile file(testDataDir + "/test.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray testJson = file.readAll();

    QJsonDocument doc = QJsonDocument::fromJson(testJson);
    QJsonDocument outdoc = QBinaryJson::fromBinaryData(QBinaryJson::toBinaryData(doc));
    QVERIFY(!outdoc.isNull());
    QCOMPARE(doc, outdoc);

    QFile bfile(testDataDir + "/test.bjson");
    QVERIFY(bfile.open(QFile::ReadOnly));
    QByteArray binary = bfile.readAll();

    QJsonDocument bdoc = QBinaryJson::fromBinaryData(binary);
    QVERIFY(!bdoc.isNull());
    QCOMPARE(doc.toVariant(), bdoc.toVariant());
    QCOMPARE(doc, bdoc);
}

void tst_QtJson::toAndFromBinary_data()
{
    QTest::addColumn<QString>("filename");
    QTest::newRow("test.json") << (testDataDir + "/test.json");
    QTest::newRow("test2.json") << (testDataDir + "/test2.json");
}

void tst_QtJson::toAndFromBinary()
{
    QFETCH(QString, filename);
    QFile file(filename);
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QVERIFY(!doc.isNull());
    {
        QJsonDocument outdoc = QBinaryJson::fromBinaryData(QBinaryJson::toBinaryData(doc));
        QVERIFY(!outdoc.isNull());
        QCOMPARE(doc, outdoc);
    }
#if QT_DEPRECATED_SINCE(6, 12)
    {
        int size = -1;
        QTest::ignoreMessage(QtWarningMsg,
                             "QBinaryJson: In Qt 6, unlike Qt 5, toRawData() transfers ownership "
                             "of the pointer to the caller. Prefer toBinaryData() instead.");
        QT_IGNORE_DEPRECATIONS(
        auto rawData = QBinaryJson::toRawData(doc, &size);
        )
        // we own the returned data
        const auto releaseRawData = qScopeGuard([rawData]() {
            free(const_cast<char *>(rawData));
        });
        QVERIFY(size > 0);
        QJsonDocument outdoc = QBinaryJson::fromRawData(rawData, size);
        QVERIFY(!outdoc.isNull());
        QCOMPARE(doc, outdoc);
    }
#endif // QT_DEPRECATED_SINCE(6, 12)
}

void tst_QtJson::invalidBinaryData()
{
    QDir dir(testDataDir + "/invalidBinaryData");
    QFileInfoList files = dir.entryInfoList();
    for (int i = 0; i < files.size(); ++i) {
        if (!files.at(i).isFile())
            continue;
        QFile file(files.at(i).filePath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray bytes = file.readAll();
        bytes.squeeze();
        QJsonDocument document = QBinaryJson::fromRawData(bytes.constData(), bytes.size());
        QVERIFY(document.isNull());
    }
}

void tst_QtJson::compactArray()
{
    QJsonArray array;
    array.append(QLatin1String("First Entry"));
    array.append(QLatin1String("Second Entry"));
    array.append(QLatin1String("Third Entry"));
    QJsonDocument doc(array);
    int s =  QBinaryJson::toBinaryData(doc).size();
    array.removeAt(1);
    doc.setArray(array);
    QVERIFY(s > QBinaryJson::toBinaryData(doc).size());
    s = QBinaryJson::toBinaryData(doc).size();
    QCOMPARE(doc.toJson(),
             QByteArray("[\n"
                        "    \"First Entry\",\n"
                        "    \"Third Entry\"\n"
                        "]\n"));

    array.removeAt(0);
    doc.setArray(array);
    QVERIFY(s > QBinaryJson::toBinaryData(doc).size());
    s = QBinaryJson::toBinaryData(doc).size();
    QCOMPARE(doc.toJson(),
             QByteArray("[\n"
                        "    \"Third Entry\"\n"
                        "]\n"));

    array.removeAt(0);
    doc.setArray(array);
    QVERIFY(s > QBinaryJson::toBinaryData(doc).size());
    s = QBinaryJson::toBinaryData(doc).size();
    QCOMPARE(doc.toJson(),
             QByteArray("[\n"
                        "]\n"));

}

void tst_QtJson::compactObject()
{
    QJsonObject object;
    object.insert(QLatin1String("Key1"), QLatin1String("First Entry"));
    object.insert(QLatin1String("Key2"), QLatin1String("Second Entry"));
    object.insert(QLatin1String("Key3"), QLatin1String("Third Entry"));
    QJsonDocument doc(object);
    int s =  QBinaryJson::toBinaryData(doc).size();
    object.remove(QLatin1String("Key2"));
    doc.setObject(object);
    QVERIFY(s > QBinaryJson::toBinaryData(doc).size());
    s = QBinaryJson::toBinaryData(doc).size();
    QCOMPARE(doc.toJson(),
             QByteArray("{\n"
                        "    \"Key1\": \"First Entry\",\n"
                        "    \"Key3\": \"Third Entry\"\n"
                        "}\n"));

    object.remove(QLatin1String("Key1"));
    doc.setObject(object);
    QVERIFY(s > QBinaryJson::toBinaryData(doc).size());
    s = QBinaryJson::toBinaryData(doc).size();
    QCOMPARE(doc.toJson(),
             QByteArray("{\n"
                        "    \"Key3\": \"Third Entry\"\n"
                        "}\n"));

    object.remove(QLatin1String("Key3"));
    doc.setObject(object);
    QVERIFY(s > QBinaryJson::toBinaryData(doc).size());
    s = QBinaryJson::toBinaryData(doc).size();
    QCOMPARE(doc.toJson(),
             QByteArray("{\n"
                        "}\n"));

}

void tst_QtJson::validation()
{
    // this basically tests that we don't crash on corrupt data
    QFile file(testDataDir + "/test.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray testJson = file.readAll();
    QVERIFY(!testJson.isEmpty());

    QJsonDocument doc = QJsonDocument::fromJson(testJson);
    QVERIFY(!doc.isNull());

    QByteArray binary = QBinaryJson::toBinaryData(doc);

    // only test the first 1000 bytes. Testing the full file takes too long
    for (int i = 0; i < 1000; ++i) {
        QByteArray corrupted = binary;
        corrupted[i] = char(0xff);
        QJsonDocument doc = QBinaryJson::fromBinaryData(corrupted);
        if (doc.isNull())
            continue;
        QByteArray json = doc.toJson();
    }


    QFile file2(testDataDir + "/test3.json");
    QVERIFY(file2.open(QFile::ReadOnly));
    testJson = file2.readAll();
    QVERIFY(!testJson.isEmpty());

    doc = QJsonDocument::fromJson(testJson);
    QVERIFY(!doc.isNull());

    binary = QBinaryJson::toBinaryData(doc);

    for (int i = 0; i < binary.size(); ++i) {
        QByteArray corrupted = binary;
        corrupted[i] = char(0xff);
        QJsonDocument doc = QBinaryJson::fromBinaryData(corrupted);
        if (doc.isNull())
            continue;
        QByteArray json = doc.toJson();

        corrupted = binary;
        corrupted[i] = 0x00;
        doc = QBinaryJson::fromBinaryData(corrupted);
        if (doc.isNull())
            continue;
        json = doc.toJson();
    }
}

void tst_QtJson::testCompactionError()
{
    QJsonObject schemaObject;
    schemaObject.insert("_Type", QLatin1String("_SchemaType"));
    schemaObject.insert("name", QLatin1String("Address"));
    schemaObject.insert("schema", QJsonObject());
    {
        QJsonObject content(schemaObject);
        QJsonDocument doc(content);
        QVERIFY(!doc.isNull());
        QByteArray hash = QCryptographicHash::hash(QBinaryJson::toBinaryData(doc), QCryptographicHash::Md5).toHex();
        schemaObject.insert("_Version", QString::fromLatin1(hash.constData(), hash.size()));
    }

    QJsonObject schema;
    schema.insert("streetNumber", schema.value("number").toObject());
    schemaObject.insert("schema", schema);
    {
        QJsonObject content(schemaObject);
        content.remove("_Uuid");
        content.remove("_Version");
        QJsonDocument doc(content);
        QVERIFY(!doc.isNull());
        QByteArray hash = QCryptographicHash::hash(QBinaryJson::toBinaryData(doc), QCryptographicHash::Md5).toHex();
        schemaObject.insert("_Version", QString::fromLatin1(hash.constData(), hash.size()));
    }
}

enum class CallType : quint8
{
    Binary,
    Raw,
};

void tst_QtJson::binaryDataOutOfBoundsRead_data()
{
    QTest::addColumn<quint32>("corruptedSize");
    QTest::addColumn<int>("corruptedTableOffset");
    QTest::addColumn<CallType>("callType");

    // The case with 0xFFFFFFF4 should be handled properly, because the
    // calculation does not roundtrip
    QTest::newRow("0xFFFFFFF4_128_binary") << 0xFFFFFFF4u << 128 << CallType::Binary;
    QTest::newRow("0xFFFFFFF8_32_binary") << 0xFFFFFFF8u << 32 << CallType::Binary;
    QTest::newRow("0xFFFFFFFC_512_binary") << 0xFFFFFFFCu << 512 << CallType::Binary;
    QTest::newRow("0xFFFFFFFF_4088_binary") << 0xFFFFFFFFu<<  4088 << CallType::Binary;

    QTest::newRow("0xFFFFFFF4_128_raw") << 0xFFFFFFF4u << 128 << CallType::Raw;
    QTest::newRow("0xFFFFFFF8_32_raw") << 0xFFFFFFF8u << 32 << CallType::Raw;
    QTest::newRow("0xFFFFFFFC_512_raw") << 0xFFFFFFFCu << 512 << CallType::Raw;
    QTest::newRow("0xFFFFFFFF_4088_raw") << 0xFFFFFFFFu<<  4088 << CallType::Raw;
}

void tst_QtJson::binaryDataOutOfBoundsRead()
{
    QFETCH(const quint32, corruptedSize);
    QFETCH(const int, corruptedTableOffset);
    QFETCH(const CallType, callType);

    // Two single element arrays whose value is small enough to be stored inline
    // in the root's value table: Value packs a 27 bit signed int, and
    // Value::isValid() accepts an inline int without checking any offset. Both
    // therefore encode to Header(8) + Base(12) + one 4 byte Value = 24 bytes
    // with no out-of-line data, so moving the table moves the value with it.
    const QJsonDocument original = QJsonDocument::fromJson("[1]");
    const QJsonDocument planted = QJsonDocument::fromJson("[424242]");
    QVERIFY(!original.isNull());
    QVERIFY(!planted.isNull());

    const QByteArray originalBlob = QBinaryJson::toBinaryData(original);
    const QByteArray plantedBlob = QBinaryJson::toBinaryData(planted);

    const qsizetype originalBlobSize = originalBlob.size();

    constexpr qsizetype HeaderSize = 8;   // Header { qle_uint tag; qle_uint version; }
    constexpr qsizetype RootSizeOffset = HeaderSize;        // Base::size
    constexpr qsizetype TableOffsetOffset = HeaderSize + 8; // Base::tableOffset

    // A backing allocation much larger than the buffer the parser will be
    // given. Everything past the first originalBlobSize bytes is memory that
    // a correct parser must never touch; planting known bytes there is what
    // makes the out-of-bounds read observable rather than merely undefined.
    // Pointing tableOffset past a tightly sized allocation instead would be a
    // genuine heap-buffer-overflow, but it would only be diagnosed under a
    // sanitizer and would otherwise crash the test run.
    QByteArray buffer(4096, '\0');
    Q_ASSERT(corruptedTableOffset + qsizetype(sizeof(quint32)) <= buffer.size());
    // Copy the original blob at the beginning
    memcpy(buffer.data(), originalBlob.constData(), originalBlobSize);

    // Copy the other document's 4 byte table entry out of bounds.
    const quint32 plantedBlobTableOffset =
            qFromLittleEndian<quint32>(plantedBlob.constData() + TableOffsetOffset);
    memcpy(buffer.data() + corruptedTableOffset,
           plantedBlob.constData() + HeaderSize + plantedBlobTableOffset,
           sizeof(quint32));

    // fromBinaryData() computes the document bound as
    //     const uint size = sizeof(Header) + root.size;
    // sizeof() is 64 bit while root.size is a 32 bit field taken straight from
    // the input, so the sum is evaluated in 64 bits and then truncated on
    // assignment to uint. Every root.size in [0xfffffff8, 0xffffffff] collapses
    // to 0..7 and trivially passes the "size > uint(data.size())" check that is
    // supposed to keep the root inside the buffer. ConstData::isValid() then
    // recomputes maxSize as "alloc - sizeof(Header)", which underflows straight
    // back to root.size, so the root ends up being validated against its own
    // attacker-supplied length instead of against the real buffer.
    //
    // Once that happens nothing constrains tableOffset either: the guard
    // "tableOffset + length() * sizeof(offset) > size" now compares against
    // root.size instead of against the buffer, so both Array::isValid() and
    // Array::toJsonArray() read the value table from 32 bytes past the end.

    // Modify the size of the original blob
    qToLittleEndian(corruptedSize, buffer.data() + RootSizeOffset);
    // Modify the tableOffset to point to a newly-injected table
    qToLittleEndian(quint32(corruptedTableOffset - HeaderSize),
                    buffer.data() + TableOffsetOffset);

    // Hand the parser a view over the first origianlBlobSize bytes only.
    // That size is the only bound it is given, so anything it reads beyond it
    // is out of bounds.
    const QByteArray nonOwningArray =
            QByteArray::fromRawData(buffer.constData(), originalBlobSize);
    QCOMPARE(nonOwningArray.size(), originalBlobSize);

    // The code below passes a correct size to fromRawData() call.
    // Surely we could observe the same problem, if we passed corruptedSize
    // instead, but that's a typical problem for all APIs that take a pointer
    // and a size.
    const QJsonDocument doc = (callType == CallType::Binary)
            ? QBinaryJson::fromBinaryData(nonOwningArray)
            : QBinaryJson::fromRawData(nonOwningArray.constData(), int(nonOwningArray.size()));
    if (!doc.isNull()) {
        QFAIL(qPrintable(QString::asprintf(
                "fromBinaryData() was handed %lld bytes but read the root array's value "
                "table from offset %lld: it returned %s, which was planted out of bounds, "
                "instead of rejecting the document",
                qlonglong(nonOwningArray.size()), qlonglong(corruptedTableOffset),
                doc.toJson(QJsonDocument::Compact).simplified().constData())));
    }

    // Check that the original blob can still be parsed correctly
    QCOMPARE(QBinaryJson::fromBinaryData(originalBlob), original);
}

QTEST_MAIN(tst_QtJson)
#include "tst_qtjson.moc"
