#ifndef CURVEDATACADENCE_H
#define CURVEDATACADENCE_H

#include <qwt_series_data.h>
#include <qpointer.h>


class DataCadence;

class CurveDataCadence: public QwtSeriesData<QPointF>
{
public:
    const DataCadence &values() const;
    DataCadence &values();

    virtual QPointF sample( size_t i ) const;
    virtual size_t size() const;

    virtual QRectF boundingRect() const;
};

#endif // CURVEDATACADENCE_H








