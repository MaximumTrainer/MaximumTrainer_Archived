#ifndef WORKOUTUTIL_H
#define WORKOUTUTIL_H

#include "workout.h"
#include "interval.h"



class WorkoutUtil
{
public:

    WorkoutUtil();


    static QList<Workout> getListWorkoutBase();
    static Workout getWorkoutMap(int userFTP);



    //return the start of the video in msec (where to start workout)
    static int startVideoSufferfest(QString sufferfestWorkoutName);


private:

    static Workout CP5();
    static Workout CP20();

    static Workout FTP();
    static Workout FTP_8min();

    static Workout MAP(int userFTP);




};

#endif // WORKOUTUTIL_H
