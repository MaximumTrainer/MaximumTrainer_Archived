#include "achievementchecker.h"

#include "interval.h"
#include <QDebug>

AchievementChecker::AchievementChecker()
{

}

AchievementChecker::~AchievementChecker()
{

}



//////////////////////////////////////////////////////////////////////////////////////////////
bool AchievementChecker::checkTargetManiac(Workout workout) {

    qDebug() << "AchievementTargetManiac, check completed";


    bool powerTarget = false;
    bool powerBalanceTarget = false;
    bool cadenceTarget = false;
    bool heartRateTarget = false;

    foreach (Interval interval, workout.getLstInterval()) {

        qDebug() << "CHECKING INTERVAL NOW!!!!";

        if (interval.getPowerStepType() != Interval::NONE)
            powerTarget = true;

        if (interval.getRightPowerTarget() != -1)
            powerBalanceTarget = true;

        if (interval.getCadenceStepType() != Interval::NONE)
            cadenceTarget = true;

        if (interval.getHRStepType() != Interval::NONE)
            heartRateTarget = true;
    }

    if (powerTarget && powerBalanceTarget && cadenceTarget && heartRateTarget) {
        qDebug() << "AchievementTargetManiac, check completed yes!";
        return true;
    }
    qDebug() << "AchievementTargetManiac, check completed no!";
    return false;

}
