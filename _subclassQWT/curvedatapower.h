#ifndef CURVEDATAPOWER_H
#define CURVEDATAPOWER_H

#include <qwt_series_data.h>
#include <qpointer.h>

class DataPower;

class CurveDataPower: public QwtSeriesData<QPointF>
{
public:
    const DataPower &values() const;
    DataPower &values();

    virtual QPointF sample( size_t i ) const;
    virtual size_t size() const;

    virtual QRectF boundingRect() const;
};

#endif // CURVEDATAPOWER_H


