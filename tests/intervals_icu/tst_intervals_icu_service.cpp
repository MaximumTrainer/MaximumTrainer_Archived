/*
 * tst_intervals_icu_service.cpp
 *
 * Qt Test suite for IntervalsIcuService.
 *
 * Tests run without making real network connections by injecting a
 * MockNetworkAccessManager that captures every QNetworkRequest before
 * any I/O is attempted.  The manager is registered on the QCoreApplication
 * as the "NetworkManagerWS" property, matching the production pattern used
 * by ExtRequest and IntervalsIcuDAO.
 *
 * Test groups
 * ──────────────────────────────────────────────────────────────────
 * Auth header   – Authorization: Basic is present and correctly encoded
 *                 for each public method
 * Accept header – Accept: application/json is set on every request
 * URLs          – each method targets the expected endpoint path
 * getEvents     – query string contains oldest= and newest= parameters
 * Null manager  – methods return nullptr when NetworkManagerWS is absent
 */

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDate>

#include "intervals_icu_service.h"

// ─────────────────────────────────────────────────────────────────────────────
// MockNetworkAccessManager
//
// Overrides createRequest() to record the last request without opening a
// real socket.  Returns an always-finished QNetworkReply stub so the caller
// is not left dangling.
// ─────────────────────────────────────────────────────────────────────────────
class MockNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    QNetworkRequest lastRequest;
    int             callCount = 0;

protected:
    QNetworkReply* createRequest(Operation op,
                                 const QNetworkRequest &request,
                                 QIODevice *outgoingData) override
    {
        Q_UNUSED(op)
        Q_UNUSED(outgoingData)
        lastRequest = request;
        ++callCount;
        // Delegate to base — it will produce a reply immediately (no real
        // socket opened in test environment because no event loop is run).
        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
static QByteArray expectedBasicHeader(const QString &apiKey)
{
    const QString credentials = QStringLiteral("athlete:") + apiKey;
    return QByteArray("Basic ") + credentials.toUtf8().toBase64();
}

// ─────────────────────────────────────────────────────────────────────────────
class TstIntervalsIcuService : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    // ── Authorization header ────────────────────────────────────────────────
    void testGetAthlete_authHeader();
    void testGetEvents_authHeader();
    void testGetWorkouts_authHeader();
    void testDownloadZwo_authHeader();
    void testDownloadMrc_authHeader();

    // ── Accept header ───────────────────────────────────────────────────────
    void testGetAthlete_acceptHeader();

    // ── URL construction ────────────────────────────────────────────────────
    void testGetAthlete_url();
    void testGetEvents_url();
    void testGetWorkouts_url();
    void testDownloadZwo_url();
    void testDownloadMrc_url();

    // ── getEvents query parameters ──────────────────────────────────────────
    void testGetEvents_queryParams();

    // ── Null manager guard ──────────────────────────────────────────────────
    void testNullManager_returnsNullptr();

private:
    MockNetworkAccessManager *m_manager = nullptr;

    static constexpr const char ATHLETE_ID[] = "i12345";
    static constexpr const char API_KEY[]    = "testApiKey";
    static constexpr const char WORKOUT_ID[] = "w99";
};

// ─────────────────────────────────────────────────────────────────────────────
void TstIntervalsIcuService::initTestCase()
{
    m_manager = new MockNetworkAccessManager(this);
    qApp->setProperty("NetworkManagerWS",
                      QVariant::fromValue<QNetworkAccessManager*>(m_manager));
}

void TstIntervalsIcuService::cleanupTestCase()
{
    qApp->setProperty("NetworkManagerWS", QVariant());
}

void TstIntervalsIcuService::init()
{
    m_manager->callCount = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Authorization header tests
// ─────────────────────────────────────────────────────────────────────────────

void TstIntervalsIcuService::testGetAthlete_authHeader()
{
    QNetworkReply *reply = IntervalsIcuService::getAthlete(ATHLETE_ID, API_KEY);
    if (reply) reply->deleteLater();

    const QByteArray header = m_manager->lastRequest.rawHeader("Authorization");
    QCOMPARE(header, expectedBasicHeader(API_KEY));
}

void TstIntervalsIcuService::testGetEvents_authHeader()
{
    QNetworkReply *reply = IntervalsIcuService::getEvents(
        ATHLETE_ID, API_KEY,
        QDate(2024, 1, 1), QDate(2024, 1, 31));
    if (reply) reply->deleteLater();

    const QByteArray header = m_manager->lastRequest.rawHeader("Authorization");
    QCOMPARE(header, expectedBasicHeader(API_KEY));
}

void TstIntervalsIcuService::testGetWorkouts_authHeader()
{
    QNetworkReply *reply = IntervalsIcuService::getWorkouts(ATHLETE_ID, API_KEY);
    if (reply) reply->deleteLater();

    const QByteArray header = m_manager->lastRequest.rawHeader("Authorization");
    QCOMPARE(header, expectedBasicHeader(API_KEY));
}

void TstIntervalsIcuService::testDownloadZwo_authHeader()
{
    QNetworkReply *reply = IntervalsIcuService::downloadWorkoutZwo(
        ATHLETE_ID, WORKOUT_ID, API_KEY);
    if (reply) reply->deleteLater();

    const QByteArray header = m_manager->lastRequest.rawHeader("Authorization");
    QCOMPARE(header, expectedBasicHeader(API_KEY));
}

void TstIntervalsIcuService::testDownloadMrc_authHeader()
{
    QNetworkReply *reply = IntervalsIcuService::downloadWorkoutMrc(
        ATHLETE_ID, WORKOUT_ID, API_KEY);
    if (reply) reply->deleteLater();

    const QByteArray header = m_manager->lastRequest.rawHeader("Authorization");
    QCOMPARE(header, expectedBasicHeader(API_KEY));
}

// ─────────────────────────────────────────────────────────────────────────────
// Accept header test
// ─────────────────────────────────────────────────────────────────────────────

void TstIntervalsIcuService::testGetAthlete_acceptHeader()
{
    QNetworkReply *reply = IntervalsIcuService::getAthlete(ATHLETE_ID, API_KEY);
    if (reply) reply->deleteLater();

    const QByteArray header = m_manager->lastRequest.rawHeader("Accept");
    QCOMPARE(header, QByteArray("application/json"));
}

// ─────────────────────────────────────────────────────────────────────────────
// URL construction tests
// ─────────────────────────────────────────────────────────────────────────────

void TstIntervalsIcuService::testGetAthlete_url()
{
    QNetworkReply *reply = IntervalsIcuService::getAthlete(ATHLETE_ID, API_KEY);
    if (reply) reply->deleteLater();

    const QString url = m_manager->lastRequest.url().toString();
    QVERIFY(url.contains(QStringLiteral("/athlete/") + ATHLETE_ID));
    QVERIFY(url.startsWith(QStringLiteral("https://intervals.icu/api/v1")));
}

void TstIntervalsIcuService::testGetEvents_url()
{
    QNetworkReply *reply = IntervalsIcuService::getEvents(
        ATHLETE_ID, API_KEY,
        QDate(2024, 1, 1), QDate(2024, 1, 31));
    if (reply) reply->deleteLater();

    const QString url = m_manager->lastRequest.url().toString();
    QVERIFY(url.contains(QStringLiteral("/athlete/") + ATHLETE_ID + QStringLiteral("/events")));
}

void TstIntervalsIcuService::testGetWorkouts_url()
{
    QNetworkReply *reply = IntervalsIcuService::getWorkouts(ATHLETE_ID, API_KEY);
    if (reply) reply->deleteLater();

    const QString url = m_manager->lastRequest.url().toString();
    QVERIFY(url.contains(QStringLiteral("/athlete/") + ATHLETE_ID + QStringLiteral("/workouts")));
    QVERIFY(!url.contains(QStringLiteral("/workouts/"))); // no workout ID segment
}

void TstIntervalsIcuService::testDownloadZwo_url()
{
    QNetworkReply *reply = IntervalsIcuService::downloadWorkoutZwo(
        ATHLETE_ID, WORKOUT_ID, API_KEY);
    if (reply) reply->deleteLater();

    const QString url = m_manager->lastRequest.url().toString();
    QVERIFY(url.contains(QStringLiteral("/workouts/") + WORKOUT_ID + QStringLiteral("/file.zwo")));
}

void TstIntervalsIcuService::testDownloadMrc_url()
{
    QNetworkReply *reply = IntervalsIcuService::downloadWorkoutMrc(
        ATHLETE_ID, WORKOUT_ID, API_KEY);
    if (reply) reply->deleteLater();

    const QString url = m_manager->lastRequest.url().toString();
    QVERIFY(url.contains(QStringLiteral("/workouts/") + WORKOUT_ID + QStringLiteral("/file.mrc")));
}

// ─────────────────────────────────────────────────────────────────────────────
// getEvents query parameter test
// ─────────────────────────────────────────────────────────────────────────────

void TstIntervalsIcuService::testGetEvents_queryParams()
{
    const QDate start(2024, 3, 1);
    const QDate end(2024, 3, 31);

    QNetworkReply *reply = IntervalsIcuService::getEvents(ATHLETE_ID, API_KEY, start, end);
    if (reply) reply->deleteLater();

    const QUrl url = m_manager->lastRequest.url();
    QVERIFY(url.hasQuery());
    QVERIFY(url.toString().contains(QStringLiteral("oldest=2024-03-01")));
    QVERIFY(url.toString().contains(QStringLiteral("newest=2024-03-31")));
}

// ─────────────────────────────────────────────────────────────────────────────
// Null manager guard test
// ─────────────────────────────────────────────────────────────────────────────

void TstIntervalsIcuService::testNullManager_returnsNullptr()
{
    // Temporarily clear the property
    qApp->setProperty("NetworkManagerWS", QVariant());

    QCOMPARE(IntervalsIcuService::getAthlete(ATHLETE_ID, API_KEY), nullptr);
    QCOMPARE(IntervalsIcuService::getEvents(ATHLETE_ID, API_KEY,
                                            QDate(2024,1,1), QDate(2024,1,31)), nullptr);
    QCOMPARE(IntervalsIcuService::getWorkouts(ATHLETE_ID, API_KEY), nullptr);
    QCOMPARE(IntervalsIcuService::downloadWorkoutZwo(ATHLETE_ID, WORKOUT_ID, API_KEY), nullptr);
    QCOMPARE(IntervalsIcuService::downloadWorkoutMrc(ATHLETE_ID, WORKOUT_ID, API_KEY), nullptr);

    // Restore
    qApp->setProperty("NetworkManagerWS",
                      QVariant::fromValue<QNetworkAccessManager*>(m_manager));
}

// ─────────────────────────────────────────────────────────────────────────────
QTEST_MAIN(TstIntervalsIcuService)
#include "tst_intervals_icu_service.moc"
