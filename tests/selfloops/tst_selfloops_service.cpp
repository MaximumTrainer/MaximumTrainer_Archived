/*
 * tst_selfloops_service.cpp
 *
 * Qt Test suite for SelfloopsService.
 *
 * Test groups
 * ──────────────────────────────────────────────────────────────────
 * URL            – upload targets the correct Selfloops endpoint
 * HTTP method    – uploadActivity uses POST
 * Null manager   – uploadActivity returns nullptr when NetworkManagerWS absent
 * Live (skipped) – optional real upload test via SELFLOOPS_EMAIL / SELFLOOPS_PW
 */

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QTemporaryFile>

#include "selfloops_service.h"

// ─────────────────────────────────────────────────────────────────────────────
// FinishedReply stub
// ─────────────────────────────────────────────────────────────────────────────
class FinishedReply : public QNetworkReply
{
    Q_OBJECT
public:
    explicit FinishedReply(const QNetworkRequest &req,
                           QNetworkAccessManager::Operation op,
                           QObject *parent = nullptr)
        : QNetworkReply(parent)
    {
        setRequest(req);
        setUrl(req.url());
        setOperation(op);
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
        setFinished(true);
        QTimer::singleShot(0, this, &FinishedReply::finished);
    }

    qint64 bytesAvailable() const override { return 0; }
    bool   isSequential()   const override { return true; }
    void   abort()                override {}

protected:
    qint64 readData(char *, qint64) override { return -1; }
};

// ─────────────────────────────────────────────────────────────────────────────
// MockNetworkAccessManager
// ─────────────────────────────────────────────────────────────────────────────
class MockNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit MockNetworkAccessManager(QObject *parent = nullptr)
        : QNetworkAccessManager(parent) {}

    QNetworkRequest lastRequest;
    QNetworkAccessManager::Operation lastOp = QNetworkAccessManager::GetOperation;
    int callCount = 0;

    void reset()
    {
        callCount   = 0;
        lastRequest = QNetworkRequest();
        lastOp      = QNetworkAccessManager::GetOperation;
    }

protected:
    QNetworkReply *createRequest(Operation op,
                                 const QNetworkRequest &request,
                                 QIODevice *) override
    {
        lastRequest = request;
        lastOp      = op;
        ++callCount;
        return new FinishedReply(request, op, this);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
class TstSelfloopsService : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    // ── URL ─────────────────────────────────────────────────────────────────
    void testUpload_url();

    // ── HTTP method ─────────────────────────────────────────────────────────
    void testUpload_isPost();

    // ── Null manager guard ──────────────────────────────────────────────────
    void testNullManager_returnsNullptr();

    // ── Optional live test ──────────────────────────────────────────────────
    void testLive_uploadActivity();

private:
    MockNetworkAccessManager *m_manager = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
void TstSelfloopsService::initTestCase()
{
    m_manager = new MockNetworkAccessManager(this);
    qApp->setProperty("NetworkManagerWS",
                      QVariant::fromValue<QNetworkAccessManager *>(m_manager));
}

void TstSelfloopsService::cleanupTestCase()
{
    qApp->setProperty("NetworkManagerWS", QVariant());
}

void TstSelfloopsService::init()
{
    m_manager->reset();
}

// ─────────────────────────────────────────────────────────────────────────────
static QString makeTempFitFile()
{
    // SelfloopsService gzip-compresses the file; it must exist and be readable.
    QTemporaryFile f(QDir::tempPath() + "/tst_XXXXXX.fit");
    f.setAutoRemove(false);
    f.open();
    f.write(QByteArray(16, 0)); // minimal non-empty content
    f.close();
    return f.fileName();
}

// ─────────────────────────────────────────────────────────────────────────────
// URL test
// ─────────────────────────────────────────────────────────────────────────────

void TstSelfloopsService::testUpload_url()
{
    const QString tmp = makeTempFitFile();

    SelfloopsService svc;
    svc.setCredentials("test@example.com", "password");
    QNetworkReply *reply = svc.uploadActivity(tmp, "test note");
    QVERIFY(reply != nullptr);
    reply->deleteLater();
    QFile::remove(tmp);
    QFile::remove(tmp + ".gz");

    const QString url = m_manager->lastRequest.url().toString();
    QVERIFY2(url.contains(QStringLiteral("selfloops.com/restapi/maximumtrainer")),
             qPrintable("Expected Selfloops upload URL, got: " + url));
    QVERIFY(url.contains(QStringLiteral("upload.json")));
}

// ─────────────────────────────────────────────────────────────────────────────
// HTTP method test
// ─────────────────────────────────────────────────────────────────────────────

void TstSelfloopsService::testUpload_isPost()
{
    const QString tmp = makeTempFitFile();

    SelfloopsService svc;
    svc.setCredentials("test@example.com", "password");
    QNetworkReply *reply = svc.uploadActivity(tmp, "note");
    QVERIFY(reply != nullptr);
    reply->deleteLater();
    QFile::remove(tmp);
    QFile::remove(tmp + ".gz");

    QCOMPARE(m_manager->lastOp, QNetworkAccessManager::PostOperation);
}

// ─────────────────────────────────────────────────────────────────────────────
// Null manager guard
// ─────────────────────────────────────────────────────────────────────────────

void TstSelfloopsService::testNullManager_returnsNullptr()
{
    qApp->setProperty("NetworkManagerWS", QVariant());

    SelfloopsService svc;
    svc.setCredentials("e@example.com", "pw");

    const QString tmp = makeTempFitFile();
    QCOMPARE(svc.uploadActivity(tmp, "note"), nullptr);
    QFile::remove(tmp);
    QFile::remove(tmp + ".gz");

    qApp->setProperty("NetworkManagerWS",
                      QVariant::fromValue<QNetworkAccessManager *>(m_manager));
}

// ─────────────────────────────────────────────────────────────────────────────
// Optional live test
// ─────────────────────────────────────────────────────────────────────────────

void TstSelfloopsService::testLive_uploadActivity()
{
    const QString email    = qEnvironmentVariable("SELFLOOPS_EMAIL");
    const QString password = qEnvironmentVariable("SELFLOOPS_PW");
    if (email.isEmpty() || password.isEmpty())
        QSKIP("Set SELFLOOPS_EMAIL and SELFLOOPS_PW to run this live test");

    // Look for a fixture .fit file; fall back to a temp file if not present.
    const QString fixturePath =
        QString::fromLatin1(__FILE__) + QStringLiteral("/../../fixtures/sample.fit");
    const QString filePath = QFile::exists(fixturePath)
                                 ? fixturePath
                                 : makeTempFitFile();

    QNetworkAccessManager realMgr;
    qApp->setProperty("NetworkManagerWS",
                      QVariant::fromValue<QNetworkAccessManager *>(&realMgr));

    SelfloopsService svc;
    svc.setCredentials(email, password);

    QNetworkReply *reply = svc.uploadActivity(filePath, "MaximumTrainer test upload");
    QVERIFY(reply != nullptr);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QVERIFY2(status == 200 || status == 400,
             qPrintable("Unexpected HTTP status from Selfloops: " + QString::number(status)));

    reply->deleteLater();
    qApp->setProperty("NetworkManagerWS",
                      QVariant::fromValue<QNetworkAccessManager *>(m_manager));
    QFile::remove(filePath + ".gz");
}

// ─────────────────────────────────────────────────────────────────────────────
QTEST_MAIN(TstSelfloopsService)
#include "tst_selfloops_service.moc"
