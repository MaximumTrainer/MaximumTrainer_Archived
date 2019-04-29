#include "trackpoint.h"


Trackpoint::Trackpoint() {

    this->lon=0;
    this->lat=0;
    this->elevation=0;
    this->slopePercentage = 0;
    this->distanceAtThisPoint=0;
}


//////////////////////////////////////////////////////////////////////////////////////////////
Trackpoint::Trackpoint(double lon, double lat, double elevation, double slopePercentage, double distanceAtThisPoint) {

    this->lon = lon;
    this->lat = lat;
    this->elevation = elevation;
    this->slopePercentage = slopePercentage;
    this->distanceAtThisPoint = distanceAtThisPoint;
}



Trackpoint::Trackpoint( const Trackpoint& other ) {

    this->lon = other.lon;
    this->lat = other.lat;
    this->elevation = other.elevation;
    this->slopePercentage = other.slopePercentage;
    this->distanceAtThisPoint = other.distanceAtThisPoint;
}


