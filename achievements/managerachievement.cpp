#include "managerachievement.h"
#include "util.h"
#include "achievementchecker.h"

#include "achievementdao.h"



ManagerAchievement::ManagerAchievement(QObject *parent) : QObject(parent)
{

    this->account = qApp->property("Account").value<Account*>();


    // number of achievements in the DB at the moment, increase when adding more
    numberOfAchievement = 7;


    // Retrieve achievement list from DB
    replyGetListAchievement = AchievementDAO::getLstAchievement();
    connect(replyGetListAchievement, SIGNAL(finished()), this, SLOT(slotGetAchievementListFinished()) );



}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ManagerAchievement::checkMAPAchievement(int lastStepCompleted) {


    qDebug() << "checkMAPAchievement" << lastStepCompleted;

    if (lastStepCompleted < 6)
        return;

    // list could be empty if fetch achievement from DB failed
    if (lstAchievement.size() < numberOfAchievement) {
        return;
    }

    // Step 6 completed Achievement
    int indexVal = 5;
    if (lastStepCompleted == 6  && !lstAchievement[indexVal].isCompleted() ) {
        lstAchievement[indexVal].setCompleted(true);
        emit achievementCompleted(lstAchievement.at(indexVal));
        AchievementDAO::putAchievement(account->id, lstAchievement[indexVal].getId() );
    }

    // Step 7 completed Achievement
    indexVal = 6;
    if (lastStepCompleted == 7  && !lstAchievement[indexVal].isCompleted() ) {
        lstAchievement[indexVal].setCompleted(true);
        emit achievementCompleted(lstAchievement.at(indexVal));
        AchievementDAO::putAchievement(account->id, lstAchievement[indexVal].getId() );
    }



}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ManagerAchievement::updateMinuteRode(int minutes) {

    account->minutes_rode += minutes;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ManagerAchievement::workoutCompleted(Workout workout) {

    // list could be empty if fetch achievement from DB failed
    if (lstAchievement.size() < numberOfAchievement) {
        return;
    }

    int secTotal = Util::convertQTimeToSecD(workout.getDurationQTime());


    // Endurance Starter (1hr)
    int indexVal = 0;
    if (!lstAchievement[indexVal].isCompleted() && secTotal >= 60*60) {  //60*60=1h
        lstAchievement[indexVal].setCompleted(true);
        emit achievementCompleted(lstAchievement.at(indexVal));
        AchievementDAO::putAchievement(account->id, lstAchievement[indexVal].getId());
    }

    // Endurance Intermediate (2hr)
    indexVal = 1;
    if (!lstAchievement[indexVal].isCompleted() && secTotal >= 60*60*2) {
        lstAchievement[indexVal].setCompleted(true);
        emit achievementCompleted(lstAchievement.at(indexVal));
        AchievementDAO::putAchievement(account->id, lstAchievement[indexVal].getId());
    }

    // Endurance Master (3hr)
    indexVal = 2;
    if (!lstAchievement[indexVal].isCompleted() && secTotal >= 60*60*3) {
        lstAchievement[indexVal].setCompleted(true);
        emit achievementCompleted(lstAchievement.at(indexVal));
        AchievementDAO::putAchievement(account->id, lstAchievement[indexVal].getId());
    }

    // Target Maniac (all target present in a workout)
    indexVal = 3;
    if (!lstAchievement[indexVal].isCompleted() && AchievementChecker::checkTargetManiac(workout)) {
        lstAchievement[indexVal].setCompleted(true);
        emit achievementCompleted(lstAchievement.at(indexVal));
        AchievementDAO::putAchievement(account->id, lstAchievement[indexVal].getId() );
    }

    // Learning to test (FTP Test or FTP_8min Test done)
    indexVal = 4;
    if (!lstAchievement[indexVal].isCompleted() && (workout.getWorkoutNameEnum() == Workout::FTP_TEST || workout.getWorkoutNameEnum() == Workout::FTP8min_TEST) ) {
        lstAchievement[indexVal].setCompleted(true);
        emit achievementCompleted(lstAchievement.at(indexVal));
        AchievementDAO::putAchievement(account->id, lstAchievement[indexVal].getId() );
    }





}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ManagerAchievement::slotGetAchievementListFinished() {


    //success, process data
    if (replyGetListAchievement->error() == QNetworkReply::NoError) {
        qDebug() << "no error process data achievement list!";
        QByteArray arrayData =  replyGetListAchievement->readAll();
        QString replyMsg(arrayData);
        lstAchievement =  Util::parseJsonAchievementList(replyMsg);
        qDebug() << "slotGetAchievementListFinished" << lstAchievement.size();

        replyGetListAchievement->deleteLater();
        replyGetListAchievementForUser = AchievementDAO::getLstAchievementForUser(account->id);
        connect(replyGetListAchievementForUser, SIGNAL(finished()), this, SLOT(slotGetAchievementListForUserFinished()) );
    }
    // error, retry request
    else {
        qDebug() << "Problem getting achievement list! retry again..." << replyGetListAchievement->errorString();
        replyGetListAchievement = AchievementDAO::getLstAchievement();
        connect(replyGetListAchievement, SIGNAL(finished()), this, SLOT(slotGetAchievementListFinished()) );
    }

}





//get lst ID achievement completed by user
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ManagerAchievement::slotGetAchievementListForUserFinished() {


    //should never happen
    if (lstAchievement.size() < numberOfAchievement)
        return;


    //success, process data
    if (replyGetListAchievementForUser->error() == QNetworkReply::NoError) {

        qDebug() << "GOT HERE#33 slotGetAchievementListForUserFinished Size of the list is!" << lstAchievement.size();
        QByteArray arrayData =  replyGetListAchievementForUser->readAll();
        QString replyMsg(arrayData);
        QSet<int> hashAchievementDone =  Util::parseJsonAchievementListForUser(replyMsg);

        //        foreach (Achievement aa, lstAchievement) {
        //            qDebug() << aa.getName() << " Completed?" << aa.isCompleted();
        //        }

        foreach (int val, hashAchievementDone) {
            qDebug() << "THIS IS IS DONEA CHIEVEMENT!" << val;
            if (val-1 < lstAchievement.size())
                lstAchievement[val-1].setCompleted(true);
        }

        //        foreach (Achievement aa, lstAchievement) {
        //            qDebug() << aa.getName() << " Completed?" << aa.isCompleted();
        //        }

        replyGetListAchievementForUser->deleteLater();
    }
    // error, retry request
    else {
        qDebug() << "Problem getting achievement list for user! retry again..." << replyGetListAchievementForUser->errorString();
        replyGetListAchievementForUser = AchievementDAO::getLstAchievementForUser(account->id);
        connect(replyGetListAchievementForUser, SIGNAL(finished()), this, SLOT(slotGetAchievementListForUserFinished()) );

    }



}
