#ifndef VERSIONDAO_H
#define VERSIONDAO_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>



class VersionDAO
{
public:

    static QNetworkReply* getVersion();
};

#endif // VERSIONDAO_H




