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

};

#endif // EXTREQUEST_H
