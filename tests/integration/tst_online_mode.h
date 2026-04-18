#pragma once

#include <QtTest/QtTest>
#include <QWidget>
#include <QLabel>
#include <QNetworkAccessManager>

// ─────────────────────────────────────────────────────────────────────────────
// OnlineModeWindow
//
// 1280x720 window that represents the MaximumTrainer online-mode login state.
// Q_OBJECT declared here (in the header) so qmake reliably runs moc on it.
// ─────────────────────────────────────────────────────────────────────────────
class OnlineModeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OnlineModeWindow(const QString &athleteId,
                              const QString &timestamp,
                              QWidget       *parent = nullptr);

    void markConnected(const QString &confirmedId, const QString &athleteName, int httpStatus);
    void markFailed(int httpStatus, const QString &errorString);

private:
    QLabel *m_statusBadge      = nullptr;
    QLabel *m_requestedIdLabel = nullptr;
    QLabel *m_confirmedIdLabel = nullptr;
    QLabel *m_athleteNameLabel = nullptr;
    QLabel *m_httpStatusLabel  = nullptr;
    QLabel *m_buildLabel       = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// TstOnlineMode — QTest class
//
// Q_OBJECT declared here (in the header) so qmake reliably runs moc on it.
// ─────────────────────────────────────────────────────────────────────────────
class TstOnlineMode : public QObject
{
    Q_OBJECT

    QString                m_apiKey;
    QString                m_athleteId;
    QNetworkAccessManager *m_manager = nullptr;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testOnlineModeAuthentication();
};
