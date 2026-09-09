// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDomDocument>
#include <qthread.h>
#include <qtcpserver.h>
#include <qtcpsocket.h>
#include <QtTest/QtTest>
#include <QtCore/qatomic.h>
#include <QtCore/qsemaphore.h>
#include <qfile.h>
#include <qstring.h>
#include <qdir.h>
#include <qbuffer.h>
#include "parser/parser.h"

#include <limits>

static const char *const inputString = "<!DOCTYPE inferno [<!ELEMENT inferno (circle+)><!ELEMENT circle (#PCDATA)>]><inferno><circle /><circle /></inferno>";
static const char *const refString = "setDocumentLocator(locator={columnNumber=1, lineNumber=1})\nstartDocument()\nstartDTD(name=\"inferno\", publicId=\"\", systemId=\"\")\nendDTD()\nstartElement(namespaceURI=\"\", localName=\"inferno\", qName=\"inferno\", atts=[])\nstartElement(namespaceURI=\"\", localName=\"circle\", qName=\"circle\", atts=[])\nendElement(namespaceURI=\"\", localName=\"circle\", qName=\"circle\")\nstartElement(namespaceURI=\"\", localName=\"circle\", qName=\"circle\", atts=[])\nendElement(namespaceURI=\"\", localName=\"circle\", qName=\"circle\")\nendElement(namespaceURI=\"\", localName=\"inferno\", qName=\"inferno\")\nendDocument()\n";

#define TEST_PORT 1088

class XmlServer : public QThread
{
    Q_OBJECT
public:
    XmlServer(QObject *parent = 0) : QThread(parent) {}

    QSemaphore threadStarted;
    bool listening = false;
    QAtomicInt quitSoon;

protected:
    void run() override;
};

#define CHUNK_SIZE 2048

void XmlServer::run()
{
    QTcpServer srv;

    listening = srv.listen(QHostAddress::Any, TEST_PORT);
    threadStarted.release();

    if (!listening) {
        qWarning() << "Failed to listen on" << TEST_PORT << srv.errorString();
        return;
    }

    for (;;) {
        srv.waitForNewConnection(100);

        if (QTcpSocket *sock = srv.nextPendingConnection()) {
            QByteArray fileName;
            for (;;) {
                char c;
                if (sock->getChar(&c)) {
                    if (c == '\n')
                        break;
                    fileName.append(c);
                } else {
                    if (!sock->waitForReadyRead(-1))
                        break;
                }
            }

            QFile file(QString::fromLocal8Bit(fileName));
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "XmlServer::run(): could not open" << fileName;
                sock->abort();
                delete sock;
                continue;
            }

            QByteArray data = file.readAll();
            for (int i = 0; i < data.size();) {
                int cnt = qMin(CHUNK_SIZE, data.size() - i);
                sock->write(data.constData() + i, cnt);
                i += cnt;
                sock->flush();
                QTest::qSleep(1);

                if (quitSoon.loadAcquire()) {
                    sock->abort();
                    break;
                }
            }

            sock->disconnectFromHost();
            delete sock;
        }

        if (quitSoon.loadAcquire())
            break;
    }

    srv.close();
}

class tst_QXmlSimpleReader : public QObject
{
    Q_OBJECT

    public:
        tst_QXmlSimpleReader();
        ~tst_QXmlSimpleReader();

    private slots:
        void initTestCase();
        void testGoodXmlFile();
        void testGoodXmlFile_data();
        void testBadXmlFile();
        void testBadXmlFile_data();
        void testIncrementalParsing();
        void testIncrementalParsing_data();
        void setDataQString();
        void inputFromQIODevice();
        void inputFromString();
        void inputFromSocket_data();
        void inputFromSocket();

        void idsInParseException1();
        void idsInParseException2();
        void preserveCharacterReferences() const;
        void reportNamespace() const;
        void reportNamespace_data() const;
        void roundtripWithNamespaces() const;
        void dtdRecursionLimit();

        void deepElementNesting_data();
        void deepElementNesting();
        void deepElementNestingIncremental_data();
        void deepElementNestingIncremental();

        void valueLongerThanIntMaxDoesNotCrash();

        void entityExpansionLimit_data();
        void entityExpansionLimit();
        void entityExpansionLimitEntityResolver_data();
        void entityExpansionLimitEntityResolver();

        void entityExpansionProperty_data();
        void entityExpansionProperty();

    private:
        static QDomDocument fromByteArray(const QString &title, const QByteArray &ba, bool *ok);
        XmlServer *server;
        QString prefix;
};

tst_QXmlSimpleReader::tst_QXmlSimpleReader() : server(new XmlServer(this))
{
    server->start();
}

tst_QXmlSimpleReader::~tst_QXmlSimpleReader()
{
    server->quitSoon.storeRelease(1);
    server->wait();
}

class MyErrorHandler : public QXmlErrorHandler
{
public:
    QString publicId;
    QString systemId;

    bool error(const QXmlParseException &) override
    {
        return false;
    }

    QString errorString() const override
    {
        return QString();
    }

    bool fatalError(const QXmlParseException &exception) override
    {
        publicId = exception.publicId();
        systemId = exception.systemId();
        return true;
    }

    bool warning(const QXmlParseException &) override
    {
        return true;
    }

};

void tst_QXmlSimpleReader::initTestCase()
{
    prefix = QFileInfo(QFINDTESTDATA("xmldocs")).absolutePath();
    if (prefix.isEmpty())
        QFAIL("Cannot find xmldocs testdata!");
    QDir::setCurrent(prefix);
}

void tst_QXmlSimpleReader::idsInParseException1()
{
    MyErrorHandler handler;
    QXmlSimpleReader reader;

    reader.setErrorHandler(&handler);

    /* A non-wellformed XML document with PUBLIC and SYSTEM. */
    QByteArray input("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
                     "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
                     "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
                     "<head>"
                         "<a/><a/><a/>"
                     "<head/>");

    QBuffer buff(&input);
    QXmlInputSource source(&buff);

    /* Yes, parsing should be reported as a failure. */
    QVERIFY(!reader.parse(source));

    QCOMPARE(handler.publicId, QString::fromLatin1("-//W3C//DTD XHTML 1.0 Strict//EN"));
    QCOMPARE(handler.systemId, QString::fromLatin1("http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"));
}

void tst_QXmlSimpleReader::idsInParseException2()
{
    MyErrorHandler handler;
    QXmlSimpleReader reader;

    reader.setErrorHandler(&handler);

    /* A non-wellformed XML document with only SYSTEM. */
    QByteArray input("<!DOCTYPE html SYSTEM \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
                      "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">"
                      "<head>"
                          "<a/><a/><a/>"
                      "<head/>");

    QBuffer buff(&input);
    QXmlInputSource source(&buff);

    /* Yes, parsing should be reported as a failure. */
    QVERIFY(!reader.parse(source));

    QCOMPARE(handler.publicId, QString());
    QCOMPARE(handler.systemId, QString::fromLatin1("http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"));
}

static QStringList findXmlFiles(QString dir_name)
{
    QStringList result;

    dir_name = QFINDTESTDATA(dir_name);
    QDir dir(dir_name);
    QFileInfoList file_list = dir.entryInfoList(QStringList("*.xml"), QDir::Files, QDir::Name);

    QFileInfoList::const_iterator it = file_list.begin();
    for (; it != file_list.end(); ++it) {
        const QFileInfo &file_info = *it;
        result.append(file_info.filePath());
    }

    return result;
}

void tst_QXmlSimpleReader::testGoodXmlFile_data()
{
    const char * const good_data_dirs[] = {
        "xmldocs/valid/sa",
        "xmldocs/valid/not-sa",
        "xmldocs/valid/ext-sa",
        0
    };
    const char * const *d = good_data_dirs;

    QStringList good_file_list;
    for (; *d != 0; ++d)
        good_file_list += findXmlFiles(*d);

    QTest::addColumn<QString>("file_name");
    QStringList::const_iterator it = good_file_list.begin();
    for (; it != good_file_list.end(); ++it)
        QTest::newRow(qPrintable(*it)) << *it;
}

void tst_QXmlSimpleReader::testGoodXmlFile()
{
    QFETCH(QString, file_name);
    QFile file(file_name);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QString content = file.readAll();
    file.close();
    QVERIFY(file.open(QIODevice::ReadOnly));
    Parser parser;

    QVERIFY(parser.parseFile(&file));

    QFile ref_file(file_name + ".ref");
    QVERIFY(ref_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QByteArray data = ref_file.readAll();
    QString ref_file_contents = QString::fromUtf8(data.constData(), data.size());

    QCOMPARE(parser.result(), ref_file_contents);
}

void tst_QXmlSimpleReader::testBadXmlFile_data()
{
    const char * const bad_data_dirs[] = {
        "xmldocs/not-wf/sa",
        0
    };
    const char * const *d = bad_data_dirs;

    QStringList bad_file_list;
    for (; *d != 0; ++d)
        bad_file_list += findXmlFiles(*d);

    QTest::addColumn<QString>("file_name");
    QStringList::const_iterator it = bad_file_list.begin();
    for (; it != bad_file_list.end(); ++it)
        QTest::newRow(qPrintable(*it)) << *it;
}

void tst_QXmlSimpleReader::testBadXmlFile()
{
    QFETCH(QString, file_name);
    QFile file(file_name);
    QVERIFY(file.open(QIODevice::ReadOnly));
    Parser parser;

    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/030.xml").toLocal8Bit().constData(), "a form feed character is not accepted in XML", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/031.xml").toLocal8Bit().constData(), "a form feed character is not accepted in a processing instruction", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/032.xml").toLocal8Bit().constData(), "a form feed character is not accepted in a comment", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/033.xml").toLocal8Bit().constData(), "overlong sequence - small latin letter d should be rejected", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/038.xml").toLocal8Bit().constData(), "attribute x redefined; should be rejected", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/072.xml").toLocal8Bit().constData(), "entity foo not defined", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/073.xml").toLocal8Bit().constData(), "entity f not defined", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/074.xml").toLocal8Bit().constData(), "entity e is not well-formed (</foo><foo>)", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/076.xml").toLocal8Bit().constData(), "entity foo is not defined", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/077.xml").toLocal8Bit().constData(), "entity bar is not defined within the definition of entity foo", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/078.xml").toLocal8Bit().constData(), "entity foo not defined", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/085.xml").toLocal8Bit().constData(), "Unfinished Public or System Id", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/086.xml").toLocal8Bit().constData(), "Unfinished Public or System Id", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/087.xml").toLocal8Bit().constData(), "Unfinished Public or System Id", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/101.xml").toLocal8Bit().constData(), "Invalid XML encoding name (space before utf-8)", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/102.xml").toLocal8Bit().constData(), "Invalid version specification (1.0 followed by space)", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/104.xml").toLocal8Bit().constData(), "Premature end of data in tag foo", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/116.xml").toLocal8Bit().constData(), "Invalid decimal value", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/117.xml").toLocal8Bit().constData(), "No name", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/119.xml").toLocal8Bit().constData(), "No name", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/122.xml").toLocal8Bit().constData(), "; expected in declaration of element", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/132.xml").toLocal8Bit().constData(), "; expected in declaration of element", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/142.xml").toLocal8Bit().constData(), "Invalid value '0'", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/143.xml").toLocal8Bit().constData(), "Invalid value '31'", Continue);

    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/144.xml").toLocal8Bit().constData(), "noncharacter code 0xFFFF should be rejected", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/145.xml").toLocal8Bit().constData(), "surrogate code point 0xD800 should be rejected", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/146.xml").toLocal8Bit().constData(), "code point out-of-range 0x110000 (must be < 0x10FFFE)", Abort);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/160.xml").toLocal8Bit().constData(), "Parameter references forbidden in internal subset", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/162.xml").toLocal8Bit().constData(), "Parameter references forbidden in internal subset", Continue);

    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/168.xml").toLocal8Bit().constData(), "Surrogate code point 0xEDA080 should be rejected", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/169.xml").toLocal8Bit().constData(), "Surrogate code point 0xEDB080 should be rejected", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/170.xml").toLocal8Bit().constData(), "Code point 0xF7808080 should be rejected", Continue);

    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/180.xml").toLocal8Bit().constData(), "Entity e is not defined", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/181.xml").toLocal8Bit().constData(), "Unregistered error message", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/182.xml").toLocal8Bit().constData(), "Comment not terminated", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/185.xml").toLocal8Bit().constData(), "Entity e not defined", Continue);
    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/186.xml").toLocal8Bit().constData(), "Attributes constructs error", Continue);

    QVERIFY(!parser.parseFile(&file));

    QFile ref_file(file_name + ".ref");
    QVERIFY(ref_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString ref_file_contents = QString::fromUtf8(ref_file.readAll());

    QEXPECT_FAIL(QFINDTESTDATA("xmldocs/not-wf/sa/145.xml").toLocal8Bit().constData(), "Surrogate code point 0xD800 should be rejected", Continue);

    QCOMPARE(parser.result(), ref_file_contents);
}

void tst_QXmlSimpleReader::testIncrementalParsing_data()
{
    QTest::addColumn<QString>("file_name");
    QTest::addColumn<int>("chunkSize");

    const char * const good_data_dirs[] = {
        "xmldocs/valid/sa",
        "xmldocs/valid/not-sa",
        "xmldocs/valid/ext-sa",
        0
    };
    const char * const *d = good_data_dirs;

    QStringList good_file_list;
    for (; *d != 0; ++d)
        good_file_list += findXmlFiles(*d);

    for (int i=1; i<10; ++i) {
        QStringList::const_iterator it = good_file_list.begin();
        const QString skip49 = QFINDTESTDATA("xmldocs/valid/sa/049.xml");
        const QString skip50 = QFINDTESTDATA("xmldocs/valid/sa/050.xml");
        const QString skip51 = QFINDTESTDATA("xmldocs/valid/sa/051.xml");
        const QString skip52 = QFINDTESTDATA("xmldocs/valid/sa/052.xml");
        const QString skip89 = QFINDTESTDATA("xmldocs/valid/sa/089.xml");

        for (; it != good_file_list.end(); ++it) {
            if ( *it == skip89 )
                continue;// TODO: fails at the moment -- don't bother
            if ( i==1 && (
                        *it == skip49 ||
                        *it == skip50 ||
                        *it == skip51 ||
                        *it == skip52 ) ) {
                continue; // TODO: fails at the moment -- don't bother
            }
            QTest::addRow("%s %d", qPrintable(*it), i) << *it << i;
        }
    }
}

void tst_QXmlSimpleReader::testIncrementalParsing()
{
    QFETCH(QString, file_name);
    QFETCH(int, chunkSize);

    QFile file(file_name);
    QVERIFY(file.open(QIODevice::ReadOnly));

    Parser parser;
    QXmlInputSource source;
    bool first = true;
    while (!file.atEnd()) {
        source.setData(file.read(chunkSize));
        if (first) {
            QVERIFY(parser.parse(&source, true));
            first = false;
        } else {
            QVERIFY(parser.parseContinue());
        }
    }
    // detect end of document
    QVERIFY(parser.parseContinue());
    // parsing should fail after the end of the document was reached
    QVERIFY(!parser.parseContinue());

    QFile ref_file(file_name + ".ref");
    QVERIFY(ref_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString ref_file_contents = QString::fromUtf8(ref_file.readAll());

    QCOMPARE(parser.result(), ref_file_contents);
}

void tst_QXmlSimpleReader::setDataQString()
{
    QString input = inputString;
    QString ref = refString;

    QXmlInputSource source;
    Parser parser;

    source.setData(input);
    QVERIFY(parser.parse(&source,false));

    QBuffer resultBuffer;
    resultBuffer.setData(parser.result().toLatin1());

    QBuffer refBuffer;
    refBuffer.setData(ref.toLatin1());

    resultBuffer.open(QIODevice::ReadOnly);
    refBuffer.open(QIODevice::ReadOnly);

    bool success = true;
    while (resultBuffer.canReadLine()) {
        if (!refBuffer.canReadLine()) {
            success = false; break ;
        }
        if (resultBuffer.readLine().simplified() != refBuffer.readLine().simplified()) {
            success = false; break ;
        }
    }
    QVERIFY(success);
}

void tst_QXmlSimpleReader::inputFromQIODevice()
{
    QBuffer inputBuffer;
    inputBuffer.setData(inputString);

    QXmlInputSource source(&inputBuffer);
    Parser parser;

    QVERIFY(parser.parse(&source,false));

    QBuffer resultBuffer;
    resultBuffer.setData(parser.result().toLatin1());

    QBuffer refBuffer;
    refBuffer.setData(refString);

    resultBuffer.open(QIODevice::ReadOnly);
    refBuffer.open(QIODevice::ReadOnly);

    bool success = true;
    while (resultBuffer.canReadLine()) {
        if (!refBuffer.canReadLine()) {
            success = false; break ;
        }
        if (resultBuffer.readLine().simplified() != refBuffer.readLine().simplified()) {
            success = false; break ;
        }
    }
    QVERIFY(success);
}

void tst_QXmlSimpleReader::inputFromString()
{
    QString str = "<foo><bar>kake</bar><bar>ja</bar></foo>";
    QBuffer buff;
    buff.setData((char*)str.utf16(), int(str.size()*sizeof(ushort)));

    QXmlInputSource input(&buff);

    QXmlSimpleReader reader;
    QXmlDefaultHandler handler;
    reader.setContentHandler(&handler);

    QVERIFY(reader.parse(&input));
}

void tst_QXmlSimpleReader::inputFromSocket_data()
{
    const QStringList files = findXmlFiles(QLatin1String("encodings"));
    QVERIFY(files.size() > 0);

    QTest::addColumn<QString>("file_name");

    for (const QString &file_name : files)
        QTest::newRow(qPrintable(file_name)) << file_name;
}

void tst_QXmlSimpleReader::inputFromSocket()
{
    QFETCH(QString, file_name);

    if (!server->threadStarted.tryAcquire(1, 15000)) {
        // If something is wrong with QThreads, it's not a reason to fail
        // XML-test, we are not testing QThread here after all!
        QSKIP("XmlServer/thread has not started yet");
    }

    // Subsequent runs should be able to acquire the semaphore.
    server->threadStarted.release(1);

    if (!server->listening) {
        // Again, QTcpServer is not the subject of this test!
        QSKIP("QTcpServer::listen failed, bailing out");
    }

    QTcpSocket sock;
    sock.connectToHost(QHostAddress::LocalHost, TEST_PORT);
    QVERIFY2(sock.waitForConnected(),
             qPrintable(QStringLiteral("Cannot connect on port ") + QString::number(TEST_PORT)
                        + QStringLiteral(": ") + sock.errorString()));

    sock.write(file_name.toLocal8Bit() + "\n");
    QVERIFY(sock.waitForBytesWritten());

    QXmlInputSource input(&sock);

    QXmlSimpleReader reader;
    QXmlDefaultHandler handler;
    reader.setContentHandler(&handler);

    QVERIFY(reader.parse(&input));

//    qDebug() << "tst_QXmlSimpleReader::inputFromSocket(): success" << file_name;
}

void tst_QXmlSimpleReader::preserveCharacterReferences() const
{
    class Handler : public QXmlDefaultHandler
    {
    public:
        bool characters(const QString &chars) override
        {
            received = chars;
            return true;
        }

        QString received;
    };

    {
        QByteArray input("<e>A&#160;&#160;&#160;&#160;A</e>");

        QBuffer buff(&input);
        QXmlInputSource source(&buff);

        Handler h;
        QXmlSimpleReader reader;
        reader.setContentHandler(&h);
        QVERIFY(reader.parse(&source, false));

        QCOMPARE(h.received, QLatin1Char('A') + QString(4, QChar(160)) + QLatin1Char('A'));
    }

    {
        QByteArray input("<e>&#160;&#160;&#160;&#160;</e>");

        QBuffer buff(&input);
        QXmlInputSource source(&buff);

        Handler h;
        QXmlSimpleReader reader;
        reader.setContentHandler(&h);
        QVERIFY(reader.parse(&source, false));

        QCOMPARE(h.received, QString(4, QChar(160)));
    }
}

void tst_QXmlSimpleReader::reportNamespace() const
{
    class Handler : public QXmlDefaultHandler
    {
    public:
        bool startElement(const QString &namespaceURI,
                          const QString &localName,
                          const QString &qName,
                          const QXmlAttributes &) override
        {
            startNamespaceURI = namespaceURI;
            startLocalName = localName;
            startQName = qName;

            return true;
        }

        bool endElement(const QString &namespaceURI,
                        const QString &localName,
                        const QString &qName) override
        {
            endNamespaceURI = namespaceURI;
            endLocalName = localName;
            endQName = qName;

            return true;
        }

        QString startLocalName;
        QString startQName;
        QString startNamespaceURI;
        QString endLocalName;
        QString endQName;
        QString endNamespaceURI;
    };

    QXmlSimpleReader reader;
    Handler handler;
    reader.setContentHandler(&handler);

    QFETCH(QByteArray,  input);

    QBuffer buffer(&input);
    QVERIFY(buffer.open(QIODevice::ReadOnly));

    QXmlInputSource source(&buffer);
    QVERIFY(reader.parse(source));

    QFETCH(QString,     expectedQName);
    QFETCH(QString,     expectedLocalName);
    QFETCH(QString,     expectedNamespace);

    QCOMPARE(handler.startNamespaceURI, expectedNamespace);
    QCOMPARE(handler.startLocalName, expectedLocalName);
    QCOMPARE(handler.startQName, expectedQName);

    QCOMPARE(handler.endNamespaceURI, expectedNamespace);
    QCOMPARE(handler.endLocalName, expectedLocalName);
    QCOMPARE(handler.endQName, expectedQName);
}

void tst_QXmlSimpleReader::reportNamespace_data() const
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QString>("expectedQName");
    QTest::addColumn<QString>("expectedLocalName");
    QTest::addColumn<QString>("expectedNamespace");

    QTest::newRow("default ns") << QByteArray("<element xmlns='http://example.com/'/>")
                                << QString("element")
                                << QString("element")
                                << QString("http://example.com/");

    QTest::newRow("with prefix") << QByteArray("<p:element xmlns:p='http://example.com/'/>")
                                 << QString("p:element")
                                 << QString("element")
                                 << QString("http://example.com/");
}

QDomDocument tst_QXmlSimpleReader::fromByteArray(const QString &title, const QByteArray &ba, bool *ok)
{
    QDomDocument doc(title);
    *ok = bool(doc.setContent(ba, QDomDocument::ParseOption::UseNamespaceProcessing));
    return doc;
}

void tst_QXmlSimpleReader::roundtripWithNamespaces() const
{
    const char *const expected = "<element b:attr=\"value\" xmlns:a=\"http://www.example.com/A\" xmlns:b=\"http://www.example.com/B\" />\n";
    bool ok;

    {
        const char *const xml = "<element xmlns:b=\"http://www.example.com/B\" b:attr=\"value\" xmlns:a=\"http://www.example.com/A\"/>";

        const QDomDocument one(fromByteArray("document", xml, &ok));
        QVERIFY(ok);
        const QDomDocument two(fromByteArray("document2", one.toByteArray(2), &ok));
        QVERIFY(ok);

        QEXPECT_FAIL("", "Known problem, see 154573. The fix happens to break uic.", Abort);

        QCOMPARE(expected, one.toByteArray().constData());
        QCOMPARE(one.toByteArray(2).constData(), two.toByteArray(2).constData());
        QCOMPARE(two.toByteArray(2).constData(), two.toByteArray(2).constData());
    }

    {
        const char *const xml = "<element b:attr=\"value\" xmlns:b=\"http://www.example.com/B\" xmlns:a=\"http://www.example.com/A\"/>";

        const QDomDocument one(fromByteArray("document", xml, &ok));
        QVERIFY(ok);
        const QDomDocument two(fromByteArray("document2", one.toByteArray(2), &ok));
        QVERIFY(ok);

        QCOMPARE(expected, one.toByteArray().constData());
        QCOMPARE(one.toByteArray(2).constData(), two.toByteArray(2).constData());
        QCOMPARE(two.toByteArray(2).constData(), two.toByteArray(2).constData());
    }
}

class TestHandler : public QXmlDefaultHandler
{
public:
    TestHandler() :
        recursionCount(0)
    {
    }

    bool internalEntityDecl(const QString &name, const QString &value) override
    {
        ++recursionCount;
        return QXmlDefaultHandler::internalEntityDecl(name, value);
    }

    int recursionCount;
};

void tst_QXmlSimpleReader::dtdRecursionLimit()
{
    QFile file(QFINDTESTDATA("xmldocs/2-levels-nested-dtd.xml"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QXmlSimpleReader xmlReader;
    {
        QXmlInputSource source(&file);
        TestHandler handler;
        xmlReader.setDeclHandler(&handler);
        xmlReader.setErrorHandler(&handler);
        QVERIFY(!xmlReader.parse(&source));
    }

    file.close();
    file.setFileName(QFINDTESTDATA("xmldocs/1-levels-nested-dtd.xml"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    {
        QXmlInputSource source(&file);
        TestHandler handler;
        xmlReader.setDeclHandler(&handler);
        xmlReader.setErrorHandler(&handler);
        QVERIFY(!xmlReader.parse(&source));
        // The error wasn't because of the recursion limit being reached,
        // it was because the document is not valid.
        QVERIFY(handler.recursionCount < 2);
    }

    file.close();
    file.setFileName(QFINDTESTDATA("xmldocs/internal-entity-polynomial-attribute.xml"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    {
        QXmlInputSource source(&file);
        TestHandler handler;
        xmlReader.setDeclHandler(&handler);
        xmlReader.setErrorHandler(&handler);
        QVERIFY(!xmlReader.parse(&source));
        // The entity sizes are checked when the entities are expanded, so all
        // four entities are declared before the expansion in the attribute fails.
        QCOMPARE(handler.recursionCount, 4);
    }
}

class DepthMeasureHandler : public QXmlDefaultHandler
{
public:
    bool startElement(const QString &, const QString &, const QString &,
                      const QXmlAttributes &) override
    {
        maxDepth = qMax(maxDepth, ++depth);
        return true;
    }

    bool endElement(const QString &, const QString &, const QString &) override
    {
        --depth;
        return true;
    }

    bool fatalError(const QXmlParseException &exception) override
    {
        errorMessage = exception.message();
        return false;
    }

    int depth = 0;
    int maxDepth = 0;
    QString errorMessage;
};

void tst_QXmlSimpleReader::deepElementNesting_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<int>("expectedDepth");

    constexpr int depth = 200000;

    {
        QByteArray xml;
        xml.reserve(depth * 7);
        for (int i = 0; i < depth; ++i)
            xml += "<a>";

        for (int i = 0; i < depth; ++i)
            xml += "</a>";

        QTest::newRow("closed tags") << xml << depth;
    }
    {
        QByteArray xml;
        xml += "<n:a xmlns:n='http://example.com/nesting'>";
        xml.reserve(xml.size() + depth * 11);
        for (int i = 0; i < depth - 1; ++i) // the outermost element is a level too
            xml += "<n:a>";
        for (int i = 0; i < depth; ++i)
            xml += "</n:a>";

        QTest::newRow("namespace prefixed tags") << xml << depth;
    }
    {
        constexpr char level[] = "<a>text&amp;<!-- comment --><?pi data?><![CDATA[<]]>";
        qsizetype levelLen = qsizetype(strlen(level));

        QByteArray xml;
        xml.reserve(depth * (levelLen + 4));
        for (int i = 0; i < depth; ++i)
            xml += level;
        for (int i = 0; i < depth; ++i)
            xml += "</a>";

        QTest::newRow("mixed content") << xml << depth;
    }
    {
        QByteArray xml;
        xml += "<!DOCTYPE a [<!ELEMENT a ";
        xml.reserve(xml.size() + depth * 2);
        xml.resize(xml.size() + depth, '(');
        xml += 'b';
        xml.resize(xml.size() + depth, ')');
        xml += ">]><a/>";

        QTest::newRow("DTD nesting") << xml << 1;
    }
}

void tst_QXmlSimpleReader::deepElementNesting()
{
    QFETCH(QByteArray, xml);
    QFETCH(const int, expectedDepth);

    enum class CallType
    {
        Ref,
        Pointer,
        TwoArg,
    };

    constexpr CallType types[] = { CallType::Ref, CallType::Pointer, CallType::TwoArg };

    for (CallType type : types) {
        auto guard = qScopeGuard([type] {
            qDebug("Failed while handling %s parse() overload",
                   type == CallType::Ref ?     "const ref" :
                   type == CallType::Pointer ? "pointer" :
                   /* else */                  "two arg");
        });

        QBuffer buffer(&xml);
        QVERIFY(buffer.open(QIODevice::ReadOnly));
        QXmlInputSource source(&buffer);

        DepthMeasureHandler handler;
        QXmlSimpleReader reader;
        reader.setContentHandler(&handler);
        reader.setErrorHandler(&handler);
        reader.setLexicalHandler(&handler);

        switch (type) {
        case CallType::Ref:
            QVERIFY(reader.parse(source));
            break;
        case CallType::Pointer:
            QVERIFY(reader.parse(&source));
            break;
        case CallType::TwoArg:
            QVERIFY(reader.parse(&source, false));
            break;
        }

        QVERIFY(handler.errorMessage.isEmpty());
        QCOMPARE(handler.maxDepth, expectedDepth);
        QCOMPARE(handler.depth, 0);

        guard.dismiss();
    }
}

void tst_QXmlSimpleReader::deepElementNestingIncremental_data()
{
    deepElementNesting_data();
}

void tst_QXmlSimpleReader::deepElementNestingIncremental()
{
    QFETCH(QByteArray, xml);
    QFETCH(const int, expectedDepth);

    DepthMeasureHandler handler;
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    reader.setLexicalHandler(&handler);

    const qsizetype chunkSize = xml.size() / 10;
    QVERIFY(chunkSize > 0);

    QXmlInputSource source;
    bool first = true;
    for (qsizetype pos = 0; pos < xml.size(); pos += chunkSize) {
        source.setData(xml.mid(pos, chunkSize));
        // In incremental mode running out of data is reported as success, so a
        // false here would be a real parse error.
        if (first) {
            QVERIFY(reader.parse(&source, true));
            first = false;
        } else {
            QVERIFY(reader.parseContinue());
        }
    }
    // detect end of document
    QVERIFY(reader.parseContinue());

    QVERIFY(handler.errorMessage.isEmpty());
    QCOMPARE(handler.maxDepth, expectedDepth);
    QCOMPARE(handler.depth, 0);
}

void tst_QXmlSimpleReader::valueLongerThanIntMaxDoesNotCrash()
{
    if constexpr (QT_POINTER_SIZE == 4)
        QSKIP("An allocated QString does not fit into a 32 bit address space.");

    if (qgetenv("QTEST_ENVIRONMENT").split(' ').contains("ci"))
        QSKIP("Too slow and consumes at least 4 GiB of memory - skipping on CI.");


    // Serves prefix + runLength copies of runChar + suffix in chunks to avoid
    // storing a multi-Gb input
    class RepeatedRunDevice : public QIODevice
    {
    public:
        RepeatedRunDevice(const QByteArray &prefix, qint64 runLength, const QByteArray &suffix,
                          char runChar = 'a')
            : m_prefix(prefix), m_suffix(suffix), m_runLength(runLength), m_runChar(runChar)
        {
        }

        qint64 size() const override { return m_prefix.size() + m_runLength + m_suffix.size(); }

    protected:
        qint64 readData(char *data, qint64 maxSize) override
        {
            const qint64 runStart = m_prefix.size();
            const qint64 suffixStart = runStart + m_runLength;
            qint64 offset = pos();
            qint64 written = 0;

            while (written < maxSize && offset < size()) {
                const char *source = nullptr;
                qint64 available = 0;
                if (offset < runStart) {
                    source = m_prefix.constData() + offset;
                    available = runStart - offset;
                } else if (offset < suffixStart) {
                    available = suffixStart - offset;
                } else {
                    source = m_suffix.constData() + (offset - suffixStart);
                    available = size() - offset;
                }

                const qint64 chunk = qMin(available, maxSize - written);
                if (source)
                    memcpy(data + written, source, chunk);
                else
                    memset(data + written, m_runChar, chunk);
                written += chunk;
                offset += chunk;
            }

            return written;
        }

        qint64 writeData(const char *, qint64) override { return -1; }

    private:
        QByteArray m_prefix;
        QByteArray m_suffix;
        qint64 m_runLength;
        char m_runChar;
    };

    class CharacterCountingHandler : public QXmlDefaultHandler
    {
    public:
        bool characters(const QString &ch) override
        {
            characterCount += ch.size();
            return true;
        }

        bool fatalError(const QXmlParseException &exception) override
        {
            errorMessage = exception.message();
            return false;
        }

        qsizetype characterCount = 0;
        QString errorMessage;
    };

    // The buffer is flushed every 256 characters, so a handful of characters
    // past INT_MAX is enough to be sure the boundary is crossed.
    constexpr auto runLength = std::numeric_limits<int>::max() + qint64{10};

    CharacterCountingHandler handler;
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    QT_TRY {
        RepeatedRunDevice device("<d>", runLength, "</d>");
        QVERIFY(device.open(QIODevice::ReadOnly));
        QXmlInputSource source(&device);
        QVERIFY2(reader.parse(source), qPrintable(handler.errorMessage));
        QCOMPARE(handler.characterCount, runLength);
    } QT_CATCH (const std::bad_alloc &) {
        QSKIP("Not enough memory to accumulate INT_MAX characters.");
    }
}

constexpr qsizetype EntityCharacterLimit = 4096;

void tst_QXmlSimpleReader::entityExpansionLimit_data()
{
    QTest::addColumn<QByteArray>("xml");

    constexpr qsizetype OversizedLiteralSize = EntityCharacterLimit + 1;
    constexpr qsizetype ExpandedSize = 16 * EntityCharacterLimit;

    // Literal entity in content is larger than the expansion limit
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e '" + QByteArray(OversizedLiteralSize, 'A') + "'>]>"
                "<d>&e;</d>";

        QTest::newRow("literal_entity_in_content") << xml;
    }

    // Literal entity in attribute value is larger than the expansion limit
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e '" + QByteArray(OversizedLiteralSize, 'A') + "'>]>"
                "<d a='&e;'/>";

        QTest::newRow("literal_entity_in_attribute_value") << xml;
    }

    constexpr qsizetype NestedReferences = 2;

    // e2 references e1 multiple times. The total expanded size of e2 exceeds the limit.
    // e2 is used in the content.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e1 '" + QByteArray(EntityCharacterLimit, 'A') + "'>"
                "<!ENTITY e2 '" + QByteArray("&e1;").repeated(NestedReferences) + "'>]>"
                "<d>&e2;</d>";

        QTest::newRow("two_declarations_in_content") << xml;
    }

    // e2 references e1 multiple times. The total expanded size of e2 exceeds the limit.
    // e2 is used in the attribute value.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e1 '" + QByteArray(EntityCharacterLimit, 'A') + "'>"
                "<!ENTITY e2 '" + QByteArray("&e1;").repeated(NestedReferences) + "'>]>"
                "<d a='&e2;'/>";

        QTest::newRow("two_declarations_in_attribute_value") << xml;
    }

    constexpr qsizetype ChainReferences = 2;
    constexpr qsizetype ChainLiteralSize = EntityCharacterLimit / 4;

    // The entities are declared in the reverse order. Verifies that we properly
    // re-evaluate the expanded sizes once new entities are parsed.
    {
        const QByteArray xml =
                "<!DOCTYPE d ["
                "<!ENTITY e4 '" + QByteArray("&e3;").repeated(ChainReferences) + "'>"
                "<!ENTITY e3 '" + QByteArray("&e2;").repeated(ChainReferences) + "'>"
                "<!ENTITY e2 '" + QByteArray("&e1;").repeated(ChainReferences) + "'>"
                "<!ENTITY e1 '" + QByteArray(ChainLiteralSize, 'A') + "'>"
                "]><d>&e4;</d>";

        QTest::newRow("reverse_declaration_order") << xml;
    }

    // Parameter entity with oversized literal size
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY % a '" + QByteArray(OversizedLiteralSize, 'A') + "'>"
                "<!ENTITY % b '%a;'>"
                "]><d/>";

        QTest::newRow("parameter_entity_in_DTD") << xml;
    }

    // Parameter entity b references parameter entity a multiple times. The total
    // expanded size of b exceeds the limit.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY % a '" + QByteArray(EntityCharacterLimit, 'A') + "'>"
                "<!ENTITY % b '"
                + QByteArray("%a;").repeated(ExpandedSize / EntityCharacterLimit)
                + "'>]><d/>";

        QTest::newRow("parameter_entity_run_in_DTD") << xml;
    }

    // Same as above, but the parameter entity b expands into a huge string in
    // memory. Needed to test that we properly limit the expansion while
    // substituting parameter entites, rather than check the full size of expanded
    // string afterwards.
    {
        constexpr qsizetype RunawayReferences = 1'000'000;

        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY % a '" + QByteArray(EntityCharacterLimit, 'A') + "'>"
                "<!ENTITY % b '"
                + QByteArray("%a;").repeated(RunawayReferences)
                + "'>]><d/>";

        QTest::newRow("parameter_entity_run_in_DTD_huge_expansion") << xml;
    }

    // Entity e references parameter entity a multiple times. The total expanded
    // size of e exceeds the limit, and parameter entity a itself exceeds the limit.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY % a '" + QByteArray(OversizedLiteralSize, 'A') + "'>"
                "<!ENTITY e '"
                + QByteArray("%a;").repeated(ExpandedSize / OversizedLiteralSize)
                +"'>]><d/>";

        QTest::newRow("parameter_entity_in_entity_value") << xml;
    }

    // The parameter entity a has references to general entity e1 in its value.
    // The expansion limit is exceeded only when the parameter entity a is used
    // in another general entity e2.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e1 '" + QByteArray(EntityCharacterLimit, 'A') + "'>"
                "<!ENTITY % a '" + QByteArray("&e1;").repeated(NestedReferences) + "'>"
                "<!ENTITY e2 '%a;'>]>"
                "<d>&e2;</d>";

        QTest::newRow("entity_refs_in_parameter_entity") << xml;
    }

    // The character reference "&#37;" is expanded to '%' when the value of the
    // parameter entity b is parsed, so b is stored as "%a;%a;", and fits into
    // the limit. When b is included in the DTD, its value is parsed again, and
    // every "%a;" includes the value of a. The parameter entity a contains
    // only spaces, so that it can be included in the DTD.
    // The expansion limit is exceeded once b is included in the DTD.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY % a '" + QByteArray(EntityCharacterLimit, ' ') + "'>"
                "<!ENTITY % b '" + QByteArray("&#37;a;").repeated(NestedReferences) + "'>"
                "%b;]><d/>";

        QTest::newRow("parameter_entity_char_ref_in_DTD") << xml;
    }

    // The parameter entity p contains an ATTLIST declaration with references to
    // the general entity e in the default value. The references are bypassed
    // when p is declared, so p fits into the limit. When p is included in the
    // DTD, the default value is parsed, and the references to e are expanded.
    // The expansion limit is exceeded once p is included in the DTD.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e '" + QByteArray(EntityCharacterLimit, 'A') + "'>"
                "<!ENTITY % p \"<!ATTLIST d a CDATA '"
                + QByteArray("&e;").repeated(NestedReferences) + "'>\">"
                "%p;]><d/>";

        QTest::newRow("general_entity_in_parameter_entity_in_DTD") << xml;
    }

    // Expansion of standard XML entities.
    // e1 contains multiple &amp; references, but fits into EntityCharacterLimit.
    // e2 is a repetition of e1 that exceeds the limit.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e1 '"
                + QByteArray("&amp;").repeated(EntityCharacterLimit / 5) + "'>"
                "<!ENTITY e2 '" + QByteArray("&e1;").repeated(NestedReferences) + "'>]>"
                "<d>&e2;</d>";

        QTest::newRow("standard_entities") << xml;
    }

    // Expansion of numeric character references.
    // The string "&#38;#65;" is first transformed into "&#65;" (when &#38; is
    // parsed as '&'), and that value is only replaced with 'A' in content.
    // e1 contains multiple "&#38;#65;" entries, but fits into
    // EntityCharacterLimit.
    // e2 is a repetition of e1 that exceeds the limit.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e1 '"
                + QByteArray("&#38;#65;").repeated(EntityCharacterLimit / 5) + "'>"
                "<!ENTITY e2 '" + QByteArray("&e1;").repeated(NestedReferences) + "'>]>"
                "<d>&e2;</d>";

        QTest::newRow("numeric_character_references") << xml;
    }

    // Expansion with unresolved entity.
    // The entity e1 contains EntityCharacterLimit valid characters + a reference
    // to an unknown entity u.
    // The entity e2 simply uses e1.
    // e1 itself already exceeds the limit, because an unknown entity reference
    // is treated as a literal.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e1 '"
                + QByteArray(EntityCharacterLimit, 'A') + "&u;'>"
                "<!ENTITY e2 '&e1;'>]>"
                "<d>&e2;</d>";

        QTest::newRow("unknown_entity") << xml;
    }

    // Expansion with unresolved entity in ATTLIST.
    // The entity e1 contains (EntityCharacterLimit - 3) characters + a reference
    // to an unknown entity u. So, it's fine on its own.
    // The entity e2 references e1 twice, so it exceeds the limit.
    // The ATTLIST expands entity e2.
    // The test should fail during the e2 entity expansion in ATTLIST, so the
    // attributeDecl() callback of the handler should never be called.
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e1 '"
                + QByteArray(EntityCharacterLimit - 3, 'A') + "&u;'>"
                "<!ENTITY e2 '&e1;&e1;'>"
                "<!ATTLIST d a CDATA '&e2;'>"
                "]><d/>";

        QTest::newRow("unknown_entity_in_attlist") << xml;
    }

    // Expansion when one of the entities contains an empty value.
    // Verifies that this empty value does not invalidate the calculations.
    // Entity e0 has an empty value.
    // Entity e1 has EntityCharacterLimit characters + a reference to e0 - it's
    // still fine.
    // Entity e2 references e1 two times - it should exceed the limit
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e0 ''>"
                "<!ENTITY e1 '" + QByteArray(EntityCharacterLimit, 'A') + "&e0;'>"
                "<!ENTITY e2 '&e1;&e1;'>]>"
                "<d>&e2;</d>";

        QTest::newRow("empty_entity") << xml;
    }

    // Expansion with entity reference loop.
    // Should be handled by the parser when substituting the entity in content
    {
        const QByteArray xml =
                "<!DOCTYPE d [<!ENTITY e1 '&e2;'>"
                "<!ENTITY e2 '&e1;'>]>"
                "<d>&e2;</d>";

        QTest::newRow("entity_loop") << xml;
    }
}

void tst_QXmlSimpleReader::entityExpansionLimit()
{
    // Counts every expanded character the parser hands out, wherever it hands it
    // out: as character data, as an attribute value, or as the value of a declared
    // entity.
    class ExpansionHandler : public QXmlDefaultHandler
    {
    public:
        bool startElement(const QString &, const QString &, const QString &,
                          const QXmlAttributes &atts) override
        {
            for (int i = 0; i < atts.length(); ++i)
                expandedCharacters += atts.value(i).size();
            return true;
        }

        bool characters(const QString &ch) override
        {
            expandedCharacters += ch.size();
            return true;
        }

        bool internalEntityDecl(const QString &, const QString &value) override
        {
            expandedCharacters += value.size();
            return true;
        }

        bool fatalError(const QXmlParseException &exception) override
        {
            errorMessage = exception.message();
            return false;
        }

        bool attributeDecl(const QString &, const QString &, const QString &,
                           const QString &, const QString &) override
        {
            attDeclCalled = true;
            return true;
        }

        qsizetype expandedCharacters = 0;
        QString errorMessage;
        bool attDeclCalled = false;
    };

    static constexpr qsizetype expansionSanityBound = EntityCharacterLimit * 2;

    QFETCH(QByteArray, xml);

    QBuffer buffer(&xml);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QXmlInputSource source(&buffer);

    ExpansionHandler handler;
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    reader.setDeclHandler(&handler);

    QVERIFY(!reader.parse(source));
    QVERIFY(!handler.errorMessage.isEmpty());
    QVERIFY(!handler.attDeclCalled);
    QVERIFY2(handler.expandedCharacters <= expansionSanityBound,
             qPrintable(QString::fromLatin1("Expanded %1 characters, which is more than the "
                                            "sanity bound of %2.")
                                .arg(handler.expandedCharacters)
                                .arg(expansionSanityBound)));
}

void tst_QXmlSimpleReader::entityExpansionLimitEntityResolver_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<bool>("expectSuccess");

    // The internal entity "outer" references the external parsed entity
    // "ext", which in turn references the internal entity "e1" twice.
    // The references to e1 come from the text of the external entity, so they
    // were not counted as a part of the value of "outer", and the full size of
    // e1 has to be added for each of them.
    // The expanded size of "outer" is 5 ("&ext;") + 2 * e1.size()

    const QByteArray xmlStart =
            "<!DOCTYPE d ["
            "<!ENTITY ext SYSTEM 'ext.ent'>"
            "<!ENTITY e1 '";

    const QByteArray withinLimit = QByteArray(EntityCharacterLimit / 2 - 3, 'A');
    const QByteArray aboveLimit = QByteArray(EntityCharacterLimit / 2, 'A');

    const QByteArray xmlEnd =
            "'>"
            "<!ENTITY outer '&ext;'>"
            "]><d>&outer;</d>";

    // 5 + 2 * (EntityCharacterLimit / 2 - 3) == EntityCharacterLimit - 1
    QTest::newRow("within_limit") << xmlStart + withinLimit + xmlEnd << true;

    // 5 + 2 * (EntityCharacterLimit / 2) == EntityCharacterLimit + 5.
    QTest::newRow("exceeds_limit") << xmlStart + aboveLimit + xmlEnd << false;
}

void tst_QXmlSimpleReader::entityExpansionLimitEntityResolver()
{
    class ExternalEntityHandler : public QXmlDefaultHandler
    {
    public:
        explicit ExternalEntityHandler(const QByteArray &externalEntity)
            : externalEntityData(externalEntity)
        {
        }

        bool resolveEntity(const QString &, const QString &systemId,
                           QXmlInputSource *&ret) override
        {
            ++resolveCount;
            if (systemId == QLatin1String("ext.ent")) {
                externalEntityBuffer.close();
                externalEntityBuffer.setData(externalEntityData);
                externalEntityBuffer.open(QIODevice::ReadOnly);
                ret = new QXmlInputSource(&externalEntityBuffer);
            }
            return true;
        }

        bool characters(const QString &ch) override
        {
            expandedCharacters += ch.size();
            return true;
        }

        bool fatalError(const QXmlParseException &exception) override
        {
            errorMessage = exception.message();
            return false;
        }

        QByteArray externalEntityData;
        QBuffer externalEntityBuffer;
        int resolveCount = 0;
        qsizetype expandedCharacters = 0;
        QString errorMessage;
    };

    QFETCH(QByteArray, xml);
    QFETCH(const bool, expectSuccess);

    QBuffer buffer(&xml);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QXmlInputSource source(&buffer);

    ExternalEntityHandler handler("&e1;&e1;");
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    reader.setEntityResolver(&handler);

    QCOMPARE(reader.parse(source), expectSuccess);
    QCOMPARE(handler.errorMessage.isEmpty(), expectSuccess);
    QCOMPARE_LE(handler.expandedCharacters, EntityCharacterLimit);
    QCOMPARE(handler.resolveCount, 1);
}

void tst_QXmlSimpleReader::entityExpansionProperty_data()
{
    QTest::addColumn<qint64>("limit");
    QTest::addColumn<qint64>("expectedLimit");
    QTest::addColumn<int>("entityLength");
    QTest::addColumn<bool>("wellFormed");

    QTest::newRow("large_limit_valid_length") << qint64(8192) << qint64(8192) << 8000 << true;
    QTest::newRow("small_limit_invalid_length") << qint64(32) << qint64(32) << 64 << false;
    QTest::newRow("zero_limit_uses_default") << qint64(0) << qint64(4096) << 5000 << false;
    QTest::newRow("negative_limit_uses_default") << qint64(-1) << qint64(4096) << 4000 << true;
    if constexpr (sizeof(qintptr) > sizeof(int)) {
        QTest::newRow("too_large_limit_uses_default")
                << qint64(std::numeric_limits<int>::max()) + 1 << qint64(4096) << 5000 << false;
    }
}

void tst_QXmlSimpleReader::entityExpansionProperty()
{
    QFETCH(const qint64, limit);
    QFETCH(const qint64, expectedLimit);
    QFETCH(const int, entityLength);
    QFETCH(const bool, wellFormed);

    constexpr const char *propName = "http://qt-project.org/xml/properties/entity-expansion-limit";

    QXmlSimpleReader reader;

    QVERIFY(reader.hasProperty(QLatin1String(propName)));

    bool ok = false;
    void *value = reader.property(QLatin1String(propName), &ok);
    QVERIFY(ok);
    QCOMPARE(reinterpret_cast<qintptr>(value), qintptr(4096));

    reader.setProperty(QLatin1String(propName), reinterpret_cast<void *>(qintptr(limit)));

    // verify the limit
    ok = false;
    value = reader.property(QLatin1String(propName), &ok);
    QVERIFY(ok);
    QCOMPARE(reinterpret_cast<qintptr>(value), qintptr(expectedLimit));

    QByteArray xml =
            "<!DOCTYPE d [<!ENTITY e '" + QByteArray(entityLength, 'A') + "'>]><d>&e;</d>";

    QBuffer buffer(&xml);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QXmlInputSource source(&buffer);

    QCOMPARE(reader.parse(source), wellFormed);
}

QTEST_MAIN(tst_QXmlSimpleReader)
#include "tst_qxmlsimplereader.moc"
