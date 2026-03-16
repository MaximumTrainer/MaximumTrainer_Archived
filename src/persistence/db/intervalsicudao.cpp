#include "intervalsicudao.h"

#include <QByteArray>

#include "environnement.h"



// ───────────────────────────────────────────────────────────────────────────────
QNetworkRequest IntervalsIcuDAO::buildRequest(const QString &url, const QString &apiKey)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));

    // Intervals.icu uses HTTP Basic Auth: username="API_KEY", password=<apiKey>
    const QString credentials = QStringLiteral("API_KEY:") + apiKey;
    const QByteArray encoded   = credentials.toUtf8().toBase64();
    request.setRawHeader("Authorization", QByteArray("Basic ") + encoded);
    request.setRawHeader("Accept", "application/json");

    return request;
}


// ───────────────────────────────────────────────────────────────────────────────
// GET /api/v1/athlete/{id}
QNetworkReply* IntervalsIcuDAO::getAthlete(const QString &athleteId, const QString &apiKey)
{
    QNetworkAccessManager *manager = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!manager) {
        qWarning() << "IntervalsIcuDAO::getAthlete: NetworkManagerWS is not available";
        return nullptr;
    }

    const QString url = urlIntervalsIcuApi + athleteId;
    QNetworkRequest request = buildRequest(url, apiKey);

    return manager->get(request);
}


// ───────────────────────────────────────────────────────────────────────────────
// GET /api/v1/athlete/{id}/settings
QNetworkReply* IntervalsIcuDAO::getAthleteSettings(const QString &athleteId, const QString &apiKey)
{
    QNetworkAccessManager *manager = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!manager) {
        qWarning() << "IntervalsIcuDAO::getAthleteSettings: NetworkManagerWS is not available";
        return nullptr;
    }

    const QString url = urlIntervalsIcuApi + athleteId + QStringLiteral("/settings");
    QNetworkRequest request = buildRequest(url, apiKey);

    return manager->get(request);
}


// ───────────────────────────────────────────────────────────────────────────────
// GET /api/v1/athlete/{athleteId}/workouts/{workoutId}.zwo
QNetworkReply* IntervalsIcuDAO::downloadWorkoutZwo(const QString &athleteId, const QString &workoutId, const QString &apiKey)
{
    QNetworkAccessManager *manager = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!manager) {
        qWarning() << "IntervalsIcuDAO::downloadWorkoutZwo: NetworkManagerWS is not available";
        return nullptr;
    }

    const QString url = urlIntervalsIcuApi + athleteId
                        + QStringLiteral("/workouts/")
                        + workoutId
                        + QStringLiteral(".zwo");
    QNetworkRequest request = buildRequest(url, apiKey);

    return manager->get(request);
}
