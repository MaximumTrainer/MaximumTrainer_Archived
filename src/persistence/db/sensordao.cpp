#include "sensordao.h"

#include "account.h"
#include "environnement.h"
#include "logger.h"



// http://localhost/index.php/api/sensor_rest/sensor/id/1
// https://maximumtrainer.com/api/sensor_rest/sensor/id/1
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* SensorDAO::getActiveSensorList(int account_id) {

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!managerWS) {
        LOG_WARN("SensorDAO", QStringLiteral("getActiveSensorList: NetworkManagerWS not available"));
        return nullptr;
    }

    const QString url =  Environnement::getURLEnvironnementWS() + "api/sensor_rest/sensor" +
            "/id/" + QString::number(account_id) +
            "/format/json";

    LOG_DEBUG("SensorDAO", QStringLiteral("getActiveSensorList: GET ") + url);

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

    QNetworkReply *replyLogin = managerWS->get(request);

    return replyLogin;
}


