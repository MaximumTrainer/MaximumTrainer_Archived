#include "intervaldata.h"

IntervalData::~IntervalData() {

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
IntervalData::IntervalData() {

    startTimeInterval.setTimeSpec(Qt::UTC);

    isFreeRideInterval = false;

    /// Cadence
    avgCadence = 0;
    maxCadence = 0;
    nbCadencePoints = 0;

    /// Hr
    avgHr = 0;
    maxHr = 0;
    nbHrPoints = 0;

    /// Power
    avgPower = 0;
    maxPower = 0;
    nbPowerPoints = 0;

    /// Speed
    avgSpeed = 0.0;
    maxSpeed = 0.0;
    nbSpeedPoints = 0;

    /// Calories
    calories = 0.0;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
IntervalData::IntervalData( const IntervalData& other ) {

    this->isFreeRideInterval = other.isFreeRideInterval;

    this->startTimeInterval = other.startTimeInterval;
    this->timeSecInterval = other.timeSecInterval;

    this->avgCadence = other.avgCadence;
    this->maxCadence = other.maxCadence;
    this->nbCadencePoints = other.nbCadencePoints;

    this->avgHr = other.avgHr;
    this->maxHr = other.maxHr;
    this->nbHrPoints = other.nbHrPoints;

    this->avgPower = other.avgPower;
    this->maxPower = other.maxPower;
    this->nbPowerPoints = other.nbPowerPoints;

    this->avgSpeed = other.avgSpeed;
    this->maxSpeed = other.maxSpeed;
    this->nbSpeedPoints = other.nbSpeedPoints;

    this->calories = other.calories;


}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//bool IntervalData::operator==(const IntervalData &other) const {
//    if (this->timeSecInterval == other.timeSecInterval && this->startTimeInterval == other.startTimeInterval)
//        return true;
//    return false;
//}
