#ifndef LOCALDATABASE_H
#define LOCALDATABASE_H

#include <QObject>
#include <QSet>
#include <QList>
#include <QSqlDatabase>

#include "account.h"
#include "sensor.h"

/// LocalDatabase — SQLite-backed local storage for MaximumTrainer.
///
/// Stores per-user data that must persist between sessions without
/// requiring a network connection:
///
///   - workout_history  : names of workouts the user has completed
///   - course_history   : names of courses the user has completed
///   - account_profile  : cached fitness metrics (FTP, LTHR, weight …)
///   - sensors          : paired BLE/ANT+ sensor configurations
///
/// One database file is created per user account in the application
/// documents directory:  <documents>/MaximumTrainer/<emailClean>.db
///
/// Usage:
///   LocalDatabase *db = qApp->property("LocalDatabase").value<LocalDatabase*>();
///   if (db && db->isOpen()) {
///       db->markWorkoutDone("ftp_test.xml");
///       QSet<QString> done = db->getWorkoutsDone();
///   }
class LocalDatabase : public QObject
{
    Q_OBJECT

public:
    explicit LocalDatabase(QObject *parent = nullptr);
    ~LocalDatabase() override;

    /// Open (or create) the SQLite database for the given user.
    /// @param emailClean  Filesystem-safe email string used as the DB filename.
    /// @return true on success.
    bool open(const QString &emailClean);

    /// Close the database connection.
    void close();

    /// @return true if the database is currently open.
    bool isOpen() const;

    // ── Workout history ──────────────────────────────────────────────────────

    /// Record that the user has completed a workout.
    bool markWorkoutDone(const QString &workoutName);

    /// Remove a workout from the completed list.
    bool markWorkoutUndone(const QString &workoutName);

    /// @return All workout names recorded as completed for this user.
    QSet<QString> getWorkoutsDone() const;

    /// Replace the entire completed-workout set (used for XML migration).
    bool setWorkoutsDone(const QSet<QString> &workouts);

    // ── Course history ───────────────────────────────────────────────────────

    /// Record that the user has completed a course.
    bool markCourseDone(const QString &courseName);

    /// Remove a course from the completed list.
    bool markCourseUndone(const QString &courseName);

    /// @return All course names recorded as completed for this user.
    QSet<QString> getCoursesDone() const;

    /// Replace the entire completed-course set (used for XML migration).
    bool setCoursesDone(const QSet<QString> &courses);

    // ── Account profile ──────────────────────────────────────────────────────

    /// Persist the account's fitness metrics and identity fields.
    bool saveAccountProfile(const Account *account);

    /// Load the cached fitness metrics into @p account.
    /// @return true if a row was found; false if no cached profile exists.
    bool loadAccountProfile(Account *account) const;

    // ── Sensors ──────────────────────────────────────────────────────────────

    /// Persist the full list of paired sensors for this user.
    bool saveSensors(const QList<Sensor> &sensors);

    /// @return The previously saved list of sensors (may be empty).
    QList<Sensor> getSensors() const;

private:
    bool createTables();

    QSqlDatabase m_db;
    /// Unique Qt connection name for this instance (avoids global-state issues).
    QString      m_connectionName;
};

Q_DECLARE_METATYPE(LocalDatabase*)

#endif // LOCALDATABASE_H
