#ifndef REPORTUTIL_H
#define REPORTUTIL_H

#include "qwt_plot.h"
#include "workout.h"


class ReportUtil
{
public:
    ReportUtil();


    static void printWorkoutToPdf(Workout workout, QwtPlot *plot, QString filename);

};

#endif // REPORTUTIL_H
