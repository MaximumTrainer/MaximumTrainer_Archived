/*
 * tst_intervals_icu_integration.cpp
 *
 * Live integration tests for IntervalsIcuService.
 *
 * These tests make REAL HTTP requests to https://intervals.icu/api/v1 using
 * a dedicated test account.  Credentials are read from environment variables:
 *
 *   INTERVALS_ICU_API_KEY    – personal API key (Settings → API on intervals.icu)
 *   INTERVALS_ICU_ATHLETE_ID – athlete ID, e.g. i12345
 *
 * Every test calls QSKIP when credentials are absent, so the suite degrades
 * gracefully in local development and fork PRs that cannot access GitHub Secrets.
 *
 * A real QNetworkAccessManager is registered as the "NetworkManagerWS" application
 * property — the same pattern used by IntervalsIcuService in production.
 *
 * Test groups
 * ──────────────────────────────────────────────────────────────────────────────
 * testGetAthlete    – GET /athlete/{id} → 200 OK, JSON "id" matches athleteId
 * testGetEvents     – GET /athlete/{id}/events?... → 200 OK, JSON array
 * testGetWorkouts   – GET /athlete/{id}/workouts → 200 OK, JSON array
 * testBadApiKey     – GET /athlete/{id} with wrong key → HTTP 401
 */

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDate>
#include <QEventLoop>
#include <QTimer>

#include "intervals_icu_service.h"

static constexpr int TIMEOUT_MS = 30'000;  // 30 s per request

// ─────────────────────────────────────────────────────────────────────────────
// Helper: spin the event loop until the reply finishes or the timeout fires.
// Returns true if the reply finished within the time limit.
// ─────────────────────────────────────────────────────────────────────────────
static bool waitForReply(QNetworkReply *reply)
{
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(TIMEOUT_MS, &loop, &QEventLoop::quit);
    loop.exec();
    return reply->isFinished();
}

// ─────────────────────────────────────────────────────────────────────────────
class TstIntervalsIcuIntegration : public QObject
{
    Q_OBJECT

    QString m_apiKey;
    QString m_athleteId;
    QNetworkAccessManager *m_manager = nullptr;

private slots:

    // ─── Setup ───────────────────────────────────────────────────────────────

    void initTestCase()
    {
        m_apiKey    = qEnvironmentVariable("INTERVALS_ICU_API_KEY");
        m_athleteId = qEnvironmentVariable("INTERVALS_ICU_ATHLETE_ID");

        if (m_apiKey.isEmpty() || m_athleteId.isEmpty()) {
            QSKIP("Set INTERVALS_ICU_API_KEY and INTERVALS_ICU_ATHLETE_ID to run "
                  "live Intervals.icu integration tests.");
        }

        m_manager = new QNetworkAccessManager(this);
        qApp->setProperty("NetworkManagerWS",
                          QVariant::fromValue<QNetworkAccessManager*>(m_manager));
    }

    void cleanupTestCase()
    {
        qApp->setProperty("NetworkManagerWS", QVariant());
    }

    // ─── Tests ───────────────────────────────────────────────────────────────

    // GET /api/v1/athlete/{id}  →  200 OK, body is a JSON object whose "id"
    // field matches the athlete ID used for authentication.
    void testGetAthlete()
    {
        QNetworkReply *reply = IntervalsIcuService::getAthlete(m_athleteId, m_apiKey);
        QVERIFY2(reply, "getAthlete returned nullptr — NetworkManagerWS not registered");

        QVERIFY2(waitForReply(reply), "getAthlete timed out after 30 s");
        QCOMPARE(reply->error(), QNetworkReply::NoError);

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QCOMPARE(status, 200);

        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        QVERIFY2(!obj.isEmpty(), "getAthlete: response body is not a JSON object");
        QCOMPARE(obj.value(QStringLiteral("id")).toString(), m_athleteId);

        reply->deleteLater();
    }

    // GET /api/v1/athlete/{id}/events?oldest=...&newest=...  →  200 OK, JSON array.
    // Uses a ±7-day window around today so at least the request round-trips cleanly.
    void testGetEvents()
    {
        const QDate today = QDate::currentDate();
        QNetworkReply *reply = IntervalsIcuService::getEvents(
            m_athleteId, m_apiKey,
            today.addDays(-7),
            today.addDays(7));
        QVERIFY2(reply, "getEvents returned nullptr — NetworkManagerWS not registered");

        QVERIFY2(waitForReply(reply), "getEvents timed out after 30 s");
        QCOMPARE(reply->error(), QNetworkReply::NoError);

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QCOMPARE(status, 200);

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QVERIFY2(doc.isArray(), "getEvents: response body is not a JSON array");

        reply->deleteLater();
    }

    // GET /api/v1/athlete/{id}/workouts  →  200 OK, JSON array.
    void testGetWorkouts()
    {
        QNetworkReply *reply = IntervalsIcuService::getWorkouts(m_athleteId, m_apiKey);
        QVERIFY2(reply, "getWorkouts returned nullptr — NetworkManagerWS not registered");

        QVERIFY2(waitForReply(reply), "getWorkouts timed out after 30 s");
        QCOMPARE(reply->error(), QNetworkReply::NoError);

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QCOMPARE(status, 200);

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QVERIFY2(doc.isArray(), "getWorkouts: response body is not a JSON array");

        reply->deleteLater();
    }

    // GET /api/v1/athlete/{id} with a deliberately wrong API key  →  HTTP 401.
    // Qt does not treat HTTP 4xx responses as QNetworkReply::NetworkError, so the
    // status code is read directly from the reply attribute.
    void testBadApiKey()
    {
        QNetworkReply *reply = IntervalsIcuService::getAthlete(
            m_athleteId, QStringLiteral("invalid-key-xyz"));
        QVERIFY2(reply, "getAthlete(bad key) returned nullptr — NetworkManagerWS not registered");

        QVERIFY2(waitForReply(reply), "getAthlete(bad key) timed out after 30 s");

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QCOMPARE(status, 401);

        reply->deleteLater();
    }
};

QTEST_MAIN(TstIntervalsIcuIntegration)
#include "tst_intervals_icu_integration.moc"
