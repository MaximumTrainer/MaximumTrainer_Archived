#ifndef STRAVA_SERVICE_H
#define STRAVA_SERVICE_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

///
/// API client for Strava (https://www.strava.com/api/v3).
///
/// Authentication: OAuth2 Bearer token set via setAccessToken().
/// Tokens expire every 6 hours; use the static refreshToken() to obtain a new
/// access token from a stored refresh token.
///
/// All methods are non-blocking and return a pending QNetworkReply*.
/// The caller must connect QNetworkReply::finished to a slot and call
/// reply->readAll() inside it — identical to the existing ExtRequest pattern.
///
class StravaService
{
public:
    /// Store the OAuth2 Bearer token used for all subsequent instance calls.
    void setAccessToken(const QString &token);

    QString accessToken() const { return m_accessToken; }

    // ── API methods ──────────────────────────────────────────────────────────

    /// Upload a FIT activity file to Strava.
    /// POST https://www.strava.com/api/v3/uploads
    /// @param filePath      Absolute path to the .fit file.
    /// @param name          Activity name shown in Strava.
    /// @param description   Activity description (a MaximumTrainer reference is appended).
    /// @param isPrivate     true → visible only to the athlete.
    /// @param onTrainer     true → marks the activity as an indoor trainer ride.
    /// @param activityType  Strava sport type string (default "ride").
    QNetworkReply* uploadActivity(const QString &filePath,
                                  const QString &name,
                                  const QString &description,
                                  bool isPrivate    = false,
                                  bool onTrainer    = true,
                                  const QString &activityType = QStringLiteral("ride"));

    /// Poll the status of a pending Strava upload.
    /// GET https://www.strava.com/api/v3/uploads/{uploadId}
    QNetworkReply* checkUploadStatus(int uploadId);

    /// Revoke the current access token.
    /// POST https://www.strava.com/oauth/deauthorize
    QNetworkReply* deauthorize();

    /// Exchange a refresh token for a new access + refresh token pair.
    /// POST https://www.strava.com/oauth/token
    /// @param clientId      Strava application client_id (see environnement.h).
    /// @param clientSecret  Strava application client_secret.
    /// @param refreshToken  Refresh token stored from the previous OAuth exchange.
    static QNetworkReply* refreshToken(const QString &clientId,
                                       const QString &clientSecret,
                                       const QString &refreshToken);

private:
    QString m_accessToken;

    QNetworkRequest buildBearerRequest(const QString &url) const;
    static QNetworkAccessManager* networkManager();
};

#endif // STRAVA_SERVICE_H
