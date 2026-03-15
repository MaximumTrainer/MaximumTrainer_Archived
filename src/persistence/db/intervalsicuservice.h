#ifndef INTERVALSICUSERVICE_H
#define INTERVALSICUSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDate>
#include <QList>
#include <QString>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

///
/// Intervals.icu API client.
///
/// Authentication: HTTP Basic Auth with username "API_KEY" and the user's
/// personal API key as the password (found at intervals.icu → Settings → API).
///
/// All methods are non-blocking and return a QNetworkReply* whose
/// finished() signal the caller must connect to a slot.
///
class IntervalsIcuService : public QObject
{
    Q_OBJECT

public:
    /// A calendar event as returned by GET /athlete/{id}/events
    struct CalendarEvent {
        QString id;
        QString name;
        QDate   date;
        QString type;
        int     duration_sec;   ///< moving_time in seconds (0 if unavailable)
        QString workout_id;     ///< workout id for .zwo download ("" if none)
        QString description;
    };

    explicit IntervalsIcuService(QObject *parent = nullptr);

    /// Set credentials before calling any API method.
    void setCredentials(const QString &apiKey, const QString &athleteId);

    QString athleteId() const { return m_athleteId; }

    // ── API methods ──────────────────────────────────────────────────────────

    /// GET /api/v1/athlete/{id}  — lightweight connection test.
    QNetworkReply *testConnection();

    /// GET /api/v1/athlete/{id}/events  — calendar events in [oldest, newest].
    QNetworkReply *fetchCalendar(const QDate &oldest, const QDate &newest);

    /// GET /api/v1/athlete/{id}/workouts/{workoutId}.zwo
    QNetworkReply *downloadWorkoutZwo(const QString &workoutId);

    // ── Static parsers ───────────────────────────────────────────────────────

    /// Parse a JSON byte array from the /events endpoint into a list of
    /// CalendarEvent structs.  Returns an empty list on parse error.
    static QList<CalendarEvent> parseEvents(const QByteArray &data);

private:
    QNetworkRequest buildRequest(const QString &path) const;

    QString m_apiKey;
    QString m_athleteId;
};

#endif // INTERVALSICUSERVICE_H
