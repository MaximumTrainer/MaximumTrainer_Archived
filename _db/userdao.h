#ifndef USERDAO_H
#define USERDAO_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

#include "account.h"
#include "settings.h"

class UserDAO
{
public:



    static QNetworkReply* putAccount(Account *account);

//    static QNetworkReply* getAccount(int id, QString session_mt_id);







};

#endif // USERDAO_H
