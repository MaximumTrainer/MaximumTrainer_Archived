#ifndef ACHIEVEMENTCHECKER_H
#define ACHIEVEMENTCHECKER_H

#include "workout.h"

class AchievementChecker
{
public:
    AchievementChecker();
    ~AchievementChecker();


    static bool checkTargetManiac(Workout workout);
};

#endif // ACHIEVEMENTCHECKER_H
