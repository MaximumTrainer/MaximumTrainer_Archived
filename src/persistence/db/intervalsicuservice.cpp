#include "intervalsicuservice.h"

#include <QApplication>
#include <QNetworkRequest>
#include <QUrl>
#include <QByteArray>
#include <QDebug>

static const QString INTERVALS_BASE_URL = "https://intervals.icu/api/v1";

// ─────────────────────────────────────────────────────────────────────────────
IntervalsIcuService::IntervalsIcuService(QObject *parent)
    : QObject(parent)
{
}

// ─────────────────────────────────────────────────────────────────────────────
void IntervalsIcuService::setCredentials(const QString &apiKey, const QString &athleteId)
{
    m_apiKey    = apiKey;
    m_athleteId = athleteId;
}

// ─────────────────────────────────────────────────────────────────────────────
QNetworkRequest IntervalsIcuService::buildRequest(const QString &path) const
{
    QNetworkRequest request;
    request.setUrl(QUrl(INTERVALS_BASE_URL + path));

    // HTTP Basic Auth: username = "API_KEY", password = user API key
    const QByteArray credentials =
        ("API_KEY:" + m_apiKey).toUtf8().toBase64();
    request.setRawHeader("Authorization", "Basic " + credentials);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/json");
    return request;
}

// ─────────────────────────────────────────────────────────────────────────────
QNetworkReply *IntervalsIcuService::testConnection()
{
    auto *mgr = qApp->property("NetworkManagerWS")
                    .value<QNetworkAccessManager *>();
    return mgr->get(buildRequest("/athlete/" + m_athleteId));
}

// ─────────────────────────────────────────────────────────────────────────────
QNetworkReply *IntervalsIcuService::fetchCalendar(const QDate &oldest,
                                                   const QDate &newest)
{
    const QString path = QString("/athlete/%1/events?oldest=%2&newest=%3")
                             .arg(m_athleteId,
                                  oldest.toString(Qt::ISODate),
                                  newest.toString(Qt::ISODate));

    auto *mgr = qApp->property("NetworkManagerWS")
                    .value<QNetworkAccessManager *>();
    return mgr->get(buildRequest(path));
}

// ─────────────────────────────────────────────────────────────────────────────
QNetworkReply *IntervalsIcuService::downloadWorkoutZwo(const QString &workoutId)
{
    const QString path =
        QString("/athlete/%1/workouts/%2.zwo")
            .arg(m_athleteId, workoutId);

    auto *mgr = qApp->property("NetworkManagerWS")
                    .value<QNetworkAccessManager *>();
    return mgr->get(buildRequest(path));
}

// ─────────────────────────────────────────────────────────────────────────────
// static
QList<IntervalsIcuService::CalendarEvent>
IntervalsIcuService::parseEvents(const QByteArray &data)
{
    QList<CalendarEvent> events;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "IntervalsIcuService::parseEvents JSON error:"
                   << err.errorString();
        return events;
    }

    if (!doc.isArray()) {
        qWarning() << "IntervalsIcuService::parseEvents: expected JSON array";
        return events;
    }

    for (const QJsonValue &val : doc.array()) {
        if (!val.isObject())
            continue;
        const QJsonObject obj = val.toObject();

        CalendarEvent ev;
        ev.id          = QString::number(obj["id"].toInt());
        ev.name        = obj["name"].toString();
        ev.type        = obj["type"].toString();
        ev.duration_sec = obj["moving_time"].toInt(0);
        ev.description = obj["description"].toString();

        // workout_id may be absent (free event, not a structured workout)
        if (obj.contains("workout_id") && !obj["workout_id"].isNull())
            ev.workout_id = QString::number(obj["workout_id"].toInt());

        // start_date_local is ISO 8601 "YYYY-MM-DDThh:mm:ss"
        const QString dateStr =
            obj["start_date_local"].toString().left(10);
        ev.date = QDate::fromString(dateStr, Qt::ISODate);

        events.append(ev);
    }

    return events;
}
