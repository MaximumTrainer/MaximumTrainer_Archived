#ifndef GPXPARSER_H
#define GPXPARSER_H

#include <QtCore>
#include <QXmlStreamReader>
#include "trackpoint.h"

class GpxParser
{
public:

    GpxParser();
    ~GpxParser();


    QList<Trackpoint> parseFile(QString filePath, double minimumDistanceBetweenPoint);



private :
    QList<Trackpoint> parseGpxFile(QString filePath);
    QList<Trackpoint> parseTcxFile(QString filePath);
    QList<Trackpoint> parseKmlFile(QString filePath);

    Trackpoint parseTrackpointGpx(QXmlStreamReader&);
    Trackpoint parseTrackpointTcx(QXmlStreamReader&);


    double distVincenty(Trackpoint tp1, Trackpoint tp2);


};

#endif // GPXPARSER_H
