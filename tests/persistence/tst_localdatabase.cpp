/*
 * tst_localdatabase.cpp
 *
 * Qt Test suite for LocalDatabase — the SQLite-backed local storage layer.
 *
 * All tests use a temporary in-memory (":memory:") database by calling
 * open(":memory:") so that no files are created on disk and tests remain
 * self-contained and side-effect-free.
 *
 * Test groups
 * ──────────────────────────────────────────────────────────────────
 * Open/close    – database opens, reports isOpen(), closes cleanly,
 *                 can be reopened
 * Workout history – markWorkoutDone, markWorkoutUndone, getWorkoutsDone,
 *                   setWorkoutsDone (bulk replace), idempotent inserts
 * Course history  – same CRUD contract as workout history
 * Account profile – saveAccountProfile round-trips through
 *                   loadAccountProfile; missing profile returns false
 * Sensors         – saveSensors / getSensors round-trip; SENSOR_TYPE
 *                   preserved as integer
 */

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QStandardPaths>

#include "localdatabase.h"
#include "account.h"
#include "sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
// The SQLite driver uses the "database name" literally as the file path.
// Passing ":memory:" opens an in-memory database — perfect for unit tests.
// ─────────────────────────────────────────────────────────────────────────────
static const QLatin1String kInMemoryDb(":memory:");

class TstLocalDatabase : public QObject
{
    Q_OBJECT

private slots:
    // ── Open / close ─────────────────────────────────────────────────────────
    void testOpen_succeeds();
    void testOpen_isOpen_afterOpen();
    void testClose_isNotOpen_afterClose();
    void testReopen_afterClose();

    // ── Workout history ───────────────────────────────────────────────────────
    void testWorkout_markAndGet();
    void testWorkout_markIdempotent();
    void testWorkout_markUndone();
    void testWorkout_undoneNonExistent_noError();
    void testWorkout_setWorkoutsDone_bulkReplace();
    void testWorkout_emptyNameIgnored();

    // ── Course history ────────────────────────────────────────────────────────
    void testCourse_markAndGet();
    void testCourse_markIdempotent();
    void testCourse_markUndone();
    void testCourse_setCoursesDone_bulkReplace();

    // ── Account profile ───────────────────────────────────────────────────────
    void testAccount_saveAndLoad();
    void testAccount_loadMissing_returnsFalse();
    void testAccount_saveOverwriteExisting();

    // ── Sensors ───────────────────────────────────────────────────────────────
    void testSensors_saveAndGet();
    void testSensors_saveOverwriteExisting();
    void testSensors_emptyList_clearsAll();
    void testSensors_deviceTypePreserved();

    // ── DB not open ───────────────────────────────────────────────────────────
    void testOps_whenClosed_returnFalseOrEmpty();
};

// ─────────────────────────────────────────────────────────────────────────────
// Open / close
// ─────────────────────────────────────────────────────────────────────────────

void TstLocalDatabase::testOpen_succeeds()
{
    LocalDatabase db;
    QVERIFY(db.open(kInMemoryDb));
}

void TstLocalDatabase::testOpen_isOpen_afterOpen()
{
    LocalDatabase db;
    db.open(kInMemoryDb);
    QVERIFY(db.isOpen());
}

void TstLocalDatabase::testClose_isNotOpen_afterClose()
{
    LocalDatabase db;
    db.open(kInMemoryDb);
    db.close();
    QVERIFY(!db.isOpen());
}

void TstLocalDatabase::testReopen_afterClose()
{
    LocalDatabase db;
    db.open(kInMemoryDb);
    db.close();
    QVERIFY(db.open(kInMemoryDb));
    QVERIFY(db.isOpen());
}

// ─────────────────────────────────────────────────────────────────────────────
// Workout history
// ─────────────────────────────────────────────────────────────────────────────

void TstLocalDatabase::testWorkout_markAndGet()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    QVERIFY(db.markWorkoutDone(QStringLiteral("ftp_test.xml")));
    QVERIFY(db.markWorkoutDone(QStringLiteral("sweet_spot_40.xml")));

    const QSet<QString> done = db.getWorkoutsDone();
    QCOMPARE(done.size(), 2);
    QVERIFY(done.contains(QStringLiteral("ftp_test.xml")));
    QVERIFY(done.contains(QStringLiteral("sweet_spot_40.xml")));
}

void TstLocalDatabase::testWorkout_markIdempotent()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    db.markWorkoutDone(QStringLiteral("ftp_test.xml"));
    // Inserting the same workout twice should not fail or create a duplicate.
    QVERIFY(db.markWorkoutDone(QStringLiteral("ftp_test.xml")));
    QCOMPARE(db.getWorkoutsDone().size(), 1);
}

void TstLocalDatabase::testWorkout_markUndone()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    db.markWorkoutDone(QStringLiteral("ftp_test.xml"));
    db.markWorkoutDone(QStringLiteral("sweet_spot_40.xml"));
    QVERIFY(db.markWorkoutUndone(QStringLiteral("ftp_test.xml")));

    const QSet<QString> done = db.getWorkoutsDone();
    QCOMPARE(done.size(), 1);
    QVERIFY(!done.contains(QStringLiteral("ftp_test.xml")));
    QVERIFY(done.contains(QStringLiteral("sweet_spot_40.xml")));
}

void TstLocalDatabase::testWorkout_undoneNonExistent_noError()
{
    LocalDatabase db;
    db.open(kInMemoryDb);
    // Removing a workout that was never marked done must succeed silently.
    QVERIFY(db.markWorkoutUndone(QStringLiteral("nonexistent.xml")));
}

void TstLocalDatabase::testWorkout_setWorkoutsDone_bulkReplace()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    db.markWorkoutDone(QStringLiteral("old_workout.xml"));

    QSet<QString> newSet = {QStringLiteral("a.xml"), QStringLiteral("b.xml")};
    QVERIFY(db.setWorkoutsDone(newSet));

    const QSet<QString> done = db.getWorkoutsDone();
    QCOMPARE(done.size(), 2);
    QVERIFY(!done.contains(QStringLiteral("old_workout.xml")));
    QVERIFY(done.contains(QStringLiteral("a.xml")));
    QVERIFY(done.contains(QStringLiteral("b.xml")));
}

void TstLocalDatabase::testWorkout_emptyNameIgnored()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    // Bulk-set that includes an empty string: empty entries must be skipped.
    QSet<QString> withEmpty = {QStringLiteral("valid.xml"), QStringLiteral("")};
    QVERIFY(db.setWorkoutsDone(withEmpty));

    const QSet<QString> done = db.getWorkoutsDone();
    QVERIFY(!done.contains(QStringLiteral("")));
    QVERIFY(done.contains(QStringLiteral("valid.xml")));
}

// ─────────────────────────────────────────────────────────────────────────────
// Course history
// ─────────────────────────────────────────────────────────────────────────────

void TstLocalDatabase::testCourse_markAndGet()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    QVERIFY(db.markCourseDone(QStringLiteral("route_1.xml")));
    QVERIFY(db.markCourseDone(QStringLiteral("route_2.xml")));

    const QSet<QString> done = db.getCoursesDone();
    QCOMPARE(done.size(), 2);
    QVERIFY(done.contains(QStringLiteral("route_1.xml")));
    QVERIFY(done.contains(QStringLiteral("route_2.xml")));
}

void TstLocalDatabase::testCourse_markIdempotent()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    db.markCourseDone(QStringLiteral("route_1.xml"));
    QVERIFY(db.markCourseDone(QStringLiteral("route_1.xml")));
    QCOMPARE(db.getCoursesDone().size(), 1);
}

void TstLocalDatabase::testCourse_markUndone()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    db.markCourseDone(QStringLiteral("route_1.xml"));
    db.markCourseDone(QStringLiteral("route_2.xml"));
    QVERIFY(db.markCourseUndone(QStringLiteral("route_1.xml")));

    const QSet<QString> done = db.getCoursesDone();
    QCOMPARE(done.size(), 1);
    QVERIFY(done.contains(QStringLiteral("route_2.xml")));
}

void TstLocalDatabase::testCourse_setCoursesDone_bulkReplace()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    db.markCourseDone(QStringLiteral("old_course.xml"));

    QSet<QString> newSet = {QStringLiteral("c1.xml"), QStringLiteral("c2.xml")};
    QVERIFY(db.setCoursesDone(newSet));

    const QSet<QString> done = db.getCoursesDone();
    QCOMPARE(done.size(), 2);
    QVERIFY(!done.contains(QStringLiteral("old_course.xml")));
}

// ─────────────────────────────────────────────────────────────────────────────
// Account profile
// ─────────────────────────────────────────────────────────────────────────────

void TstLocalDatabase::testAccount_saveAndLoad()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    Account saved;
    saved.email_clean          = QStringLiteral("testuser");
    saved.id                   = 42;
    saved.FTP                  = 280;
    saved.LTHR                 = 168;
    saved.weight_kg            = 73.5;
    saved.height_cm            = 178;
    saved.first_name           = QStringLiteral("Jane");
    saved.last_name            = QStringLiteral("Doe");
    saved.display_name         = QStringLiteral("JaneDoe");
    saved.subscription_type_id = 2;

    QVERIFY(db.saveAccountProfile(&saved));

    Account loaded;
    loaded.email_clean = QStringLiteral("testuser");
    QVERIFY(db.loadAccountProfile(&loaded));

    QCOMPARE(loaded.id,                   42);
    QCOMPARE(loaded.FTP,                  280);
    QCOMPARE(loaded.LTHR,                 168);
    QCOMPARE(loaded.weight_kg,            73.5);
    QCOMPARE(loaded.height_cm,            178);
    QCOMPARE(loaded.first_name,           QStringLiteral("Jane"));
    QCOMPARE(loaded.last_name,            QStringLiteral("Doe"));
    QCOMPARE(loaded.display_name,         QStringLiteral("JaneDoe"));
    QCOMPARE(loaded.subscription_type_id, 2);
}

void TstLocalDatabase::testAccount_loadMissing_returnsFalse()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    Account account;
    account.email_clean = QStringLiteral("nobody");
    QVERIFY(!db.loadAccountProfile(&account));
}

void TstLocalDatabase::testAccount_saveOverwriteExisting()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    Account first;
    first.email_clean = QStringLiteral("testuser");
    first.FTP         = 200;
    db.saveAccountProfile(&first);

    Account updated;
    updated.email_clean = QStringLiteral("testuser");
    updated.FTP         = 250;
    QVERIFY(db.saveAccountProfile(&updated));

    Account loaded;
    loaded.email_clean = QStringLiteral("testuser");
    db.loadAccountProfile(&loaded);
    QCOMPARE(loaded.FTP, 250);
}

// ─────────────────────────────────────────────────────────────────────────────
// Sensors
// ─────────────────────────────────────────────────────────────────────────────

void TstLocalDatabase::testSensors_saveAndGet()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    QList<Sensor> sensors = {
        Sensor(101, Sensor::SENSOR_HR,    QStringLiteral("HR Belt"),   QStringLiteral("BLE HR")),
        Sensor(202, Sensor::SENSOR_POWER, QStringLiteral("Power Tap"), QStringLiteral("ANT+ PM")),
    };

    QVERIFY(db.saveSensors(sensors));

    const QList<Sensor> loaded = db.getSensors();
    QCOMPARE(loaded.size(), 2);

    // Verify round-trip by checking IDs and types (order may differ).
    QSet<int> loadedIds;
    for (const Sensor &s : loaded)
        loadedIds.insert(s.getAntId());

    QVERIFY(loadedIds.contains(101));
    QVERIFY(loadedIds.contains(202));
}

void TstLocalDatabase::testSensors_saveOverwriteExisting()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    QList<Sensor> first = {
        Sensor(101, Sensor::SENSOR_HR, QStringLiteral("HR Belt"), QStringLiteral(""))
    };
    db.saveSensors(first);

    QList<Sensor> second = {
        Sensor(303, Sensor::SENSOR_CADENCE, QStringLiteral("Cadence Pod"), QStringLiteral(""))
    };
    QVERIFY(db.saveSensors(second));

    const QList<Sensor> loaded = db.getSensors();
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded.first().getAntId(), 303);
}

void TstLocalDatabase::testSensors_emptyList_clearsAll()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    QList<Sensor> sensors = {
        Sensor(101, Sensor::SENSOR_HR, QStringLiteral("HR Belt"), QStringLiteral(""))
    };
    db.saveSensors(sensors);

    QVERIFY(db.saveSensors({}));
    QVERIFY(db.getSensors().isEmpty());
}

void TstLocalDatabase::testSensors_deviceTypePreserved()
{
    LocalDatabase db;
    db.open(kInMemoryDb);

    QList<Sensor> sensors = {
        Sensor(777, Sensor::SENSOR_OXYGEN, QStringLiteral("O2 Ring"), QStringLiteral(""))
    };
    db.saveSensors(sensors);

    const QList<Sensor> loaded = db.getSensors();
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded.first().getDeviceType(), Sensor::SENSOR_OXYGEN);
}

// ─────────────────────────────────────────────────────────────────────────────
// Operations when DB is closed
// ─────────────────────────────────────────────────────────────────────────────

void TstLocalDatabase::testOps_whenClosed_returnFalseOrEmpty()
{
    LocalDatabase db;
    // Do not open — all operations should degrade gracefully.

    QVERIFY(!db.markWorkoutDone(QStringLiteral("x.xml")));
    QVERIFY(!db.markWorkoutUndone(QStringLiteral("x.xml")));
    QVERIFY(db.getWorkoutsDone().isEmpty());
    QVERIFY(!db.setWorkoutsDone({QStringLiteral("x.xml")}));

    QVERIFY(!db.markCourseDone(QStringLiteral("c.xml")));
    QVERIFY(!db.markCourseUndone(QStringLiteral("c.xml")));
    QVERIFY(db.getCoursesDone().isEmpty());
    QVERIFY(!db.setCoursesDone({QStringLiteral("c.xml")}));

    Account account;
    QVERIFY(!db.saveAccountProfile(&account));
    QVERIFY(!db.loadAccountProfile(&account));

    QVERIFY(!db.saveSensors({}));
    QVERIFY(db.getSensors().isEmpty());
}

QTEST_MAIN(TstLocalDatabase)
#include "tst_localdatabase.moc"
