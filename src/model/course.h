#ifndef COURSE_H
#define COURSE_H

#include <QString>
#include "trackpoint.h"


class Course
{

public:


    //----------------------------
    enum COURSE_TYPE
    {
        USER_MADE,
        INCLUDED,
    };


    Course();
    ~Course() {}
    Course( const Course& other );
    Course(QString filePath, COURSE_TYPE courseType, QString name, QString location, QString description, QList<Trackpoint> lstTrack);

    //getters
    QString getFilePath() const {
        return this->filePath;
    }
    COURSE_TYPE getCourseType() const {
        return this->courseType;
    }
    QString getName() const {
        return this->name;
    }
    QString getLocation() const {
        return this->location;
    }
    QString getDescription() const {
        return this->description;
    }
    QList<Trackpoint> getLstTrack() const {
        return this->lstTrack;
    }

    double getMaxSlopePercentage() const {
        return this->maxSlopePercentage;
    }
    double getElevationMin() const {
        return this->elevationMin;
    }
    double getElevationMax() const {
        return this->elevationMax;
    }
    double getElevationDiff() const {
        return (this->elevationMax - this->elevationMin);
    }
    double getDistanceMeters() const {
        return this->distanceMeters;
    }

    //setters
    void setFilePath(QString filepath) {
        this->filePath = filepath;
    }
    void setCourseType(COURSE_TYPE courseType) {
        this->courseType = courseType;
    }



private :
    void calculateCourseData();


private:

    QString filePath;
    COURSE_TYPE courseType;
    QString name;
    QString location;
    QString description; 
    QList<Trackpoint> lstTrack;

    //calculated
    double maxSlopePercentage;
    double elevationMin;
    double elevationMax;
    double elevationDiff; //max from min to max elevation
    double distanceMeters;

};
Q_DECLARE_METATYPE(Course)

#endif // COURSE_H
