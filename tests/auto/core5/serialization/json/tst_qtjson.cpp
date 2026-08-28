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

using namespace Qt::StringLiterals;

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
    void invalidBinaryData_data();
    void invalidBinaryData();
    void compactArray();
    void compactObject();
    void validation();
    void testCompactionError();

    void binaryDataOutOfBoundsRead_data();
    void binaryDataOutOfBoundsRead();
    void binaryDataDeepNestingArray_data();
    void binaryDataDeepNestingArray();
    void binaryDataDeepNestingObject_data();
    void binaryDataDeepNestingObject();
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
    {
        int size = -1;
        QTest::ignoreMessage(QtWarningMsg,
                             "QBinaryJson: In Qt 6, unlike Qt 5, toRawData() transfers ownership "
                             "of the pointer to the caller. Prefer toBinaryData() instead.");
        auto rawData = QBinaryJson::toRawData(doc, &size);
        // we own the returned data
        const auto releaseRawData = qScopeGuard([rawData]() {
            free(const_cast<char *>(rawData));
        });
        QVERIFY(size > 0);
        QJsonDocument outdoc = QBinaryJson::fromRawData(rawData, size);
        QVERIFY(!outdoc.isNull());
        QCOMPARE(doc, outdoc);
    }
}

void tst_QtJson::invalidBinaryData_data()
{
    QTest::addColumn<QString>("name");
    QDir dir(testDataDir + "/invalidBinaryData");
    const QStringList files = dir.entryList({u"*.bjson"_s},
                                            QDir::Filter::Files, QDir::SortFlag::Name);
    for (const QString &file : files)
        QTest::addRow("%s", QFile::encodeName(file).constData()) << file;
}

void tst_QtJson::invalidBinaryData()
{
    QFETCH(const QString, name);
    const QDir dir(testDataDir + "/invalidBinaryData");
    {
        QFile file(dir.filePath(name));
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

// Builds a binary JSON document made of `depth` arrays nested directly inside
// one another, the innermost one empty. The format lays the Bases out
// consecutively from the outermost inwards, followed by the one-entry value
// tables in the opposite order:
//
//     [Header][Base 0][Base 1]...[Base d-1][table d-2]...[table 0]
//
// so every extra level costs exactly sizeof(Base) + sizeof(offset) = 16 bytes.
// This has to be written by hand: going through QJsonArray instead would
// recurse in the Qt API long before the code under test got a chance to.
static QByteArray nestedArrayBinaryJson(int depth)
{
    Q_ASSERT(depth >= 1);

    constexpr qsizetype HeaderSize = 8;   // Header { qle_uint tag; qle_uint version; }
    constexpr qsizetype BaseSize = 12;    // Base { size; isObjectAndLength; tableOffset; }
    constexpr qsizetype EntrySize = 4;    // one Value

    // Borrow a real document's header rather than spelling out the format tag.
    // The document has to be non-empty.
    const QByteArray header =
            QBinaryJson::toBinaryData(QJsonDocument::fromJson("[0]")).left(HeaderSize);
    Q_ASSERT(header.size() == HeaderSize);

    QByteArray blob(HeaderSize + BaseSize * depth + EntrySize * (depth - 1), '\0');
    memcpy(blob.data(), header.constData(), HeaderSize);

    char *const root = blob.data() + HeaderSize;
    const qsizetype tablesStart = BaseSize * depth;

    for (int i = 0; i < depth; ++i) {
        char *const base = root + BaseSize * i;
        const bool innermost = (i == depth - 1);

        // Everything belonging to this array: its own Base, every level nested
        // below it, and their tables.
        const quint32 size = quint32(BaseSize + 16 * (depth - i - 1));
        // isObject is bit 0 -- zero for an array -- and length is bits 1..31.
        const quint32 isObjectAndLength = innermost ? 0u : (1u << 1);
        const quint32 tableOffset = innermost
                ? quint32(BaseSize)
                : quint32(tablesStart + EntrySize * (depth - 2 - i) - BaseSize * i);

        qToLittleEndian(size, base);
        qToLittleEndian(isObjectAndLength, base + 4);
        qToLittleEndian(tableOffset, base + 8);

        if (!innermost) {
            // A Value naming the next Base, 12 bytes further in: the type goes
            // in bits 0..2 and the payload offset in bits 5..31.
            const quint32 entry = quint32(QJsonValue::Array) | (quint32(BaseSize) << 5);
            qToLittleEndian(entry, base + tableOffset);
        }
    }

    return blob;
}

void tst_QtJson::binaryDataDeepNestingArray_data()
{
    QTest::addColumn<QBinaryJson::DataValidation>("mode");
    QTest::addColumn<int>("depth");

    QTest::newRow("validate_4096")
            << QBinaryJson::Validate << 4096;
    QTest::newRow("bypass_validation_4096")
            << QBinaryJson::BypassValidation << 4096;
}

void tst_QtJson::binaryDataDeepNestingArray()
{
    QFETCH(const QBinaryJson::DataValidation, mode);
    QFETCH(const int, depth);

    // nestedArrayBinaryJson() is hand-rolled, so check it really does produce a
    // well formed document at a depth that is obviously fine. Without this the
    // deep case below could pass for the wrong reason: a malformed blob that is
    // rejected on its first field never recurses at all.
    QCOMPARE(QBinaryJson::fromBinaryData(nestedArrayBinaryJson(3), mode),
             QJsonDocument::fromJson("[[[]]]"));

    // Depending on the mode, this checks either the validation path, or the
    // JSON construction path. Both could recurse and cause stack overflow with
    // deeply nested documents.
    const QJsonDocument doc = QBinaryJson::fromBinaryData(nestedArrayBinaryJson(depth), mode);
    QVERIFY(!doc.isNull());
    QVERIFY(doc.isArray());

    QJsonArray arr = doc.array();
    // We have depth-1 nested arrays
    for (int i = 0; i < depth - 1; ++i) {
        QVERIFY(!arr.isEmpty());
        QJsonValue val = arr.first();
        QVERIFY(val.isArray());
        arr = val.toArray();
    }
    // And the last array is empty
    QVERIFY(arr.isEmpty());
}

// The object counterpart of nestedArrayBinaryJson(): `depth` objects nested
// directly inside one another, each holding the single key "a", the innermost
// one empty -- {"a":{"a":...{}}}.
//
// Objects do not store their values in the table the way arrays do. The table
// holds offsets to Entry records, and an Entry is a Value followed by its key,
// so a level costs Base(12) + Entry(8) + table(4) = 24 bytes rather than 16.
// Relative to each Base the layout is
//
//     [Base][child subtree][Entry][table]
//
// which satisfies Object::isValid(): the Entry sits immediately before the
// table, so the "table()[i] + sizeof(Entry) >= tableOffset" guard passes and
// Entry::isValid() is handed exactly the 8 bytes the Entry occupies.
static QByteArray nestedObjectBinaryJson(int depth)
{
    Q_ASSERT(depth >= 1);

    constexpr qsizetype HeaderSize = 8;   // Header { qle_uint tag; qle_uint version; }
    constexpr qsizetype BaseSize = 12;    // Base { size; isObjectAndLength; tableOffset; }
    constexpr qsizetype EntrySize = 8;    // Value(4) + latin1 key "a" (2 + 1), padded to 4
    constexpr qsizetype TableSize = 4;    // one offset
    constexpr qsizetype LevelSize = BaseSize + EntrySize + TableSize;

    const QByteArray header =
            QBinaryJson::toBinaryData(QJsonDocument::fromJson("[0]")).left(HeaderSize);
    Q_ASSERT(header.size() == HeaderSize);

    // Bytes belonging to the object at level i: its own Base, every level
    // nested below it, and their Entries and tables.
    const auto subtreeSize = [depth](int i) {
        return quint32(BaseSize + LevelSize * (depth - 1 - i));
    };

    QByteArray blob(HeaderSize + subtreeSize(0), '\0');
    memcpy(blob.data(), header.constData(), HeaderSize);

    char *const root = blob.data() + HeaderSize;

    for (int i = 0; i < depth; ++i) {
        char *const base = root + BaseSize * i;
        const bool innermost = (i == depth - 1);

        const quint32 size = subtreeSize(i);
        // isObject is bit 0 -- one for an object -- and length is bits 1..31.
        const quint32 isObjectAndLength = innermost ? 1u : (1u | (1u << 1));
        const quint32 entryOffset =
                innermost ? 0u : quint32(BaseSize) + subtreeSize(i + 1);
        const quint32 tableOffset =
                innermost ? quint32(BaseSize) : entryOffset + quint32(EntrySize);

        qToLittleEndian(size, base);
        qToLittleEndian(isObjectAndLength, base + 4);
        qToLittleEndian(tableOffset, base + 8);

        if (innermost)
            continue;

        // The table holds one offset, naming the Entry.
        qToLittleEndian(entryOffset, base + tableOffset);

        // Entry: a Value of type Object pointing at the next Base 12 bytes
        // further in, with the latin1-key bit set, followed by the key stored
        // as Latin1String::Data { qle_ushort length; char latin1[1]; }.
        constexpr quint32 LatinKeyBit = 1u << 4;
        const quint32 value = quint32(QJsonValue::Object) | LatinKeyBit
                | (quint32(BaseSize) << 5);
        qToLittleEndian(value, base + entryOffset);
        qToLittleEndian(quint16(1), base + entryOffset + 4);
        base[entryOffset + 6] = 'a';
    }

    return blob;
}

void tst_QtJson::binaryDataDeepNestingObject_data()
{
    binaryDataDeepNestingArray_data();
}

void tst_QtJson::binaryDataDeepNestingObject()
{
    QFETCH(const QBinaryJson::DataValidation, mode);
    QFETCH(const int, depth);

    QCOMPARE(QBinaryJson::fromBinaryData(nestedObjectBinaryJson(3), mode),
             QJsonDocument::fromJson(R"({"a":{"a":{}}})"));

    // Similarly to the array case, this could recurse either in validation
    // or JSON construction path, potentially causing stack overflow.
    const QJsonDocument doc = QBinaryJson::fromBinaryData(nestedObjectBinaryJson(depth), mode);
    QVERIFY(!doc.isNull());
    QVERIFY(doc.isObject());

    QJsonObject obj = doc.object();
    // We have depth-1 objects with key "a"...
    for (int i = 0; i < depth - 1; ++i) {
        QVERIFY(obj.contains("a"));
        QJsonValue val = obj.value("a");
        QVERIFY(val.isObject());
        obj = val.toObject();
    }
    // And the last one is an empty object
    QVERIFY(obj.isEmpty());
}

QTEST_MAIN(tst_QtJson)
#include "tst_qtjson.moc"
