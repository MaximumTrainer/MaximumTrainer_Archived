#include "extrequest.h"
#include "util.h"


#include <QHttpMultiPart>




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* ExtRequest::checkGoogleConnection() {


    qDebug() << "check Google Start";
    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();

    const QString urlGoogle = "http://www.google.com/";
    QNetworkRequest request2;
    request2.setUrl(QUrl(urlGoogle));
    request2.setRawHeader("User-Agent", "MyOwnBrowser 1.0");
    QNetworkReply *replyGoogle = managerWS->get(request2);

    return replyGoogle;


}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* ExtRequest::checkIpAddress() {

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();

    const QString url2 = "http://bot.whatismyipaddress.com/";
    QNetworkRequest request1;
    request1.setUrl(QUrl(url2));
    request1.setRawHeader("User-Agent", "MyOwnBrowser 1.0");
    QNetworkReply *replyMyIp = managerWS->get(request1);
    return replyMyIp;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* ExtRequest::stravaDeauthorization(QString access_token) {

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();

    const QString url =  "https://www.strava.com/oauth/deauthorize";
    qDebug() << "URL IS:" << url;;
    QUrlQuery postData;
    postData.addQueryItem("access_token", access_token);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    QNetworkReply *replyPutUser = managerWS->post(request, postData.toString(QUrl::FullyEncoded).toUtf8() );

    qDebug() << "putAccount end";
    return replyPutUser;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* ExtRequest::stravaCheckUploadStatus(QString access_token, int uploadID) {


    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();

    QString urlStrava = "https://www.strava.com/api/v3/uploads/" + QString::number(uploadID);

    QNetworkRequest request;
    request.setUrl(QUrl(urlStrava));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    request.setRawHeader("Authorization", "Bearer " + access_token.toUtf8());

    QNetworkReply *replyLogin = managerWS->get(request);

    return replyLogin;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* ExtRequest::stravaUploadFile(QString access_token, QString activityName, QString activityDescription,
                                            bool activityOnTrainer, bool activityIsPrivate, QString typeActivity, QString pathToFile) {


    QFileInfo fileInfo(pathToFile);
    QString fileName = fileInfo.fileName(); //just the filename without the path
    qDebug() << "stravaUploadFile" << pathToFile << "fileName:" << fileName;


    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();

    const QString urlStrava = "https://www.strava.com/api/v3/uploads";
    const QString fileType = "fit";

    QString fileActivityType;
    if (typeActivity == "course") {
        fileActivityType = "ride";
    }
    else {
        //        fileActivityType = "workout";
        //        fileActivityType = "cycling";
        fileActivityType = "ride";
    }

    QString reference = QApplication::translate("ExtRequest: ", " - Activity done with MaximumTrainer.com");
    activityDescription = activityDescription + reference;

    //activities without lat/lng info in the file are auto marked as stationary, set to 1 to force
    QString activityOnTrainerStr = "1";
    if (!activityOnTrainer) {
        activityOnTrainerStr = "0";
    }

    //activities without lat/lng info in the file are auto marked as stationary, set to 1 to force
    QString activityIsPrivateStr = "1";
    if (!activityIsPrivate) {
        activityIsPrivateStr = "0";
    }



    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart accessPart;
    accessPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"access_token\""));
    accessPart.setBody(access_token.toUtf8());

    QHttpPart activityNamePart;
    activityNamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"name\""));
    activityNamePart.setBody(activityName.toUtf8());

    QHttpPart activityDescriptionPart;
    activityDescriptionPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"description\""));
    activityDescriptionPart.setBody(activityDescription.toUtf8());

    QHttpPart activityPrivatePart;
    activityPrivatePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"private\""));
    activityPrivatePart.setBody(activityIsPrivateStr.toUtf8());

    QHttpPart activityTrainerPart;
    activityTrainerPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"trainer\""));
    activityTrainerPart.setBody(activityOnTrainerStr.toUtf8());

    QHttpPart activityTypePart;
    activityTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"activity_type\""));
    activityTypePart.setBody(fileActivityType.toUtf8());

    QHttpPart fileDataPart;
    QString contentDispoHeaderStr = "form-data; name=\"file\"; filename=\"" + fileName + "\"";
    fileDataPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(contentDispoHeaderStr));
    fileDataPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));

    QFile *file = new QFile(pathToFile);
    if (!file->open(QIODevice::ReadOnly))
        return nullptr;

    qDebug() << "test file size:" << file->size();
    fileDataPart.setBodyDevice(file);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart


    QHttpPart fileTypePart;
    fileTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data_type\""));
    fileTypePart.setBody(fileType.toUtf8());


    multiPart->append(accessPart);
    multiPart->append(activityNamePart);
    multiPart->append(activityDescriptionPart);
    multiPart->append(activityPrivatePart);
    multiPart->append(activityTrainerPart);
    multiPart->append(activityTypePart);
    multiPart->append(fileDataPart);
    multiPart->append(fileTypePart);


    QUrl url(urlStrava);
    QNetworkRequest request(url);

    QNetworkReply *reply = managerWS->post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply

    return reply;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* ExtRequest::trainingPeaksRefreshToken(QString access_token, QString refresh_token)
{
    qDebug() << "TrainingPeaksRefreshToken, " << access_token << " refresh is :" << refresh_token;

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();

    qDebug() << "URL IS:" << URL_TOKEN_TP;
    QUrlQuery postData;
    postData.addQueryItem("client_id", CLIENT_ID_TP);
    postData.addQueryItem("client_secret", CLIENT_SECRET_TP);
    postData.addQueryItem("grant_type", "refresh_token");
    postData.addQueryItem("refresh_token", refresh_token);

    QNetworkRequest request;
    request.setUrl(URL_TOKEN_TP);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    QNetworkReply *replyPutUser = managerWS->post(request, postData.toString(QUrl::FullyEncoded).toUtf8() );

    qDebug() << "putAccount end";
    return replyPutUser;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* ExtRequest::trainingPeaksUploadFile(QString access_token, bool workoutPublic,
                                                   QString activityName, QString activityDescription, QString pathToFile) {



    QFileInfo fileInfo(pathToFile);
    QString fileName = fileInfo.fileName(); //just the filename without the path
    qDebug() << "trainingPeaksUploadFile" << pathToFile << "fileName:" << fileName;


    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();

    const QString urlPost = URL_POST_FILE_TP;


    QString reference = QApplication::translate("ExtRequest: ", " - Activity done with MaximumTrainer.com");
    activityDescription = activityDescription + reference;

//    QString activityIsPublicStr = "1";
//    if (!workoutPublic) {
//        activityIsPublicStr = "0";
//    }

    QString boolPublicText = workoutPublic ? "true" : "false";


    QFile *file = new QFile(pathToFile);
    if (!file->open(QIODevice::ReadOnly))
        return nullptr;
    QByteArray base64Encoded  = QByteArray(file->readAll().toBase64());


    QString jsonString = QString("{");
    jsonString.append("\"UploadClient\":");
    jsonString.append("\"MaximumTrainer.com\"");

    jsonString.append(",\"Filename\":");
    jsonString.append("\"" + fileName.toUtf8() + "\"");

     //string, required. Base64 encoded file contents. GZipped files are supported to decrease size, especially for large XML format files.
    jsonString.append(",\"Data\":");
    jsonString.append("\"" + base64Encoded  + "\"");

    qDebug() << "WorkoutPublic TP?" << boolPublicText ;
    jsonString.append(",\"SetWorkoutPublic\":");
    jsonString.append("\"" + boolPublicText + "\"");

    jsonString.append(",\"Title\":");
    jsonString.append("\"" + activityName.toUtf8() + "\"");

    jsonString.append(",\"Comment\":");
    jsonString.append("\"" + activityDescription.toUtf8() + "\"");

    jsonString.append(",\"Type\":");
    jsonString.append("\"Bike\"");

    jsonString.append("}");

    qDebug() << "JSON TO POST TP IS: " << jsonString;


    QUrl url(urlPost);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + access_token.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, jsonString.size());

    QNetworkReply *reply = managerWS->post(request, jsonString.toUtf8());
    file->deleteLater();

    return reply;

}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* ExtRequest::selfloopsUploadFile(QString email, QString password, QString pathToFile, QString note) {


    //    pathToFile = "C:/2015-07-31-05-31-36.fit";
    //    QString pathToZip = pathToFile + ".gz";


    QString pathToZip = pathToFile + ".gz";
    Util::zipFileToDisk(pathToFile, pathToZip, true);

    QFileInfo fileInfo(pathToZip);
    QString fileName = fileInfo.fileName(); //just the filename without the path


    qDebug() << "selfloopsUploadFile"  << "pathToFile:" << pathToFile << "pathToZip" << pathToZip ;


    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    const QString urlSelfloops = "https://www.selfloops.com/restapi/maximumtrainer/activities/upload.json";


    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart userPart;
    userPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"email\""));
    userPart.setBody(email.toUtf8());

    QHttpPart pwPart;
    pwPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"pw\""));
    pwPart.setBody(password.toUtf8());



    QHttpPart fileDataPart;
    QString contentDispoHeaderStr = "form-data; name=\"fitfile\"; filename=\"" + fileName + "\"";
    fileDataPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(contentDispoHeaderStr));
    fileDataPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-gzip"));

    QFile *file = new QFile(pathToZip);
    if (!file->open(QIODevice::ReadOnly))
        return nullptr;

    qDebug() << "test file size:" << file->size();

    fileDataPart.setBodyDevice(file);
    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart


    //--------------------------------------
    QHttpPart notePart;
    notePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"note\""));
    notePart.setBody(note.toUtf8());


    multiPart->append(userPart);
    multiPart->append(pwPart);
    multiPart->append(fileDataPart);
    multiPart->append(notePart);


    QUrl url(urlSelfloops);
    QNetworkRequest request(url);

    QNetworkReply *reply = managerWS->post(request, multiPart);
    multiPart->setParent(reply); // delete the multiPart with the reply

    return reply;
}


