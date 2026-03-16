#ifndef INTERVALSICUDAO_H
#define INTERVALSICUDAO_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

/// Data-access object for the Intervals.icu REST API.
///
/// Authentication uses HTTP Basic Auth where the username is the literal
/// string "API_KEY" and the password is the user's personal API key.
///
/// Endpoints used:
///   GET /api/v1/athlete/{id}          – basic profile (name, weight, FTP, LTHR)
///   GET /api/v1/athlete/{id}/settings – detailed training zones
class IntervalsIcuDAO
{
public:
    /// Fetch the athlete's basic profile from Intervals.icu.
    /// @param athleteId  The Intervals.icu athlete ID (e.g. "i12345").
    /// @param apiKey     The user's Intervals.icu API key.
    /// @return The pending QNetworkReply; caller must connect finished().
    static QNetworkReply* getAthlete(const QString &athleteId, const QString &apiKey);

    /// Fetch the athlete's detailed training-zone settings from Intervals.icu.
    /// @param athleteId  The Intervals.icu athlete ID (e.g. "i12345").
    /// @param apiKey     The user's Intervals.icu API key.
    /// @return The pending QNetworkReply; caller must connect finished().
    static QNetworkReply* getAthleteSettings(const QString &athleteId, const QString &apiKey);

    /// Download a workout as a ZWO (Zwift workout XML) file from Intervals.icu.
    /// Endpoint: GET /api/v1/athlete/{athleteId}/workouts/{workoutId}.zwo
    /// @param athleteId  The Intervals.icu athlete ID (e.g. "i12345").
    /// @param workoutId  The numeric workout ID shown on the calendar (e.g. "12345678").
    /// @param apiKey     The user's Intervals.icu API key.
    /// @return The pending QNetworkReply; caller must connect finished().
    static QNetworkReply* downloadWorkoutZwo(const QString &athleteId, const QString &workoutId, const QString &apiKey);

private:
    /// Build a QNetworkRequest with the Intervals.icu Basic Auth header set.
    static QNetworkRequest buildRequest(const QString &url, const QString &apiKey);
};

#endif // INTERVALSICUDAO_H
