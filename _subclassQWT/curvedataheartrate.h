#ifndef CURVEDATAHEARTRATE_H
#define CURVEDATAHEARTRATE_H

#include <qwt_series_data.h>
#include <qpointer.h>

class DataHeartRate;

class CurveDataHeartRate: public QwtSeriesData<QPointF>
{
public:
    const DataHeartRate &values() const;
    DataHeartRate &values();

    virtual QPointF sample( size_t i ) const;
    virtual size_t size() const;

    virtual QRectF boundingRect() const;
};


#endif // CURVEDATAHEARTRATE_H



