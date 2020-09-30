#include "gpxparser.h"

#include <QDebug>
#include <QFile>
#include <QApplication>
#include <QtMath>


GpxParser::~GpxParser()
{

}



GpxParser::GpxParser()
{

}


///////////////////////////////////////////////////////////////////////////////
QList<Trackpoint> GpxParser::parseFile(QString filePath, double minimumDistanceBetweenPoint) {


    qDebug() << "PARSING GPX FILE NOW!" << filePath;




    QFileInfo fileInfo(filePath);
    QString fileSuffix = fileInfo.completeSuffix();
    fileSuffix = fileSuffix.toLower();


    QList<Trackpoint> lstTrack;
    QList<Trackpoint> lstTrkptCondensed;

    if (fileSuffix == "gpx") {
        lstTrack = parseGpxFile(filePath);
    }
    else if (fileSuffix == "tcx") {
        lstTrack = parseTcxFile(filePath);
    }
    else if (fileSuffix == "kml") {
        lstTrack = parseKmlFile(filePath);
    }
    else { //other format not supported
        return lstTrack;
    }

    if (lstTrack.size() < 2) {
        qDebug() << "error happened while parsing file...leave";
        return lstTrack;
    }


    qDebug() << "number of trackpoint1:" << lstTrack.size();

    //-------- Calculate Distance at each point, filter point
    double totalDistance = 0;
    QList<Trackpoint> filteredList;
    filteredList.append(lstTrack.at(0)); //add first point (distance=0)
    for (int i=1; i<lstTrack.size(); i++) {

        Trackpoint tk0 = lstTrack.at(i-1);
        Trackpoint tk1 = lstTrack.at(i);

        double distanceSinceLast = distVincenty(tk0, tk1);
        // Distance should be positive and valid
        if (distanceSinceLast != -1) {
            if (distanceSinceLast > 0 || i == lstTrack.size()-1) {
                totalDistance += distanceSinceLast;
                tk1.setDistanceAtThisPoint(totalDistance);
                filteredList.append(tk1);
            }
        }
    }
    lstTrack = filteredList;

    qDebug() << "number of trackpoint2:" << lstTrack.size();



    // ---------- Only keep point with a minimum distance between them
    double accumulatedDistance = 0;
    double lastPointInsertedDistance = 0;
    QList<Trackpoint> filteredList2;
    filteredList2.append(lstTrack.at(0)); //add first point (distance=0)
    for (int i=1; i<lstTrack.size(); i++) {

        Trackpoint tk0 = lstTrack.at(i-1);
        Trackpoint tk1 = lstTrack.at(i);

        double distanceDif = tk1.getDistanceAtThisPoint() - tk0.getDistanceAtThisPoint();
        accumulatedDistance += distanceDif;
        if (accumulatedDistance > minimumDistanceBetweenPoint || i==lstTrack.size()-1) {
            double distanceNow = lastPointInsertedDistance + accumulatedDistance;
            tk1.setDistanceAtThisPoint(distanceNow);
            filteredList2.append(tk1);
            accumulatedDistance = 0;
            lastPointInsertedDistance = distanceNow;
        }

    }
    lstTrack = filteredList2;


    qDebug() << "number of trackpoint3:" << lstTrack.size();
    Trackpoint tpLast = lstTrack.at(lstTrack.size()-1);
    qDebug() << "last point distance is:" << tpLast.getDistanceAtThisPoint();
    qDebug() << "calc over! total distance is" << totalDistance;
    qDebug() << "number of trackpoint:" << lstTrack.size() << " Condensed list:" << lstTrkptCondensed.size();



    return lstTrack;
}

///////////////////////////////////////////////////////////////////////////////////////////////
QList<Trackpoint> GpxParser::parseGpxFile(QString filePath) {

    qDebug() << "parseGpxFile";
    QFile file(filePath);
    QXmlStreamReader xml(&file);

    QList<Trackpoint> lstTrack;


    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "problem parseTcxFile" << filePath;
        return lstTrack;
    }
    else {
        while(!xml.atEnd()) {
            if (xml.hasError()) {
                qDebug() << "Error in XML - parseCourseFile" << xml.error();
                return lstTrack;
            }
            xml.readNext();
            if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == "trkpt") {
                Trackpoint tp = parseTrackpointGpx(xml);
                lstTrack.append(tp);
            }
        }
    }
    return lstTrack;
}
///////////////////////////////////////////////////////////////////////////////////////////////
QList<Trackpoint> GpxParser::parseTcxFile(QString filePath) {

    qDebug() << "parseTcxFile";
    QFile file(filePath);
    QXmlStreamReader xml(&file);

    QList<Trackpoint> lstTrack;

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "problem parseTcxFile" << filePath;
        return lstTrack;
    }
    else {
        while(!xml.atEnd()) {
            if (xml.hasError()) {
                qDebug() << "Error in XML - parseCourseFile" << xml.error();
                return lstTrack;
            }
            xml.readNext();
            if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == "Trackpoint") {
                Trackpoint tp = parseTrackpointTcx(xml);
                lstTrack.append(tp);
            }
        }
    }
    return lstTrack;
}
///////////////////////////////////////////////////////////////////////////////////////////////
QList<Trackpoint> GpxParser::parseKmlFile(QString filePath) {

    qDebug() << "parseKmlFile";
    QFile file(filePath);
    QXmlStreamReader xml(&file);

    QList<Trackpoint> lstTrack;

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "problem parseKmlFile" << filePath;
        return lstTrack;
    }
    else {
        while(!xml.atEnd()) {
            if (xml.hasError()) {
                qDebug() << "Error in XML - parseCourseFile" << xml.error();
                return lstTrack;
            }
            xml.readNext();
            if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == "coordinates") {

                QString blockText = xml.readElementText().remove(" ");
                blockText = blockText.replace("\n", ",");
                QStringList lstCoords = blockText.split(",");

                if (lstCoords.size() < 1)
                    return lstTrack ;

                if (lstCoords.at(0) == "" )
                    lstCoords.removeAt(0);
                if (lstCoords.at(lstCoords.size()-1) == "")
                    lstCoords.removeAt(lstCoords.size()-1);


                int i=0;
                while (i < lstCoords.size()) {

                    if (i + 2 >= lstCoords.size() -1)
                        break;

                    double lon = lstCoords.at(i).toDouble();
                    i++;
                    double lat = lstCoords.at(i).toDouble();
                    i++;
                    double ele = lstCoords.at(i).toDouble();
                    i++;

                    //                    qDebug() << "lon:" << lon << "lat:" << lat << "ele" << ele << "I is:" << i;
                    Trackpoint tp(lon, lat, ele, 0, 0);
                    lstTrack.append(tp);
                }
            }
        }
    }
    return lstTrack;
}

///////////////////////////////////////////////////////////////////////////////////////////////
Trackpoint GpxParser::parseTrackpointGpx(QXmlStreamReader& xml) {

    double lon = 0;
    double lat = 0;
    double ele = 0;

    //Read Lon, Lat
//    foreach(const QXmlStreamAttribute &attr, xml.attributes()) {
//        if (attr.name().compare("lon", Qt::CaseInsensitive) == 0) {
//            lon = attr.value().toDouble();
//        }
//        else  if (attr.name().compare("lat", Qt::CaseInsensitive) == 0) {
//            lat = attr.value().toDouble();
//        }
//    }

//    while (xml.tokenType() != QXmlStreamReader::EndElement || xml.name() != "trkpt") {
//        xml.readNext();

//        if (xml.name().compare("ele", Qt::CaseInsensitive) == 0) {
//            ele  = xml.readElementText().toDouble();
//            //            qDebug() << "found ele!" << ele;
//        }
//    }
    //    qDebug() << "---------------done reading parseTrackpointGpx-----------------\n";

    Trackpoint trackpoint(lon, lat, ele, 0, 0);
    return trackpoint;
}
///////////////////////////////////////////////////////////////////////////////////////////////
Trackpoint GpxParser::parseTrackpointTcx(QXmlStreamReader& xml) {

    double lon = 0;
    double lat = 0;
    double ele = 0;


//    while (xml.tokenType() != QXmlStreamReader::EndElement || xml.name() != "Trackpoint") {
//        xml.readNext();

//        if (xml.name().compare("LatitudeDegrees", Qt::CaseInsensitive) == 0) {
//            lat  = xml.readElementText().toDouble();
//            //            qDebug() << "found LatitudeDegrees!" << lat;
//        }
//        else if (xml.name().compare("LongitudeDegrees", Qt::CaseInsensitive) == 0) {
//            lon  = xml.readElementText().toDouble();
//            //            qDebug() << "found LongitudeDegrees!" << lon;
//        }
//        else if (xml.name().compare("AltitudeMeters", Qt::CaseInsensitive) == 0) {
//            ele  = xml.readElementText().toDouble();
//            //            qDebug() << "found AltitudeMeters!" << ele;
//        }
//    }
    //    qDebug() << "---------------done reading parseTrackpointTcx-----------------\n";

    Trackpoint trackpoint(lon, lat, ele, 0, 0);
    return trackpoint;
}





//http://maps.google.com/?saddr=45.51731077954173,-73.52830465883017&daddr=45.517270965501666,-73.52829476818442

//////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Calculates geodetic distance between two points specified by latitude/longitude using
 * Vincenty inverse formula for ellipsoids
 *
 * @param   {Number} lat1, lon1: first point in decimal degrees
 * @param   {Number} lat2, lon2: second point in decimal degrees
 * @returns (Number} distance in metres between points
 */
double GpxParser::distVincenty(Trackpoint tp1, Trackpoint tp2) {


    double lat1 = tp1.getLat();
    double lon1 = tp1.getLon();
    double lat2 = tp2.getLat();
    double lon2 = tp2.getLon();

    //same point, do not calculate to save process time
    if (lon1 == lon2 && lat1 == lat2)
        return 0;

    double a = 6378137;
    double b = 6356752.314245;
    double f = 1/298.257223563;  // WGS-84 ellipsoid params

    double L = qDegreesToRadians(lon2-lon1);

    double U1 = qAtan((1-f) * qTan(qDegreesToRadians(lat1)));
    double U2 = qAtan((1-f) * qTan(qDegreesToRadians(lat2)));
    double sinU1 = qSin(U1);
    double cosU1 = qCos(U1);
    double sinU2 = qSin(U2);
    double cosU2 = qCos(U2);

    double lambda = L;
    double lambdaP = 100;
    double iterLimit = 100;

    double sinLambda, cosLambda, sinSigma, cosSigma, sigma, sinAlpha, cosSqAlpha, cos2SigmaM;
    do {
        sinLambda = qSin(lambda);
        cosLambda = qCos(lambda);
        sinSigma = qSqrt((cosU2*sinLambda) * (cosU2*sinLambda) +
                         (cosU1*sinU2-sinU1*cosU2*cosLambda) * (cosU1*sinU2-sinU1*cosU2*cosLambda));

        if (sinSigma==0) return 0;  // co-incident points
        cosSigma = sinU1*sinU2 + cosU1*cosU2*cosLambda;
        sigma = qAtan2(sinSigma, cosSigma);
        sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;
        cosSqAlpha = 1 - sinAlpha*sinAlpha;
        cos2SigmaM = cosSigma - 2*sinU1*sinU2/cosSqAlpha;
        if (isnan(cos2SigmaM)) cos2SigmaM = 0;  // equatorial line: cosSqAlpha=0 (§6)
        double C = f/16*cosSqAlpha*(4+f*(4-3*cosSqAlpha));
        lambdaP = lambda;
        lambda = L + (1-C) * f * sinAlpha *
                (sigma + C*sinSigma*(cos2SigmaM+C*cosSigma*(-1+2*cos2SigmaM*cos2SigmaM)));
    }
    while (qAbs(lambda-lambdaP) > 1e-12 && --iterLimit>0);

    if (iterLimit==0) return -1;  // formula failed to converge

    double uSq = cosSqAlpha * (a*a - b*b) / (b*b);
    double A = 1 + uSq/16384*(4096+uSq*(-768+uSq*(320-175*uSq)));
    double B = uSq/1024 * (256+uSq*(-128+uSq*(74-47*uSq)));
    double deltaSigma = B*sinSigma*(cos2SigmaM+B/4*(cosSigma*(-1+2*cos2SigmaM*cos2SigmaM)-
                                                    B/6*cos2SigmaM*(-3+4*sinSigma*sinSigma)*(-3+4*cos2SigmaM*cos2SigmaM)));
    double s = b*A*(sigma-deltaSigma);

    //    s = s.toFixed(3); // round to 1mm precision
    return s;

    // note: to return initial/final bearings in addition to distance, use something like:
    //    var fwdAz = Math.atan2(cosU2*sinLambda,  cosU1*sinU2-sinU1*cosU2*cosLambda);
    //    var revAz = Math.atan2(cosU1*sinLambda, -sinU1*cosU2+cosU1*sinU2*cosLambda);
    //    return { distance: s, initialBearing: fwdAz.toDeg(), finalBearing: revAz.toDeg() };


}

