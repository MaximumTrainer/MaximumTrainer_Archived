#include "curvedataheartrate.h"
#include "dataheartrate.h"


const DataHeartRate &CurveDataHeartRate::values() const
{
    return DataHeartRate::instance();
}

DataHeartRate &CurveDataHeartRate::values()
{
    return DataHeartRate::instance();
}

QPointF CurveDataHeartRate::sample( size_t i ) const
{
    return DataHeartRate::instance().value( i );
}

size_t CurveDataHeartRate::size() const
{
    return DataHeartRate::instance().size();
}

QRectF CurveDataHeartRate::boundingRect() const
{
    return DataHeartRate::instance().boundingRect();
}


