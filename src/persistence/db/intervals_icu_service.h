#ifndef INTERVALS_ICU_SERVICE_H
#define INTERVALS_ICU_SERVICE_H

#include <QDate>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

/// Static service class for the Intervals.icu REST API.
///
/// Authentication uses HTTP Basic Auth where the username is the literal
/// string "athlete" and the password is the user's personal API key.
///
/// All methods return the pending QNetworkReply; the caller must connect
/// &QNetworkReply::finished to its own slot and call reply->readAll() inside
/// it — identical to the existing ExtRequest pattern.
class IntervalsIcuService
{
public:
    /// Validate credentials / fetch athlete profile.
    /// GET /athlete/{id}
    static QNetworkReply* getAthlete(const QString &athleteId, const QString &apiKey);

    /// Fetch calendar events for the given date range.
    /// GET /athlete/{id}/events?oldest=YYYY-MM-DD&newest=YYYY-MM-DD
    static QNetworkReply* getEvents(const QString &athleteId, const QString &apiKey,
                                    const QDate &startDate, const QDate &endDate);

    /// Fetch the athlete's workout library.
    /// GET /athlete/{id}/workouts
    static QNetworkReply* getWorkouts(const QString &athleteId, const QString &apiKey);

    /// Download a workout as a ZWO file.
    /// GET /athlete/{id}/workouts/{workoutId}.zwo
    static QNetworkReply* downloadWorkoutZwo(const QString &athleteId,
                                             const QString &workoutId,
                                             const QString &apiKey);

    /// Download a workout as an MRC file.
    /// GET /athlete/{id}/workouts/{workoutId}/file.mrc
    static QNetworkReply* downloadWorkoutMrc(const QString &athleteId,
                                             const QString &workoutId,
                                             const QString &apiKey);

    /// Create a workout in the athlete's library.
    /// POST /athlete/{id}/workouts
    /// @param json  UTF-8 encoded JSON object describing the new workout.
    ///              Minimum required fields: "name" (string), "type" (string).
    /// Returns the pending reply; on success the server responds HTTP 200 with
    /// the created workout object (including its "id" field).
    static QNetworkReply* createWorkout(const QString &athleteId,
                                        const QString &apiKey,
                                        const QByteArray &json);

    /// Delete a workout from the athlete's library.
    /// DELETE /athlete/{id}/workouts/{workoutId}
    static QNetworkReply* deleteWorkout(const QString &athleteId,
                                        const QString &workoutId,
                                        const QString &apiKey);

    /// List all the athlete's workout folders.
    /// GET /athlete/{id}/folders
    static QNetworkReply* listFolders(const QString &athleteId, const QString &apiKey);

private:
    /// Build a QNetworkRequest with Authorization: Basic and Accept: application/json.
    static QNetworkRequest buildRequest(const QString &url, const QString &apiKey);

    static constexpr const char BASE_URL[] = "https://intervals.icu/api/v1";
};

#endif // INTERVALS_ICU_SERVICE_H
