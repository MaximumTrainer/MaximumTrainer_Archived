#ifndef TRACKPOINT_H
#define TRACKPOINT_H



#include <QtCore>


class Trackpoint
{

public:
    Trackpoint();
    ~Trackpoint() {}
    Trackpoint( const Trackpoint& other );

    Trackpoint(double lon, double lat, double elevation, double slopePercentage, double distanceAtThisPoint);

    // Getters
    double getLon() const {
        return this->lon;
    }
    double getLat() const {
        return this->lat;
    }

    double getElevation() const {
        return this->elevation;
    }
    double getSlopePercentage() const {
        return this->slopePercentage;
    }
    double getDistanceAtThisPoint() const {
        return this->distanceAtThisPoint;
    }
    // Setters
    void setDistanceAtThisPoint(double dist) {
        this->distanceAtThisPoint = dist;
    }
    void setSlopePercentage(double perc) {
        this->slopePercentage = perc;
    }
    void setElevation(double ele) {
        this->elevation = ele;
    }

private :

    double lon;
    double lat;
    double elevation;

    double slopePercentage;
    double distanceAtThisPoint; //m


};
Q_DECLARE_METATYPE(Trackpoint)

#endif // TRACKPOINT_H
