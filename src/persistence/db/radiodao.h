#ifndef RADIODAO_H
#define RADIODAO_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

class RadioDAO
{
public:


    static QNetworkReply* getAllRadios();
};

#endif // RADIODAO_H


