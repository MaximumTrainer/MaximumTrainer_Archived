/*
 * tst_online_mode.cpp
 *
 * Online Mode Integration Test -- MaximumTrainer
 *
 * Purpose
 * -------------------------------------------------------------------------
 * Simulates a user authenticating into the MaximumTrainer application on the
 * target OS.  A 1280x720 window is created and shown (mirroring the online
 * mode state of the running application), a live HTTPS authentication request
 * is made to https://intervals.icu/api/v1 using the credentials provided via
 * GitHub Actions secrets, and the result is displayed in the window and
 * captured as a screenshot.
 *
 * Credentials
 * -------------------------------------------------------------------------
 * Read from environment variables (set from GitHub Actions secrets):
 *   INTERVALS_ICU_API_KEY    – personal API key (intervals.icu Settings → API)
 *   INTERVALS_ICU_ATHLETE_ID – athlete ID, e.g. i12345
 *
 * The test calls QSKIP gracefully when either variable is absent, so the
 * suite degrades cleanly in local development and fork PRs without configured
 * secrets.
 *
 * Test flow
 * -------------------------------------------------------------------------
 * 1. OnlineModeWindow shown with "[ AUTHENTICATING... ]" status badge.
 * 2. QNetworkAccessManager registered as "NetworkManagerWS" app property
 *    (same pattern used by IntervalsIcuService in production).
 * 3. IntervalsIcuService::getAthlete() called with credentials from env vars.
 * 4. QEventLoop spins until the reply finishes or 30 s timeout.
 * 5. Window badge updated: "[ CONNECTED ]" on HTTP 200, "[ AUTH FAILED ]" otherwise.
 * 6. Athlete name and ID populated in the window.
 * 7. Screenshot captured: build/tests/online-mode-{platform}-{timestamp}.png
 * 8. Assertions: HTTP 200, no network error, matching athlete ID, non-empty name.
 *
 * Visual Evidence
 * -------------------------------------------------------------------------
 * A labelled 1280x720 screenshot is saved as:
 *   build/tests/online-mode-{platform}-{YYYY-MM-DDTHH-MM-SS}Z.png
 *
 * Build:
 *   qmake online_mode_tests.pro && make
 * Run headless (Linux CI):
 *   Xvfb :99 -screen 0 1280x800x24 & export DISPLAY=:99
 *   ../../build/tests/online_mode_tests -v2
 * Run directly (Windows / macOS CI):
 *   .\build\tests\online_mode_tests.exe -v2
 */

#include "tst_online_mode.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScreen>
#include <QPixmap>
#include <QDir>
#include <QSysInfo>
#include <QDateTime>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>

#include "intervals_icu_service.h"

// ─────────────────────────────────────────────────────────────────────────────
// Compile-time platform tag
// ─────────────────────────────────────────────────────────────────────────────
#if defined(Q_OS_WIN)
    static const QString kPlatformTag = QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    static const QString kPlatformTag = QStringLiteral("macos");
#else
    static const QString kPlatformTag = QStringLiteral("linux");
#endif

static constexpr int TIMEOUT_MS = 30'000;  // 30 s per request

// ─────────────────────────────────────────────────────────────────────────────
// OnlineModeWindow implementation
// ─────────────────────────────────────────────────────────────────────────────
OnlineModeWindow::OnlineModeWindow(const QString &athleteId,
                                   const QString &timestamp,
                                   QWidget       *parent)
    : QWidget(parent)
    {
        const QString platform = kPlatformTag.toUpper();
        const QString osName   = QSysInfo::prettyProductName();
        const QString qtVer    = QString("Qt %1").arg(qVersion());

        setWindowTitle(
            QString("MaximumTrainer -- Online Mode Authentication [%1]").arg(platform));
        setFixedSize(1280, 720);

        setStyleSheet(
            "OnlineModeWindow { background-color: #0d1117; }"
            "QLabel { color: #c9d1d9;"
            "         font-family: 'DejaVu Sans', 'Segoe UI', sans-serif; }"
        );

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(48, 32, 48, 32);
        root->setSpacing(0);

        // ── Header bar ────────────────────────────────────────────────────────
        auto *headerRow = new QHBoxLayout();

        auto *appTitle = new QLabel("MaximumTrainer", this);
        appTitle->setStyleSheet(
            "font-size: 28px; font-weight: bold; color: #58a6ff;");

        m_statusBadge = new QLabel("[ AUTHENTICATING... ]", this);
        m_statusBadge->setStyleSheet(
            "font-size: 14px; color: #f0883e; background: #161b22;"
            "border: 1px solid #30363d; border-radius: 4px; padding: 4px 12px;");
        m_statusBadge->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        headerRow->addWidget(appTitle,     1);
        headerRow->addWidget(m_statusBadge, 0, Qt::AlignRight | Qt::AlignVCenter);
        root->addLayout(headerRow);
        root->addSpacing(6);

        // ── Meta row ─────────────────────────────────────────────────────────
        auto *metaLabel = new QLabel(
            QString("Platform: %1  |  %2  |  %3  |  %4")
                .arg(platform, osName, qtVer, timestamp),
            this);
        metaLabel->setStyleSheet("font-size: 12px; color: #8b949e;");
        root->addWidget(metaLabel);
        root->addSpacing(18);

        auto *sep1 = new QFrame(this);
        sep1->setFrameShape(QFrame::HLine);
        sep1->setStyleSheet("color: #21262d;");
        root->addWidget(sep1);
        root->addSpacing(24);

        // ── Auth panel ────────────────────────────────────────────────────────
        auto *panelFrame = new QFrame(this);
        panelFrame->setStyleSheet(
            "QFrame { background: #161b22; border: 1px solid #30363d;"
            "         border-radius: 8px; }");
        auto *panelLayout = new QVBoxLayout(panelFrame);
        panelLayout->setContentsMargins(40, 32, 40, 32);
        panelLayout->setSpacing(20);

        auto *panelTitle = new QLabel(
            "Intervals.icu -- User Authentication", panelFrame);
        panelTitle->setStyleSheet(
            "font-size: 14px; color: #8b949e; font-weight: bold;");
        panelLayout->addWidget(panelTitle);

        // Requested athlete ID row
        auto *idRow = new QHBoxLayout();
        auto *idKey = new QLabel("Athlete ID (requested):", panelFrame);
        idKey->setStyleSheet("font-size: 14px; color: #8b949e;");
        m_requestedIdLabel = new QLabel(athleteId, panelFrame);
        m_requestedIdLabel->setStyleSheet(
            "font-size: 14px; color: #c9d1d9; font-weight: bold;");
        idRow->addWidget(idKey);
        idRow->addWidget(m_requestedIdLabel);
        idRow->addStretch();
        panelLayout->addLayout(idRow);

        // Confirmed athlete ID row (populated after auth)
        auto *confirmedRow = new QHBoxLayout();
        auto *confirmedKey = new QLabel("Athlete ID (confirmed):", panelFrame);
        confirmedKey->setStyleSheet("font-size: 14px; color: #8b949e;");
        m_confirmedIdLabel = new QLabel("—", panelFrame);
        m_confirmedIdLabel->setStyleSheet(
            "font-size: 14px; color: #8b949e;");
        confirmedRow->addWidget(confirmedKey);
        confirmedRow->addWidget(m_confirmedIdLabel);
        confirmedRow->addStretch();
        panelLayout->addLayout(confirmedRow);

        // Athlete name row (populated after auth)
        auto *nameRow = new QHBoxLayout();
        auto *nameKey = new QLabel("Athlete Name:", panelFrame);
        nameKey->setStyleSheet("font-size: 14px; color: #8b949e;");
        m_athleteNameLabel = new QLabel("—", panelFrame);
        m_athleteNameLabel->setStyleSheet(
            "font-size: 24px; font-weight: bold; color: #7ee787;");
        nameRow->addWidget(nameKey);
        nameRow->addWidget(m_athleteNameLabel);
        nameRow->addStretch();
        panelLayout->addLayout(nameRow);

        // HTTP status row
        auto *httpRow = new QHBoxLayout();
        auto *httpKey = new QLabel("HTTP Status:", panelFrame);
        httpKey->setStyleSheet("font-size: 14px; color: #8b949e;");
        m_httpStatusLabel = new QLabel("—", panelFrame);
        m_httpStatusLabel->setStyleSheet(
            "font-size: 14px; color: #8b949e;");
        httpRow->addWidget(httpKey);
        httpRow->addWidget(m_httpStatusLabel);
        httpRow->addStretch();
        panelLayout->addLayout(httpRow);

        root->addWidget(panelFrame);
        root->addStretch();

        // ── Footer ────────────────────────────────────────────────────────────
        auto *sep2 = new QFrame(this);
        sep2->setFrameShape(QFrame::HLine);
        sep2->setStyleSheet("color: #21262d;");
        root->addWidget(sep2);
        root->addSpacing(14);

        auto *footerRow = new QHBoxLayout();
        auto *footerLeft = new QLabel(
            "intervals.icu — Live Authentication Test", this);
        footerLeft->setStyleSheet("font-size: 12px; color: #8b949e;");

        m_buildLabel = new QLabel(
            QString("Artefact: online-mode-%1-%2.png")
                .arg(kPlatformTag, timestamp),
            this);
        m_buildLabel->setStyleSheet("font-size: 12px; color: #8b949e;");
        m_buildLabel->setAlignment(Qt::AlignRight);

        footerRow->addWidget(footerLeft, 1);
        footerRow->addWidget(m_buildLabel, 1, Qt::AlignRight);
        root->addLayout(footerRow);
    }

void OnlineModeWindow::markConnected(const QString &confirmedId, const QString &athleteName, int httpStatus)
{
        m_statusBadge->setText("[ CONNECTED ]");
        m_statusBadge->setStyleSheet(
            "font-size: 14px; color: #3fb950; background: #0d2010;"
            "border: 1px solid #238636; border-radius: 4px; padding: 4px 12px;");
        m_confirmedIdLabel->setText(confirmedId);
        m_confirmedIdLabel->setStyleSheet(
            "font-size: 14px; color: #3fb950; font-weight: bold;");
        m_athleteNameLabel->setText(athleteName.isEmpty() ? "(no name)" : athleteName);
        m_httpStatusLabel->setText(QString::number(httpStatus) + " OK");
        m_httpStatusLabel->setStyleSheet(
            "font-size: 14px; color: #3fb950; font-weight: bold;");
    }

void OnlineModeWindow::markFailed(int httpStatus, const QString &errorString)
{
        m_statusBadge->setText("[ AUTH FAILED ]");
        m_statusBadge->setStyleSheet(
            "font-size: 14px; color: #f85149; background: #200d0d;"
            "border: 1px solid #58181a; border-radius: 4px; padding: 4px 12px;");
        m_httpStatusLabel->setText(
            QString("HTTP %1  —  %2").arg(httpStatus).arg(errorString));
        m_httpStatusLabel->setStyleSheet(
            "font-size: 14px; color: #f85149;");
    }

// ─────────────────────────────────────────────────────────────────────────────
// Shared helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Spin an event loop until @p reply finishes or @p timeoutMs elapses.
/// Returns true iff the reply finished before the timeout.
static bool waitForReply(QNetworkReply *reply, int timeoutMs = TIMEOUT_MS)
{
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
    return reply->isFinished();
}

// ─────────────────────────────────────────────────────────────────────────────
// ResultWindow implementation
// ─────────────────────────────────────────────────────────────────────────────
ResultWindow::ResultWindow(const QString &testTitle,
                           const QString &subtitle,
                           const QString &timestamp,
                           QWidget       *parent)
    : QWidget(parent)
{
    const QString platform = kPlatformTag.toUpper();
    const QString osName   = QSysInfo::prettyProductName();
    const QString qtVer    = QString("Qt %1").arg(qVersion());

    setWindowTitle(QString("MaximumTrainer — %1 [%2]").arg(testTitle, platform));
    setFixedSize(1280, 720);

    setStyleSheet(
        "ResultWindow { background-color: #0d1117; }"
        "QLabel { color: #c9d1d9;"
        "         font-family: 'DejaVu Sans', 'Segoe UI', sans-serif; }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(48, 32, 48, 32);
    root->setSpacing(0);

    // ── Header ───────────────────────────────────────────────────────────────
    auto *headerRow = new QHBoxLayout();

    auto *appTitle = new QLabel("MaximumTrainer", this);
    appTitle->setStyleSheet("font-size: 28px; font-weight: bold; color: #58a6ff;");

    m_statusBadge = new QLabel("[ TESTING... ]", this);
    m_statusBadge->setStyleSheet(
        "font-size: 14px; color: #f0883e; background: #161b22;"
        "border: 1px solid #30363d; border-radius: 4px; padding: 4px 12px;");
    m_statusBadge->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headerRow->addWidget(appTitle,      1);
    headerRow->addWidget(m_statusBadge, 0, Qt::AlignRight | Qt::AlignVCenter);
    root->addLayout(headerRow);
    root->addSpacing(6);

    // ── Meta row ─────────────────────────────────────────────────────────────
    auto *metaLabel = new QLabel(
        QString("Platform: %1  |  %2  |  %3  |  %4")
            .arg(platform, osName, qtVer, timestamp),
        this);
    metaLabel->setStyleSheet("font-size: 12px; color: #8b949e;");
    root->addWidget(metaLabel);
    root->addSpacing(18);

    auto *sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #21262d;");
    root->addWidget(sep1);
    root->addSpacing(24);

    // ── Result panel ─────────────────────────────────────────────────────────
    auto *panelFrame = new QFrame(this);
    panelFrame->setStyleSheet(
        "QFrame { background: #161b22; border: 1px solid #30363d;"
        "         border-radius: 8px; }");
    auto *panelLayout = new QVBoxLayout(panelFrame);
    panelLayout->setContentsMargins(40, 32, 40, 32);
    panelLayout->setSpacing(20);

    auto *panelTitle = new QLabel(
        QString("Intervals.icu — %1").arg(subtitle), panelFrame);
    panelTitle->setStyleSheet(
        "font-size: 14px; color: #8b949e; font-weight: bold;");
    panelLayout->addWidget(panelTitle);

    m_rowsLayout = new QVBoxLayout();
    m_rowsLayout->setSpacing(16);
    panelLayout->addLayout(m_rowsLayout);

    root->addWidget(panelFrame);
    root->addStretch();

    // ── Footer ───────────────────────────────────────────────────────────────
    auto *sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #21262d;");
    root->addWidget(sep2);
    root->addSpacing(14);

    auto *footerRow = new QHBoxLayout();
    auto *footerLeft = new QLabel(
        QString("intervals.icu — %1").arg(testTitle), this);
    footerLeft->setStyleSheet("font-size: 12px; color: #8b949e;");

    auto *footerRight = new QLabel(
        QString("Artefact: online-mode-%1-%2.png").arg(kPlatformTag, timestamp), this);
    footerRight->setStyleSheet("font-size: 12px; color: #8b949e;");
    footerRight->setAlignment(Qt::AlignRight);

    footerRow->addWidget(footerLeft,  1);
    footerRow->addWidget(footerRight, 1, Qt::AlignRight);
    root->addLayout(footerRow);
}

void ResultWindow::addRow(const QString &key, const QString &value, bool highlight)
{
    auto *row    = new QHBoxLayout();
    auto *keyLbl = new QLabel(key + ":", this);
    keyLbl->setStyleSheet("font-size: 14px; color: #8b949e;");

    auto *valLbl = new QLabel(value, this);
    const QString colour = highlight ? "#7ee787" : "#c9d1d9";
    valLbl->setStyleSheet(
        QString("font-size: 14px; color: %1; font-weight: bold;").arg(colour));

    row->addWidget(keyLbl);
    row->addWidget(valLbl);
    row->addStretch();
    m_rowsLayout->addLayout(row);

    QCoreApplication::processEvents();
}

void ResultWindow::markPassed(const QString &summary)
{
    m_statusBadge->setText(QString("[ PASSED ]  %1").arg(summary));
    m_statusBadge->setStyleSheet(
        "font-size: 14px; color: #3fb950; background: #0d2010;"
        "border: 1px solid #238636; border-radius: 4px; padding: 4px 12px;");
}

void ResultWindow::markFailed(const QString &summary)
{
    m_statusBadge->setText(QString("[ FAILED ]  %1").arg(summary));
    m_statusBadge->setStyleSheet(
        "font-size: 14px; color: #f85149; background: #200d0d;"
        "border: 1px solid #58181a; border-radius: 4px; padding: 4px 12px;");
}

// ─────────────────────────────────────────────────────────────────────────────
// TstOnlineMode -- slot implementations
// ─────────────────────────────────────────────────────────────────────────────
void TstOnlineMode::initTestCase()
{
        m_apiKey    = qEnvironmentVariable("INTERVALS_ICU_API_KEY");
        m_athleteId = qEnvironmentVariable("INTERVALS_ICU_ATHLETE_ID");

        if (m_apiKey.isEmpty() || m_athleteId.isEmpty()) {
            QSKIP("Set INTERVALS_ICU_API_KEY and INTERVALS_ICU_ATHLETE_ID to "
                  "run the online mode authentication test.");
        }

        m_manager = new QNetworkAccessManager(this);
        qApp->setProperty("NetworkManagerWS",
                          QVariant::fromValue<QNetworkAccessManager *>(m_manager));
    }

void TstOnlineMode::cleanupTestCase()
{
        // Clean up any workout created by testWorkoutPush
        if (!m_pushedWorkoutId.isEmpty()) {
            qDebug().noquote() << "[Cleanup] Deleting test workout:" << m_pushedWorkoutId;
            QNetworkReply *del =
                IntervalsIcuService::deleteWorkout(m_athleteId, m_pushedWorkoutId, m_apiKey);
            if (del) {
                waitForReply(del, 15'000);
                const int status =
                    del->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                qDebug().noquote()
                    << "[Cleanup] Delete workout HTTP status:" << status;
                del->deleteLater();
            }
            m_pushedWorkoutId.clear();
        }

        qApp->setProperty("NetworkManagerWS", QVariant());
    }

void TstOnlineMode::testOnlineModeAuthentication()
{
        const QString timestamp =
            QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH-mm-ssZ");
        const QString screenshotName =
            QString("online-mode-%1-%2.png").arg(kPlatformTag, timestamp);
        const QString outPath =
            QCoreApplication::applicationDirPath() + "/" + screenshotName;

        // ── 1. Show OnlineModeWindow ──────────────────────────────────────────
        OnlineModeWindow window(m_athleteId, timestamp);
        window.show();
        QCoreApplication::processEvents();

        // Helper: settle paint events, grab screenshot, save — used on all paths.
        auto grabScreenshot = [&]() {
            QCoreApplication::processEvents();
            QTest::qWait(500);
            QCoreApplication::processEvents();
            QPixmap shot = window.grab();
            if (!shot.isNull())
                shot.save(outPath, "PNG");
            qDebug().noquote() << "[Screenshot] Saved to:" << outPath;
        };

        // ── 2. Make the live auth request ─────────────────────────────────────
        QNetworkReply *reply =
            IntervalsIcuService::getAthlete(m_athleteId, m_apiKey);

        if (!reply) {
            window.markFailed(0, "getAthlete() returned nullptr");
            grabScreenshot();
            QFAIL("getAthlete() returned nullptr — "
                  "NetworkManagerWS app property not registered");
        }

        // ── 3. Wait for the reply (up to 30 s) ────────────────────────────────
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(TIMEOUT_MS, &loop, &QEventLoop::quit);
        loop.exec();

        if (!reply->isFinished()) {
            reply->abort();
            reply->deleteLater();
            window.markFailed(0, "Request timed out after 30 s");
            grabScreenshot();
            QFAIL("Auth request timed out after 30 s — "
                  "check INTERVALS_ICU_API_KEY and network connectivity");
        }

        const int httpStatus =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();

        // ── 4. Parse response and update window ───────────────────────────────
        QString confirmedId;
        QString athleteName;

        // Cache all reply values before deleteLater() to avoid use-after-free.
        const QNetworkReply::NetworkError replyError = reply->error();
        const QString replyErrorString = reply->errorString();

        if (replyError == QNetworkReply::NoError && httpStatus == 200) {
            const QJsonObject obj = QJsonDocument::fromJson(body).object();
            confirmedId  = obj.value(QStringLiteral("id")).toString();
            athleteName  = obj.value(QStringLiteral("name")).toString();
            if (athleteName.isEmpty())
                athleteName = obj.value(QStringLiteral("firstname")).toString()
                            + " "
                            + obj.value(QStringLiteral("lastname")).toString();
            window.markConnected(confirmedId, athleteName.trimmed(), httpStatus);
        } else {
            window.markFailed(httpStatus, replyErrorString);
        }

        reply->deleteLater();

        // ── 5. Settle paint events and grab screenshot ─────────────────────────
        grabScreenshot();

        // ── 6. Assertions ─────────────────────────────────────────────────────
        QVERIFY2(replyError == QNetworkReply::NoError,
                 qPrintable(QString("Network error during auth: %1").arg(replyErrorString)));

        QVERIFY2(httpStatus == 200,
                 qPrintable(
                     QString("Expected HTTP 200, got %1. "
                             "Check INTERVALS_ICU_API_KEY validity.")
                         .arg(httpStatus)));

        QVERIFY2(!confirmedId.isEmpty(),
                 "Auth response did not contain an athlete 'id' field");

        QVERIFY2(confirmedId == m_athleteId,
                 qPrintable(
                     QString("Athlete ID mismatch: requested '%1', got '%2'")
                         .arg(m_athleteId, confirmedId)));

        QVERIFY2(!athleteName.trimmed().isEmpty(),
                 "Auth response returned an empty athlete name");
}

// ─────────────────────────────────────────────────────────────────────────────
// testCalendar
// ─────────────────────────────────────────────────────────────────────────────
void TstOnlineMode::testCalendar()
{
    const QString timestamp =
        QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH-mm-ssZ");
    const QString screenshotName =
        QString("online-mode-%1-calendar-%2.png").arg(kPlatformTag, timestamp);
    const QString outPath =
        QCoreApplication::applicationDirPath() + "/" + screenshotName;

    const QDate startDate = QDate::currentDate().addDays(-7);
    const QDate endDate   = QDate::currentDate().addDays(7);

    ResultWindow window("Calendar Fetch", "Calendar Fetch", timestamp);
    window.addRow("Athlete ID", m_athleteId);
    window.addRow("Date Range",
                  startDate.toString(Qt::ISODate) + " to " + endDate.toString(Qt::ISODate));
    window.show();
    QCoreApplication::processEvents();

    auto grabScreenshot = [&]() {
        QCoreApplication::processEvents();
        QTest::qWait(500);
        QCoreApplication::processEvents();
        const QPixmap shot = window.grab();
        if (!shot.isNull())
            shot.save(outPath, "PNG");
        qDebug().noquote() << "[Screenshot] Saved to:" << outPath;
    };

    QNetworkReply *reply =
        IntervalsIcuService::getEvents(m_athleteId, m_apiKey, startDate, endDate);

    if (!reply) {
        window.markFailed("getEvents() returned nullptr");
        grabScreenshot();
        QFAIL("getEvents() returned nullptr");
    }

    if (!waitForReply(reply)) {
        reply->abort();
        reply->deleteLater();
        window.markFailed("Request timed out after 30 s");
        grabScreenshot();
        QFAIL("Calendar request timed out after 30 s");
    }

    const int        httpStatus  = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body        = reply->readAll();
    const auto       replyError  = reply->error();
    const QString    replyErrStr = reply->errorString();
    reply->deleteLater();

    if (replyError != QNetworkReply::NoError || httpStatus != 200) {
        window.addRow("HTTP Status", QString::number(httpStatus));
        window.markFailed(replyErrStr);
        grabScreenshot();
        QFAIL(qPrintable(
            QString("Calendar fetch failed: HTTP %1  —  %2").arg(httpStatus).arg(replyErrStr)));
    }

    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isArray()) {
        window.addRow("HTTP Status", QString::number(httpStatus) + " OK");
        window.markFailed("Response is not a JSON array");
        grabScreenshot();
        QFAIL("Calendar fetch returned non-array JSON");
    }

    const int eventCount = doc.array().size();
    window.addRow("HTTP Status",  QString::number(httpStatus) + " OK", /*highlight=*/true);
    window.addRow("Event Count",  QString::number(eventCount),         /*highlight=*/true);
    window.markPassed(QString("%1 event(s) in window").arg(eventCount));
    grabScreenshot();

    QVERIFY2(replyError == QNetworkReply::NoError,
             qPrintable(QString("Network error: %1").arg(replyErrStr)));
    QCOMPARE(httpStatus, 200);
}

// ─────────────────────────────────────────────────────────────────────────────
// testWorkoutPush
// ─────────────────────────────────────────────────────────────────────────────
void TstOnlineMode::testWorkoutPush()
{
    const QString timestamp =
        QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH-mm-ssZ");
    const QString screenshotName =
        QString("online-mode-%1-workout-push-%2.png").arg(kPlatformTag, timestamp);
    const QString outPath =
        QCoreApplication::applicationDirPath() + "/" + screenshotName;

    static constexpr const char kWorkoutName[] = "MaximumTrainer CI Test Workout";

    ResultWindow window("Workout Push", "Workout Library — Create", timestamp);
    window.addRow("Athlete ID",    m_athleteId);
    window.addRow("Workout Name",  kWorkoutName);
    window.show();
    QCoreApplication::processEvents();

    auto grabScreenshot = [&]() {
        QCoreApplication::processEvents();
        QTest::qWait(500);
        QCoreApplication::processEvents();
        const QPixmap shot = window.grab();
        if (!shot.isNull())
            shot.save(outPath, "PNG");
        qDebug().noquote() << "[Screenshot] Saved to:" << outPath;
    };

    // ── 1. Fetch folders to get a valid folder_id ─────────────────────────────
    // WorkoutEx requires folder_id (integer) — there is no "folder" string field.
    QNetworkReply *foldersReply = IntervalsIcuService::listFolders(m_athleteId, m_apiKey);
    if (!foldersReply) {
        window.markFailed("listFolders() returned nullptr");
        grabScreenshot();
        QFAIL("listFolders() returned nullptr");
    }
    if (!waitForReply(foldersReply)) {
        foldersReply->abort();
        foldersReply->deleteLater();
        window.markFailed("Folders request timed out");
        grabScreenshot();
        QFAIL("listFolders() timed out");
    }
    const int        foldersStatus = foldersReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray foldersBody   = foldersReply->readAll();
    foldersReply->deleteLater();

    if (foldersStatus != 200) {
        window.markFailed(QString("listFolders HTTP %1").arg(foldersStatus));
        grabScreenshot();
        QFAIL(qPrintable(QString("listFolders failed: HTTP %1\nBody: %2")
                             .arg(foldersStatus)
                             .arg(QString::fromUtf8(foldersBody.left(256)))));
    }

    // Find the first FOLDER (not PLAN) to use as the target.
    int folderId = -1;
    const QJsonArray foldersArr = QJsonDocument::fromJson(foldersBody).array();
    for (const QJsonValue &v : foldersArr) {
        const QJsonObject f = v.toObject();
        if (f.value(QStringLiteral("type")).toString() == QLatin1String("FOLDER")) {
            folderId = f.value(QStringLiteral("id")).toInt(-1);
            if (folderId != -1) break;
        }
    }

    if (folderId == -1) {
        window.markPassed(QString("No FOLDER found in library (got %1 entries) — skipping push")
                              .arg(foldersArr.size()));
        grabScreenshot();
        QSKIP("No workout folder found in athlete library; cannot create workout");
    }

    window.addRow("Folder ID", QString::number(folderId));

    // ── 2. Build and send the workout ────────────────────────────────────────
    // Embed the timestamp so the workout can be identified if automated cleanup fails.
    const QString uniqueName =
        QString("MaximumTrainer CI Test Workout [%1]").arg(timestamp);

    // Use file_contents with a valid ZWO so the workout has downloadable structured data.
    // The 'description' field in WorkoutEx is intervals.icu's native workout format, not
    // free text — sending arbitrary text causes a 422 Unprocessable Entity.
    const QString zwoXml = QString(
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<workout_file>\n"
        "  <author>MaximumTrainer CI</author>\n"
        "  <name>%1</name>\n"
        "  <description>Auto-created by CI integration test. Safe to delete.</description>\n"
        "  <sportType>bike</sportType>\n"
        "  <tags/>\n"
        "  <workout>\n"
        "    <SteadyState Duration=\"300\" Power=\"0.5\"/>\n"
        "  </workout>\n"
        "</workout_file>\n").arg(uniqueName);

    const QByteArray json =
        QJsonDocument(QJsonObject{
            { "name",          uniqueName },
            { "type",          "Ride" },
            { "folder_id",     folderId },
            { "file_contents", zwoXml },
            { "filename",      QStringLiteral("MaximumTrainer_CI_Test.zwo") }
        }).toJson(QJsonDocument::Compact);

    QNetworkReply *reply =
        IntervalsIcuService::createWorkout(m_athleteId, m_apiKey, json);

    if (!reply) {
        window.markFailed("createWorkout() returned nullptr");
        grabScreenshot();
        QFAIL("createWorkout() returned nullptr");
    }

    if (!waitForReply(reply)) {
        reply->abort();
        reply->deleteLater();
        window.markFailed("Request timed out after 30 s");
        grabScreenshot();
        QFAIL("Workout push request timed out after 30 s");
    }

    const int        httpStatus  = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body        = reply->readAll();
    const auto       replyError  = reply->error();
    const QString    replyErrStr = reply->errorString();
    reply->deleteLater();

    if (replyError != QNetworkReply::NoError || httpStatus != 200) {
        window.addRow("HTTP Status", QString::number(httpStatus));
        window.markFailed(replyErrStr);
        grabScreenshot();
        QFAIL(qPrintable(
            QString("Workout push failed: HTTP %1  —  %2\nResponse body: %3")
                .arg(httpStatus).arg(replyErrStr).arg(QString::fromUtf8(body.left(512)))));
    }

    const QJsonObject obj = QJsonDocument::fromJson(body).object();
    // The API returns id as an integer JSON value; use toVariant() for safe string conversion.
    const QString workoutId = obj.value(QStringLiteral("id")).toVariant().toString();

    if (workoutId.isEmpty()) {
        window.addRow("HTTP Status", QString::number(httpStatus) + " OK");
        window.markFailed("Response did not contain an 'id' field");
        grabScreenshot();
        QFAIL("createWorkout response missing 'id' field");
    }

    m_pushedWorkoutId = workoutId;   // stored for cleanup in cleanupTestCase

    window.addRow("Workout Name",  uniqueName);
    window.addRow("HTTP Status",        QString::number(httpStatus) + " OK", /*highlight=*/true);
    window.addRow("Created Workout ID", workoutId,                            /*highlight=*/true);
    window.markPassed("Workout created successfully");
    grabScreenshot();

    QVERIFY2(replyError == QNetworkReply::NoError,
             qPrintable(QString("Network error: %1").arg(replyErrStr)));
    QCOMPARE(httpStatus, 200);
    QVERIFY2(!workoutId.isEmpty(), "createWorkout response missing 'id' field");
}

// ─────────────────────────────────────────────────────────────────────────────
// testWorkoutPull
// ─────────────────────────────────────────────────────────────────────────────
void TstOnlineMode::testWorkoutPull()
{
    const QString timestamp =
        QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH-mm-ssZ");
    const QString screenshotName =
        QString("online-mode-%1-workout-pull-%2.png").arg(kPlatformTag, timestamp);
    const QString outPath =
        QCoreApplication::applicationDirPath() + "/" + screenshotName;

    ResultWindow window("Workout Pull", "Workout Library — Download ZWO", timestamp);
    window.addRow("Athlete ID", m_athleteId);
    window.show();
    QCoreApplication::processEvents();

    auto grabScreenshot = [&]() {
        QCoreApplication::processEvents();
        QTest::qWait(500);
        QCoreApplication::processEvents();
        const QPixmap shot = window.grab();
        if (!shot.isNull())
            shot.save(outPath, "PNG");
        qDebug().noquote() << "[Screenshot] Saved to:" << outPath;
    };

    // We fetch the workout created by testWorkoutPush, then convert it to ZWO.
    // If push didn't succeed this test skips rather than fails.
    if (m_pushedWorkoutId.isEmpty()) {
        window.markPassed("testWorkoutPush did not store a workout ID — skipping ZWO download");
        grabScreenshot();
        QSKIP("testWorkoutPush did not produce a workout ID; skipping ZWO download test");
    }

    window.addRow("Workout ID", m_pushedWorkoutId);

    // ── Step 1: GET /athlete/{id}/workouts/{workoutId} ───────────────────────
    QNetworkReply *getReply =
        IntervalsIcuService::getWorkout(m_athleteId, m_pushedWorkoutId, m_apiKey);

    if (!getReply) {
        window.markFailed("getWorkout() returned nullptr");
        grabScreenshot();
        QFAIL("getWorkout() returned nullptr");
    }

    if (!waitForReply(getReply)) {
        getReply->abort();
        getReply->deleteLater();
        window.markFailed("GET workout timed out after 30 s");
        grabScreenshot();
        QFAIL("getWorkout() timed out after 30 s");
    }

    const int        getStatus  = getReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray getBody    = getReply->readAll();
    const auto       getError   = getReply->error();
    const QString    getErrStr  = getReply->errorString();
    getReply->deleteLater();

    if (getError != QNetworkReply::NoError || getStatus != 200) {
        window.addRow("GET HTTP Status", QString::number(getStatus));
        window.markFailed(getErrStr);
        grabScreenshot();
        QFAIL(qPrintable(
            QString("GET workout failed: HTTP %1  —  %2\nResponse body: %3")
                .arg(getStatus).arg(getErrStr).arg(QString::fromUtf8(getBody.left(512)))));
    }

    window.addRow("GET HTTP Status", QString::number(getStatus) + " OK", /*highlight=*/true);

    // ── Step 2: POST /athlete/{id}/download-workout.zwo ─────────────────────
    QNetworkReply *zwoReply =
        IntervalsIcuService::convertWorkoutToZwo(m_athleteId, m_apiKey, getBody);

    if (!zwoReply) {
        window.markFailed("convertWorkoutToZwo() returned nullptr");
        grabScreenshot();
        QFAIL("convertWorkoutToZwo() returned nullptr");
    }

    if (!waitForReply(zwoReply)) {
        zwoReply->abort();
        zwoReply->deleteLater();
        window.markFailed("ZWO conversion timed out after 30 s");
        grabScreenshot();
        QFAIL("convertWorkoutToZwo() timed out after 30 s");
    }

    const int        zwoStatus  = zwoReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray zwoBody    = zwoReply->readAll();
    const auto       zwoError   = zwoReply->error();
    const QString    zwoErrStr  = zwoReply->errorString();
    zwoReply->deleteLater();

    if (zwoError != QNetworkReply::NoError || zwoStatus != 200) {
        window.addRow("ZWO HTTP Status", QString::number(zwoStatus));
        window.markFailed(zwoErrStr);
        grabScreenshot();
        QFAIL(qPrintable(
            QString("ZWO conversion failed: HTTP %1  —  %2\nResponse body: %3")
                .arg(zwoStatus).arg(zwoErrStr).arg(QString::fromUtf8(zwoBody.left(512)))));
    }

    const bool isXml = zwoBody.startsWith("<?xml") || zwoBody.startsWith("<workout_file");

    window.addRow("ZWO HTTP Status", QString::number(zwoStatus) + " OK", /*highlight=*/true);
    window.addRow("ZWO Size",        QString("%1 bytes").arg(zwoBody.size()), /*highlight=*/true);
    window.addRow("Format",          isXml ? "XML (valid)" : "Not XML");

    if (!isXml) {
        window.markFailed("Response is not XML");
        grabScreenshot();
        QFAIL(qPrintable(
            QString("ZWO response does not start with XML header. First 64 bytes: %1")
                .arg(QString::fromUtf8(zwoBody.left(64)))));
    }

    window.markPassed("ZWO downloaded and validated");
    grabScreenshot();

    QCOMPARE(zwoStatus, 200);
    QVERIFY2(zwoBody.size() > 0, "ZWO response body is empty");
    QVERIFY2(isXml, "ZWO response is not valid XML");
}

QTEST_MAIN(TstOnlineMode)
