#ifndef SENSORDAO_H
#define SENSORDAO_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>


class SensorDAO
{
public:

    static QNetworkReply* getActiveSensorList(int account_id);
};

#endif // SENSORDAO_H
