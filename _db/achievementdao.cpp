#include "achievementdao.h"


#include "environnement.h"





// http://localhost/index.php/api/achievement_rest/achievement/format/json
// https://maximumtrainer.com/api/sensor_rest/sensor/id/1
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* AchievementDAO::getLstAchievement() {

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();


    const QString url =  Environnement::getURLEnvironnementWS() + "api/achievement_rest/achievement/format/json";


    qDebug() << "OK CHECKING ACTIVE LIST WITH URL" << url;

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

    QNetworkReply *reply = managerWS->get(request);

    return reply;
}



// http://localhost/index.php/api/achievement_rest/achievementuser/id/1/format/json
// https://maximumtrainer.com/api/sensor_rest/sensor/id/1
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* AchievementDAO::getLstAchievementForUser(int account_id) {

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();



    const QString url =  Environnement::getURLEnvironnementWS() + "api/achievement_rest/achievementuser" +
            "/id/" + QString::number(account_id) +
            "/format/json";


    qDebug() << "OK CHECKING ACTIVE LIST WITH URL" << url;

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

    QNetworkReply *reply = managerWS->get(request);

    return reply;
}





// http://localhost/index.php/api/achievement_rest/achievement/
// https://www.dropbox.com/s/goo1xia31e83e7b/achievementRest.png?dl=0
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* AchievementDAO::putAchievement(int account_id, int achievement_id) {


    qDebug() << "putAchievement start";
    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();


    const QString url =  Environnement::getURLEnvironnementWS() + "api/achievement_rest/achievement/";
    qDebug() << "URL IS:" << url;;

    //Todo: add security so you cant "cheat" achievements?
    QUrlQuery postData;



    /// ----- Data to put
    qDebug() << "PUT DATA:" << account_id << "achiemveent_id:" << achievement_id;


    postData.addQueryItem("account_id", QString::number(account_id));
    postData.addQueryItem("achievement_id", QString::number(achievement_id));



    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    QNetworkReply *replyPutUser = managerWS->put(request, postData.toString(QUrl::FullyEncoded).toUtf8() );

    qDebug() << "putAccount end";
    return replyPutUser;
}
