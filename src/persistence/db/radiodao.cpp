#include "radiodao.h"

#include "environnement.h"
#include "logger.h"





//http://localhost/index.php/api/radio_rest/radio/format/json
//https://maximumtrainer.com/index.php/api/radio_rest/radio/format/json
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* RadioDAO::getAllRadios() {

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();
    if (!managerWS) {
        LOG_WARN("RadioDAO", QStringLiteral("getAllRadios: NetworkManagerWS not available"));
        return nullptr;
    }

    const QString url =  Environnement::getURLEnvironnementWS() + "api/radio_rest/radio/format/json";

    LOG_DEBUG("RadioDAO", QStringLiteral("getAllRadios: GET ") + url);

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

    QNetworkReply *replyLogin = managerWS->get(request);

    return replyLogin;
}
