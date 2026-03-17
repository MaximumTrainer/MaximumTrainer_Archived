#ifndef EXTREQUEST_H
#define EXTREQUEST_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QApplication>
#include <QtCore>
#include "environnement.h"




class ExtRequest
{
public:


    static QNetworkReply* checkGoogleConnection();
    static QNetworkReply* checkIpAddress();

    //-- Strava
    static QNetworkReply* stravaDeauthorization(QString access_token);
    static QNetworkReply* stravaUploadFile(QString access_token, QString activityName, QString activityDescription,
                                           bool activityOnTrainer, bool activityIsPrivate, QString typeActivity, QString pathToFile);
    static QNetworkReply* stravaCheckUploadStatus(QString access_token, int uploadID);


    //-- Training Peaks
    static QNetworkReply* trainingPeaksRefreshToken(QString access_token, QString refresh_token);
    static QNetworkReply* trainingPeaksUploadFile(QString access_token, bool workoutPublic, QString activityName, QString activityDescription, QString pathToFile);

    //-- SelfLoops
    static QNetworkReply* selfloopsUploadFile(QString email, QString password, QString pathToFile, QString note);

    //-- Intervals.icu OAuth2
    /// Exchange an authorization code for an access token by POSTing directly
    /// to the Intervals.icu token endpoint.  This is used as a client-side
    /// fallback when the MaximumTrainer.com backend proxy is unavailable.
    /// @param code        The authorization code received from the redirect URI.
    /// @param redirectUri The exact redirect_uri used in the authorization request.
    static QNetworkReply* intervalsIcuOAuthExchange(const QString &code, const QString &redirectUri);

};

#endif // EXTREQUEST_H
