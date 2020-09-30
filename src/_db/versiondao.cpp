#include "versiondao.h"

#include "environnement.h"




// http://localhost/index.php/api/version_rest/version/lang/notused/os/noteused/format/json
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QNetworkReply* VersionDAO::getVersion() {

    QNetworkAccessManager *managerWS = qApp->property("NetworkManagerWS").value<QNetworkAccessManager*>();


    const QString url =  Environnement::getURLEnvironnementWS() + "api/version_rest/version/format/json";


    qDebug() << "Get Version: " << url;


    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("User-Agent", "MyOwnBrowser 1.0");

    QNetworkReply *replyLogin = managerWS->get(request);
    return replyLogin;
}

