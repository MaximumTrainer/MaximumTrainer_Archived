#ifndef CURVEDATASPEED_H
#define CURVEDATASPEED_H

#include <qwt_series_data.h>
#include <qpointer.h>

class DataSpeed;

class CurveDataSpeed: public QwtSeriesData<QPointF>
{
public:
    const DataSpeed &values() const;
    DataSpeed &values();

    virtual QPointF sample( size_t i ) const;
    virtual size_t size() const;

    virtual QRectF boundingRect() const;
};

#endif // CURVEDATASPEED_H





