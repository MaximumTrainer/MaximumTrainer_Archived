#include "localdatabase.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QVariant>
#include <QDebug>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static QString dbDirectoryPath()
{
    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return docs + QDir::separator() + QStringLiteral("MaximumTrainer");
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

LocalDatabase::LocalDatabase(QObject *parent)
    : QObject(parent)
    , m_connectionName(QUuid::createUuid().toString())
{
}

LocalDatabase::~LocalDatabase()
{
    close();
}

// ---------------------------------------------------------------------------
// open / close / isOpen
// ---------------------------------------------------------------------------

bool LocalDatabase::open(const QString &emailClean)
{
    if (m_db.isOpen())
        close();

    QString dbPath;
    if (emailClean == QLatin1String(":memory:")) {
        // Special SQLite in-memory database — used by unit tests.
        dbPath = QStringLiteral(":memory:");
    } else {
        const QString dirPath = dbDirectoryPath();
        QDir dir(dirPath);
        if (!dir.exists() && !dir.mkpath(dirPath)) {
            qWarning() << "LocalDatabase: cannot create directory" << dirPath;
            return false;
        }
        dbPath = dirPath + QDir::separator() + emailClean + QStringLiteral(".db");
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "LocalDatabase: failed to open" << dbPath
                   << m_db.lastError().text();
        return false;
    }

    // Enable WAL mode for better concurrent read performance and durability.
    // (WAL is not applicable to :memory: databases but is harmless there.)
    QSqlQuery pragmaQuery(m_db);
    if (!pragmaQuery.exec(QStringLiteral("PRAGMA journal_mode=WAL"))) {
        qWarning() << "LocalDatabase: could not enable WAL mode:"
                   << pragmaQuery.lastError().text();
        // WAL is a performance hint; failure is non-fatal.
    }

    if (!createTables()) {
        close();
        return false;
    }

    qDebug() << "LocalDatabase: opened" << dbPath;
    return true;
}

void LocalDatabase::close()
{
    if (m_db.isOpen())
        m_db.close();

    // Remove the named connection so Qt doesn't warn about duplicate names
    // if open() is called again.
    if (QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);
}

bool LocalDatabase::isOpen() const
{
    return m_db.isOpen();
}

// ---------------------------------------------------------------------------
// Schema creation
// ---------------------------------------------------------------------------

bool LocalDatabase::createTables()
{
    QSqlQuery q(m_db);

    const QStringList statements = {
        // Completed-workout tracking (replaces hashWorkoutDone in .save XML)
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS workout_history ("
            "    workout_name TEXT NOT NULL PRIMARY KEY,"
            "    completed_at TEXT NOT NULL"
            "        DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))"
            ")"),

        // Completed-course tracking (replaces hashCourseDone in .save XML)
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS course_history ("
            "    course_name  TEXT NOT NULL PRIMARY KEY,"
            "    completed_at TEXT NOT NULL"
            "        DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))"
            ")"),

        // Cached fitness metrics / identity for offline use
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS account_profile ("
            "    email_clean          TEXT PRIMARY KEY,"
            "    user_id              INTEGER,"
            "    ftp                  INTEGER,"
            "    lthr                 INTEGER,"
            "    weight_kg            REAL,"
            "    height_cm            INTEGER,"
            "    first_name           TEXT,"
            "    last_name            TEXT,"
            "    display_name         TEXT,"
            "    subscription_type_id INTEGER,"
            "    updated_at           TEXT NOT NULL"
            "        DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))"
            ")"),

        // Paired sensor configurations
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS sensors ("
            "    ant_id      INTEGER NOT NULL,"
            "    device_type INTEGER NOT NULL,"
            "    name        TEXT,"
            "    details     TEXT,"
            "    PRIMARY KEY (ant_id, device_type)"
            ")")
    };

    for (const QString &sql : statements) {
        if (!q.exec(sql)) {
            qWarning() << "LocalDatabase: schema error:" << q.lastError().text();
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Workout history
// ---------------------------------------------------------------------------

bool LocalDatabase::markWorkoutDone(const QString &workoutName)
{
    if (!m_db.isOpen() || workoutName.isEmpty())
        return false;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO workout_history (workout_name) VALUES (:name)"));
    q.bindValue(QStringLiteral(":name"), workoutName);

    if (!q.exec()) {
        qWarning() << "LocalDatabase::markWorkoutDone:" << q.lastError().text();
        return false;
    }
    return true;
}

bool LocalDatabase::markWorkoutUndone(const QString &workoutName)
{
    if (!m_db.isOpen() || workoutName.isEmpty())
        return false;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "DELETE FROM workout_history WHERE workout_name = :name"));
    q.bindValue(QStringLiteral(":name"), workoutName);

    if (!q.exec()) {
        qWarning() << "LocalDatabase::markWorkoutUndone:" << q.lastError().text();
        return false;
    }
    return true;
}

QSet<QString> LocalDatabase::getWorkoutsDone() const
{
    QSet<QString> result;
    if (!m_db.isOpen())
        return result;

    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT workout_name FROM workout_history"))) {
        qWarning() << "LocalDatabase::getWorkoutsDone:" << q.lastError().text();
        return result;
    }

    while (q.next())
        result.insert(q.value(0).toString());

    return result;
}

bool LocalDatabase::setWorkoutsDone(const QSet<QString> &workouts)
{
    if (!m_db.isOpen())
        return false;

    m_db.transaction();

    QSqlQuery del(m_db);
    if (!del.exec(QStringLiteral("DELETE FROM workout_history"))) {
        qWarning() << "LocalDatabase::setWorkoutsDone (delete):" << del.lastError().text();
        m_db.rollback();
        return false;
    }

    QSqlQuery ins(m_db);
    ins.prepare(QStringLiteral(
        "INSERT INTO workout_history (workout_name) VALUES (:name)"));

    for (const QString &name : workouts) {
        if (name.isEmpty())
            continue;
        ins.bindValue(QStringLiteral(":name"), name);
        if (!ins.exec()) {
            qWarning() << "LocalDatabase::setWorkoutsDone (insert):" << ins.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    m_db.commit();
    return true;
}

// ---------------------------------------------------------------------------
// Course history
// ---------------------------------------------------------------------------

bool LocalDatabase::markCourseDone(const QString &courseName)
{
    if (!m_db.isOpen() || courseName.isEmpty())
        return false;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO course_history (course_name) VALUES (:name)"));
    q.bindValue(QStringLiteral(":name"), courseName);

    if (!q.exec()) {
        qWarning() << "LocalDatabase::markCourseDone:" << q.lastError().text();
        return false;
    }
    return true;
}

bool LocalDatabase::markCourseUndone(const QString &courseName)
{
    if (!m_db.isOpen() || courseName.isEmpty())
        return false;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "DELETE FROM course_history WHERE course_name = :name"));
    q.bindValue(QStringLiteral(":name"), courseName);

    if (!q.exec()) {
        qWarning() << "LocalDatabase::markCourseUndone:" << q.lastError().text();
        return false;
    }
    return true;
}

QSet<QString> LocalDatabase::getCoursesDone() const
{
    QSet<QString> result;
    if (!m_db.isOpen())
        return result;

    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT course_name FROM course_history"))) {
        qWarning() << "LocalDatabase::getCoursesDone:" << q.lastError().text();
        return result;
    }

    while (q.next())
        result.insert(q.value(0).toString());

    return result;
}

bool LocalDatabase::setCoursesDone(const QSet<QString> &courses)
{
    if (!m_db.isOpen())
        return false;

    m_db.transaction();

    QSqlQuery del(m_db);
    if (!del.exec(QStringLiteral("DELETE FROM course_history"))) {
        qWarning() << "LocalDatabase::setCoursesDone (delete):" << del.lastError().text();
        m_db.rollback();
        return false;
    }

    QSqlQuery ins(m_db);
    ins.prepare(QStringLiteral(
        "INSERT INTO course_history (course_name) VALUES (:name)"));

    for (const QString &name : courses) {
        if (name.isEmpty())
            continue;
        ins.bindValue(QStringLiteral(":name"), name);
        if (!ins.exec()) {
            qWarning() << "LocalDatabase::setCoursesDone (insert):" << ins.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    m_db.commit();
    return true;
}

// ---------------------------------------------------------------------------
// Account profile
// ---------------------------------------------------------------------------

bool LocalDatabase::saveAccountProfile(const Account *account)
{
    if (!m_db.isOpen() || !account)
        return false;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO account_profile "
        "(email_clean, user_id, ftp, lthr, weight_kg, height_cm,"
        " first_name, last_name, display_name, subscription_type_id,"
        " updated_at)"
        " VALUES "
        "(:email_clean, :user_id, :ftp, :lthr, :weight_kg, :height_cm,"
        " :first_name, :last_name, :display_name, :subscription_type_id,"
        " strftime('%Y-%m-%dT%H:%M:%SZ','now'))"));

    q.bindValue(QStringLiteral(":email_clean"),          account->email_clean);
    q.bindValue(QStringLiteral(":user_id"),              account->id);
    q.bindValue(QStringLiteral(":ftp"),                  account->FTP);
    q.bindValue(QStringLiteral(":lthr"),                 account->LTHR);
    q.bindValue(QStringLiteral(":weight_kg"),            account->weight_kg);
    q.bindValue(QStringLiteral(":height_cm"),            account->height_cm);
    q.bindValue(QStringLiteral(":first_name"),           account->first_name);
    q.bindValue(QStringLiteral(":last_name"),            account->last_name);
    q.bindValue(QStringLiteral(":display_name"),         account->display_name);
    q.bindValue(QStringLiteral(":subscription_type_id"), account->subscription_type_id);

    if (!q.exec()) {
        qWarning() << "LocalDatabase::saveAccountProfile:" << q.lastError().text();
        return false;
    }
    return true;
}

bool LocalDatabase::loadAccountProfile(Account *account) const
{
    if (!m_db.isOpen() || !account)
        return false;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT user_id, ftp, lthr, weight_kg, height_cm,"
        "       first_name, last_name, display_name, subscription_type_id"
        "  FROM account_profile"
        " WHERE email_clean = :email_clean"));
    q.bindValue(QStringLiteral(":email_clean"), account->email_clean);

    if (!q.exec()) {
        qWarning() << "LocalDatabase::loadAccountProfile:" << q.lastError().text();
        return false;
    }

    if (!q.next())
        return false;  // No cached profile

    account->id                   = q.value(0).toInt();
    account->FTP                  = q.value(1).toInt();
    account->LTHR                 = q.value(2).toInt();
    account->weight_kg            = q.value(3).toDouble();
    account->height_cm            = q.value(4).toInt();
    account->first_name           = q.value(5).toString();
    account->last_name            = q.value(6).toString();
    account->display_name         = q.value(7).toString();
    account->subscription_type_id = q.value(8).toInt();

    return true;
}

// ---------------------------------------------------------------------------
// Sensors
// ---------------------------------------------------------------------------

bool LocalDatabase::saveSensors(const QList<Sensor> &sensors)
{
    if (!m_db.isOpen())
        return false;

    m_db.transaction();

    QSqlQuery del(m_db);
    if (!del.exec(QStringLiteral("DELETE FROM sensors"))) {
        qWarning() << "LocalDatabase::saveSensors (delete):" << del.lastError().text();
        m_db.rollback();
        return false;
    }

    QSqlQuery ins(m_db);
    ins.prepare(QStringLiteral(
        "INSERT INTO sensors (ant_id, device_type, name, details)"
        " VALUES (:ant_id, :device_type, :name, :details)"));

    for (const Sensor &s : sensors) {
        ins.bindValue(QStringLiteral(":ant_id"),      s.getAntId());
        ins.bindValue(QStringLiteral(":device_type"), static_cast<int>(s.getDeviceType()));
        ins.bindValue(QStringLiteral(":name"),        s.getName());
        ins.bindValue(QStringLiteral(":details"),     s.getDetails());

        if (!ins.exec()) {
            qWarning() << "LocalDatabase::saveSensors (insert):" << ins.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    m_db.commit();
    return true;
}

QList<Sensor> LocalDatabase::getSensors() const
{
    QList<Sensor> result;
    if (!m_db.isOpen())
        return result;

    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT ant_id, device_type, name, details FROM sensors"))) {
        qWarning() << "LocalDatabase::getSensors:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        Sensor s(q.value(0).toInt(),
                 static_cast<Sensor::SENSOR_TYPE>(q.value(1).toInt()),
                 q.value(2).toString(),
                 q.value(3).toString());
        result.append(s);
    }

    return result;
}
