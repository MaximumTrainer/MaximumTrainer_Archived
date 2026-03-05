#include "radiodao.h"



#include "environnement.h"





//http://localhost/index.php/api/radio_rest/radio/format/json
//https://maximumtrainer.com/index.php/api/radio_rest/radio/format/json
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* RadioDAO::getAllRadios() {



    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();

    const QString url =  Environnement::getURLEnvironnementWS() + "api/radio_rest/radio/format/json";

    qDebug() << "RadioDAO:GetAllRadios" << url;

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

    QNetworkReply *replyLogin = managerWS->get(request);


    return replyLogin;
}
