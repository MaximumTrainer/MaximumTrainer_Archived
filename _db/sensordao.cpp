#include "sensordao.h"

#include "account.h"
#include "environnement.h"



// http://localhost/index.php/api/sensor_rest/sensor/id/1
// https://maximumtrainer.com/api/sensor_rest/sensor/id/1
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* SensorDAO::getActiveSensorList(int account_id) {

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();


    const QString url =  Environnement::getURLEnvironnementWS() + "api/sensor_rest/sensor" +
            "/id/" + QString::number(account_id) +
            "/format/json";

//    const QString url =  Environnement::getURLEnvironnementWS() + "api/sensor_rest/sensor" +
//            "/id/" + QString::number(714) +
//            "/format/json";

    qDebug() << "OK CHECKING ACTIVE LIST WITH URL" << url;

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

    QNetworkReply *replyLogin = managerWS->get(request);



    return replyLogin;
}


