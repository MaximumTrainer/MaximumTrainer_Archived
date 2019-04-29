#ifndef ACHIEVEMENTDAO_H
#define ACHIEVEMENTDAO_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>


class AchievementDAO
{

public:
    static QNetworkReply* getLstAchievement();
    static QNetworkReply* getLstAchievementForUser(int account_id);

    static QNetworkReply* putAchievement(int account_id, int achievement_id);



};

#endif // ACHIEVEMENTDAO_H
