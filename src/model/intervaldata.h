#ifndef INTERVALDATA_H
#define INTERVALDATA_H

#include <QtCore>


class IntervalData
{


public:
    IntervalData();
    IntervalData( const IntervalData& other );
    ~IntervalData();
//    bool operator==(const IntervalData &other) const;


    bool isFreeRideInterval;

    QDateTime startTimeInterval;
    int timeSecInterval;


    //////////////////////////////////////////////////////////////////////////////////////////
    /// Cadence
    double avgCadence;
    int maxCadence;
    int nbCadencePoints;

    /// Hr
    double avgHr;
    int maxHr;
    int nbHrPoints;

    /// Power
    double avgPower;
    int maxPower;
    int nbPowerPoints;

    /// Speed
    double avgSpeed;
    double maxSpeed;
    int nbSpeedPoints;

    /// Calories
    double calories;






};
Q_DECLARE_METATYPE(IntervalData)


#endif // INTERVALDATA_H
