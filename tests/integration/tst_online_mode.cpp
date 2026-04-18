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

#include <QtTest/QtTest>
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScreen>
#include <QPixmap>
#include <QDir>
#include <QSysInfo>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
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
// OnlineModeWindow
//
// 1280x720 window that represents the MaximumTrainer online-mode login state.
// Shows a status badge that transitions from "AUTHENTICATING..." to
// "CONNECTED" (green) or "AUTH FAILED" (red), and displays the athlete
// name and ID returned from the live intervals.icu API.
// ─────────────────────────────────────────────────────────────────────────────
class OnlineModeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OnlineModeWindow(const QString &athleteId,
                              const QString &timestamp,
                              QWidget       *parent = nullptr)
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

    void markConnected(const QString &confirmedId, const QString &athleteName, int httpStatus)
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

    void markFailed(int httpStatus, const QString &errorString)
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

private:
    QLabel *m_statusBadge      = nullptr;
    QLabel *m_requestedIdLabel = nullptr;
    QLabel *m_confirmedIdLabel = nullptr;
    QLabel *m_athleteNameLabel = nullptr;
    QLabel *m_httpStatusLabel  = nullptr;
    QLabel *m_buildLabel       = nullptr;
};


// ─────────────────────────────────────────────────────────────────────────────
// TstOnlineMode -- QTest class
// ─────────────────────────────────────────────────────────────────────────────
class TstOnlineMode : public QObject
{
    Q_OBJECT

    QString               m_apiKey;
    QString               m_athleteId;
    QNetworkAccessManager *m_manager = nullptr;

private slots:

    void initTestCase()
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

    void cleanupTestCase()
    {
        qApp->setProperty("NetworkManagerWS", QVariant());
    }

    /*
     * testOnlineModeAuthentication
     *
     * 1.  Shows the OnlineModeWindow with "AUTHENTICATING..." badge.
     * 2.  Calls IntervalsIcuService::getAthlete() — same path as the app.
     * 3.  Waits up to 30 s for the HTTP reply.
     * 4.  Updates the window badge and athlete fields.
     * 5.  Grabs a labelled 1280x720 screenshot and saves it as build evidence.
     * 6.  Asserts HTTP 200, no network error, and matching athlete ID.
     */
    void testOnlineModeAuthentication()
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

        // ── 2. Make the live auth request ─────────────────────────────────────
        QNetworkReply *reply =
            IntervalsIcuService::getAthlete(m_athleteId, m_apiKey);

        QVERIFY2(reply,
                 "getAthlete() returned nullptr — "
                 "NetworkManagerWS app property not registered");

        // ── 3. Wait for the reply (up to 30 s) ────────────────────────────────
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer::singleShot(TIMEOUT_MS, &loop, &QEventLoop::quit);
        loop.exec();

        QVERIFY2(reply->isFinished(),
                 "Auth request timed out after 30 s — "
                 "check INTERVALS_ICU_API_KEY and network connectivity");

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
        QCoreApplication::processEvents();
        QTest::qWait(500);
        QCoreApplication::processEvents();

        QPixmap screenshot = window.grab();
        QVERIFY2(!screenshot.isNull(), "Screenshot grab() returned null pixmap");

        const bool saved = screenshot.save(outPath, "PNG");
        QVERIFY2(saved,
                 qPrintable(QString("Failed to save screenshot: %1").arg(outPath)));

        qDebug().noquote() << "[Screenshot] Saved to:" << outPath;

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

        qDebug().noquote()
            << "[Auth] Athlete:" << athleteName
            << "  ID:" << confirmedId
            << "  HTTP:" << httpStatus;
    }
};

QTEST_MAIN(TstOnlineMode)
#include "tst_online_mode.moc"
