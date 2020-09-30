#include "course.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Course::Course() {

    this->courseType = Course::COURSE_TYPE::INCLUDED;
    this->filePath = "";
    this->name = "";
    this->description = "";

    this->maxSlopePercentage = 0;
    this->elevationMin = 0;
    this->elevationMax = 0;
    this->elevationDiff = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Course::Course(QString filePath, COURSE_TYPE courseType, QString name, QString location, QString description, QList<Trackpoint> lstTrack) {

    this->filePath = filePath;
    this->courseType = courseType;
    this->name = name;
    this->location = location;
    this->description = description;
    this->lstTrack = lstTrack;

    if (lstTrack.size() > 2)
        calculateCourseData();

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Course::Course( const Course& other ) {

    this->filePath = other.filePath;
    this->courseType = other.courseType;
    this->name = other.name;
    this->location = other.location;
    this->description = other.description;
    this->lstTrack = other.lstTrack;

    this->maxSlopePercentage = other.maxSlopePercentage;
    this->elevationMin = other.elevationMin;
    this->elevationMax = other.elevationMax;
    this->elevationDiff = other.elevationDiff;
    this->distanceMeters = other.distanceMeters;



}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Course::calculateCourseData() {


    Trackpoint firstTrackpoint = lstTrack.at(0);
    Trackpoint lastTrackpoint = lstTrack.at(lstTrack.size()-1);

    double lmaxSlopePercentage = 0;
    double lelevationMin = firstTrackpoint.getElevation();
    double lelevationMax = 0;

    foreach(Trackpoint tp, lstTrack) {

        if (tp.getSlopePercentage() > lmaxSlopePercentage) {
            lmaxSlopePercentage = tp.getSlopePercentage();
        }
        if (tp.getElevation() > lelevationMax) {
            lelevationMax = tp.getElevation();
        }
        if (tp.getElevation() < lelevationMin)
            lelevationMin = tp.getElevation();
    }


    this->maxSlopePercentage = lmaxSlopePercentage;
    this->elevationMin = lelevationMin;
    this->elevationMax = lelevationMax;
    this->distanceMeters = lastTrackpoint.getDistanceAtThisPoint();
}

