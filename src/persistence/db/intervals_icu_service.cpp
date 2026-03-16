#include "intervals_icu_service.h"

#include <QCoreApplication>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QUrlQuery>

// ─────────────────────────────────────────────────────────────────────────────
QNetworkRequest IntervalsIcuService::buildRequest(const QString &url, const QString &apiKey)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));

    // Intervals.icu uses HTTP Basic Auth: username="API_KEY", password=<apiKey>
    const QString credentials = QStringLiteral("API_KEY:") + apiKey;
    request.setRawHeader("Authorization",
                         QByteArray("Basic ") + credentials.toUtf8().toBase64());
    request.setRawHeader("Accept", "application/json");

    return request;
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/athlete/{id}
QNetworkReply* IntervalsIcuService::getAthlete(const QString &athleteId, const QString &apiKey)
{
    QNetworkAccessManager *manager =
        qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!manager) {
        qWarning() << "IntervalsIcuService::getAthlete: NetworkManagerWS is not available";
        return nullptr;
    }

    const QString url = QLatin1String(BASE_URL) + QStringLiteral("/athlete/") + athleteId;
    return manager->get(buildRequest(url, apiKey));
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/athlete/{id}/events?oldest=YYYY-MM-DD&newest=YYYY-MM-DD
QNetworkReply* IntervalsIcuService::getEvents(const QString &athleteId, const QString &apiKey,
                                              const QDate &startDate, const QDate &endDate)
{
    QNetworkAccessManager *manager =
        qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!manager) {
        qWarning() << "IntervalsIcuService::getEvents: NetworkManagerWS is not available";
        return nullptr;
    }

    QUrl url(QLatin1String(BASE_URL) + QStringLiteral("/athlete/") + athleteId
             + QStringLiteral("/events"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("oldest"), startDate.toString(Qt::ISODate));
    query.addQueryItem(QStringLiteral("newest"), endDate.toString(Qt::ISODate));
    url.setQuery(query);

    return manager->get(buildRequest(url.toString(), apiKey));
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/athlete/{id}/workouts
QNetworkReply* IntervalsIcuService::getWorkouts(const QString &athleteId, const QString &apiKey)
{
    QNetworkAccessManager *manager =
        qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!manager) {
        qWarning() << "IntervalsIcuService::getWorkouts: NetworkManagerWS is not available";
        return nullptr;
    }

    const QString url = QLatin1String(BASE_URL) + QStringLiteral("/athlete/") + athleteId
                        + QStringLiteral("/workouts");
    return manager->get(buildRequest(url, apiKey));
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/athlete/{id}/workouts/{workoutId}/file.zwo
QNetworkReply* IntervalsIcuService::downloadWorkoutZwo(const QString &athleteId,
                                                       const QString &workoutId,
                                                       const QString &apiKey)
{
    QNetworkAccessManager *manager =
        qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!manager) {
        qWarning() << "IntervalsIcuService::downloadWorkoutZwo: NetworkManagerWS is not available";
        return nullptr;
    }

    const QString url = QLatin1String(BASE_URL) + QStringLiteral("/athlete/") + athleteId
                        + QStringLiteral("/workouts/") + workoutId
                        + QStringLiteral("/file.zwo");
    return manager->get(buildRequest(url, apiKey));
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/athlete/{id}/workouts/{workoutId}/file.mrc
QNetworkReply* IntervalsIcuService::downloadWorkoutMrc(const QString &athleteId,
                                                       const QString &workoutId,
                                                       const QString &apiKey)
{
    QNetworkAccessManager *manager =
        qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!manager) {
        qWarning() << "IntervalsIcuService::downloadWorkoutMrc: NetworkManagerWS is not available";
        return nullptr;
    }

    const QString url = QLatin1String(BASE_URL) + QStringLiteral("/athlete/") + athleteId
                        + QStringLiteral("/workouts/") + workoutId
                        + QStringLiteral("/file.mrc");
    return manager->get(buildRequest(url, apiKey));
}
