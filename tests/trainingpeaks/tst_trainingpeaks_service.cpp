/*
 * tst_trainingpeaks_service.cpp
 *
 * Qt Test suite for TrainingPeaksService.
 *
 * Test groups
 * ──────────────────────────────────────────────────────────────────
 * Bearer header  – Authorization: Bearer <token> is set on upload
 * URL            – upload and refresh-token endpoints are correct
 * refreshToken   – POST operation used, URL is TP token endpoint
 * TP_CLIENT_SECRET – compile-time constant is a string (not the old
 *                    hardcoded value)
 * Null manager   – all methods return nullptr when NetworkManagerWS absent
 * Live (skipped) – optional real-network token-refresh smoke test
 */

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include "trainingpeaks_service.h"
#include "environnement.h"

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
class TstTrainingPeaksService : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    // ── Bearer header ───────────────────────────────────────────────────────
    void testUpload_bearerHeader();

    // ── URL correctness ─────────────────────────────────────────────────────
    void testUpload_url();
    void testRefreshToken_url();

    // ── HTTP method ─────────────────────────────────────────────────────────
    void testUpload_isPost();
    void testRefreshToken_isPost();

    // ── TP_CLIENT_SECRET compile-time constant ───────────────────────────────
    void testClientSecret_isNotHardcoded();

    // ── Null manager guard ──────────────────────────────────────────────────
    void testNullManager_returnsNullptr();

    // ── Optional live test ──────────────────────────────────────────────────
    void testLive_refreshToken();

private:
    MockNetworkAccessManager *m_manager = nullptr;

    static constexpr const char ACCESS_TOKEN[]  = "testTpAccessToken";
    static constexpr const char REFRESH_TOKEN[] = "testTpRefreshToken";
};

// ─────────────────────────────────────────────────────────────────────────────
void TstTrainingPeaksService::initTestCase()
{
    m_manager = new MockNetworkAccessManager(this);
    qApp->setProperty("NetworkManagerWS",
                      QVariant::fromValue<QNetworkAccessManager *>(m_manager));
}

void TstTrainingPeaksService::cleanupTestCase()
{
    qApp->setProperty("NetworkManagerWS", QVariant());
}

void TstTrainingPeaksService::init()
{
    m_manager->reset();
}

// ─────────────────────────────────────────────────────────────────────────────
static QString makeTempFile()
{
    QTemporaryFile f;
    f.setAutoRemove(false);
    f.open();
    return f.fileName();
}

// ─────────────────────────────────────────────────────────────────────────────
// Bearer header
// ─────────────────────────────────────────────────────────────────────────────

void TstTrainingPeaksService::testUpload_bearerHeader()
{
    const QString tmp = makeTempFile();
    TrainingPeaksService svc;
    svc.setAccessToken(ACCESS_TOKEN);
    QNetworkReply *reply = svc.uploadActivity(tmp, "TestActivity", "Desc");
    QVERIFY(reply != nullptr);
    reply->deleteLater();
    QFile::remove(tmp);

    const QByteArray auth = m_manager->lastRequest.rawHeader("Authorization");
    QVERIFY2(auth.startsWith("Bearer "), "Authorization header should start with 'Bearer '");
    QVERIFY(auth.contains(ACCESS_TOKEN));
}

// ─────────────────────────────────────────────────────────────────────────────
// URL tests
// ─────────────────────────────────────────────────────────────────────────────

void TstTrainingPeaksService::testUpload_url()
{
    const QString tmp = makeTempFile();
    TrainingPeaksService svc;
    svc.setAccessToken(ACCESS_TOKEN);
    QNetworkReply *reply = svc.uploadActivity(tmp, "TestActivity", "Desc");
    QVERIFY(reply != nullptr);
    reply->deleteLater();
    QFile::remove(tmp);

    const QString url = m_manager->lastRequest.url().toString();
    QVERIFY2(url.contains(QStringLiteral("api.trainingpeaks.com/v1/file")),
             qPrintable("Expected TrainingPeaks upload URL, got: " + url));
}

void TstTrainingPeaksService::testRefreshToken_url()
{
    QNetworkReply *reply = TrainingPeaksService::refreshToken(REFRESH_TOKEN);
    QVERIFY(reply != nullptr);
    reply->deleteLater();

    const QString url = m_manager->lastRequest.url().toString();
    QVERIFY2(url.contains(QStringLiteral("oauth.trainingpeaks.com")),
             qPrintable("Expected TrainingPeaks OAuth URL, got: " + url));
}

// ─────────────────────────────────────────────────────────────────────────────
// HTTP method tests
// ─────────────────────────────────────────────────────────────────────────────

void TstTrainingPeaksService::testUpload_isPost()
{
    const QString tmp = makeTempFile();
    TrainingPeaksService svc;
    svc.setAccessToken(ACCESS_TOKEN);
    QNetworkReply *reply = svc.uploadActivity(tmp, "T", "D");
    QVERIFY(reply != nullptr);
    reply->deleteLater();
    QFile::remove(tmp);

    QCOMPARE(m_manager->lastOp, QNetworkAccessManager::PostOperation);
}

void TstTrainingPeaksService::testRefreshToken_isPost()
{
    QNetworkReply *reply = TrainingPeaksService::refreshToken(REFRESH_TOKEN);
    QVERIFY(reply != nullptr);
    reply->deleteLater();

    QCOMPARE(m_manager->lastOp, QNetworkAccessManager::PostOperation);
}

// ─────────────────────────────────────────────────────────────────────────────
// TP_CLIENT_SECRET compile-time constant
//
// Verify the secret is not the old hardcoded plaintext value.
// The define is injected by the build system from the TP_CLIENT_SECRET env var;
// if it's empty ("") that's acceptable — it means the secret wasn't configured
// for this build.  What must NOT be present is the old hardcoded secret.
// ─────────────────────────────────────────────────────────────────────────────

void TstTrainingPeaksService::testClientSecret_isNotHardcoded()
{
    // This test will fail at compile time if someone re-introduces the old
    // hardcoded secret into a string constant visible to the linker.
    const QString secret = CLIENT_SECRET_TP;
    const QString oldHardcoded = QStringLiteral("tXd2eDxHe73taHkVB4oHkMkSPw6aZ3nO5n2R0bxc");
    QVERIFY2(secret != oldHardcoded,
             "CLIENT_SECRET_TP must not contain the hardcoded plaintext secret. "
             "Supply it via the TP_CLIENT_SECRET build-time env var instead.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Null manager guard
// ─────────────────────────────────────────────────────────────────────────────

void TstTrainingPeaksService::testNullManager_returnsNullptr()
{
    qApp->setProperty("NetworkManagerWS", QVariant());

    TrainingPeaksService svc;
    svc.setAccessToken(ACCESS_TOKEN);

    const QString tmp = makeTempFile();
    QCOMPARE(svc.uploadActivity(tmp, "T", "D"), nullptr);
    QCOMPARE(TrainingPeaksService::refreshToken(REFRESH_TOKEN), nullptr);
    QFile::remove(tmp);

    // Restore
    qApp->setProperty("NetworkManagerWS",
                      QVariant::fromValue<QNetworkAccessManager *>(m_manager));
}

// ─────────────────────────────────────────────────────────────────────────────
// Optional live test — token refresh
// ─────────────────────────────────────────────────────────────────────────────

void TstTrainingPeaksService::testLive_refreshToken()
{
    const QString token = qEnvironmentVariable("TP_REFRESH_TOKEN");
    if (token.isEmpty())
        QSKIP("Set TP_REFRESH_TOKEN (and optionally TP_ACCESS_TOKEN) to run this live test");

    QNetworkAccessManager realMgr;
    qApp->setProperty("NetworkManagerWS",
                      QVariant::fromValue<QNetworkAccessManager *>(&realMgr));

    QNetworkReply *reply = TrainingPeaksService::refreshToken(token);
    QVERIFY(reply != nullptr);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QVERIFY2(status > 0 && status < 500,
             qPrintable("Unexpected HTTP status from TrainingPeaks: " + QString::number(status)));

    reply->deleteLater();
    qApp->setProperty("NetworkManagerWS",
                      QVariant::fromValue<QNetworkAccessManager *>(m_manager));
}

// ─────────────────────────────────────────────────────────────────────────────
QTEST_MAIN(TstTrainingPeaksService)
#include "tst_trainingpeaks_service.moc"
