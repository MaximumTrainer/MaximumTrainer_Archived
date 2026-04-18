#ifndef SELFLOOPS_SERVICE_H
#define SELFLOOPS_SERVICE_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

///
/// API client for Selfloops (https://www.selfloops.com).
///
/// Authentication: email + password sent as multipart form fields on every
/// request — Selfloops uses HTTP basic-style credentials per-upload rather
/// than an OAuth bearer token.
///
/// Credentials are set via setCredentials() and are held in memory only
/// (encrypted persistence is handled by Account::saveSelfloopsCredentials()).
///
/// All methods are non-blocking and return a pending QNetworkReply*.
/// The caller must connect QNetworkReply::finished to a slot.
///
class SelfloopsService
{
public:
    /// Store the email + password used for all subsequent instance calls.
    void setCredentials(const QString &email, const QString &password);

    QString email()    const { return m_email;    }
    QString password() const { return m_password; }

    // ── API methods ──────────────────────────────────────────────────────────

    /// Upload a .fit (or .tcx) activity to Selfloops.
    /// The file is gzip-compressed before upload (a .gz temp file is created
    /// adjacent to the original, then deleted by the reply cleanup path via
    /// the QFile parent-object mechanism).
    ///
    /// POST https://www.selfloops.com/restapi/maximumtrainer/activities/upload.json
    /// @param filePath   Absolute path to the activity file (.fit or .tcx).
    /// @param note       Free-text activity description / note.
    QNetworkReply* uploadActivity(const QString &filePath, const QString &note);

private:
    QString m_email;
    QString m_password;

    static QNetworkAccessManager* networkManager();
};

#endif // SELFLOOPS_SERVICE_H
